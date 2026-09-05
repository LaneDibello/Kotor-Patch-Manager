#include "wrapper_x86.h"
#include "patcher.h"
#include "platform.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace KotorPatcher {
    namespace Wrappers {
        // FXSAVE writes 512 bytes and needs a 16-byte aligned destination, so the
        // reserved area carries enough slack to find one wherever the game's ESP
        // happened to be. Present on every CPU with SSE, which these games require.
        constexpr int kFpSaveSize = 512;
        constexpr int kFpAreaSize = kFpSaveSize + 16;

        WrapperGenerator_x86::WrapperGenerator_x86() {
        }

        WrapperGenerator_x86::~WrapperGenerator_x86() {
            FreeAllWrappers();
        }

        void* WrapperGenerator_x86::AllocateExecutableMemory(size_t size, uintptr_t nearAddress) {
            void* mem = Platform::AllocExec(size, nearAddress);
            if (mem) {
                m_allocatedWrappers.push_back({ mem, size });
            }
            return mem;
        }

        void WrapperGenerator_x86::FreeAllWrappers() {
            for (const auto& wrapper : m_allocatedWrappers) {
                Platform::FreeExec(wrapper.address, wrapper.size);
            }
            m_allocatedWrappers.clear();
        }

        void WrapperGenerator_x86::EmitBytes(uint8_t*& code, const uint8_t* bytes, size_t count) {
            std::memcpy(code, bytes, count);
            code += count;
        }

        void WrapperGenerator_x86::EmitByte(uint8_t*& code, uint8_t value) {
            *code++ = value;
        }

        void WrapperGenerator_x86::EmitDword(uint8_t*& code, uint32_t value) {
            std::memcpy(code, &value, 4);
            code += 4;
        }

        void WrapperGenerator_x86::EmitFpStateAccess(uint8_t*& code, int savedStateSize, bool restore) {
            // EAX is borrowed and given back. On the way out it still holds the
            // handler's return value, which the consumed-exit test is about to read,
            // and which the caller was told to exclude from restore so it survives.
            EmitByte(code, 0x50);  // PUSH EAX

            // LEA EAX, [EBX + savedStateSize]: the FXSAVE area sits just above the
            // pushed state, and EBX is what still points at it
            EmitByte(code, 0x8D);  // LEA r32, m
            EmitByte(code, 0x83);  // ModRM: EAX, [EBX + disp32]
            EmitDword(code, static_cast<uint32_t>(savedStateSize));

            // Round the address up to the 16-byte boundary FXSAVE requires.
            // ADD and AND write EFLAGS, which is why both callers emit this while
            // the flags are saved.
            EmitByte(code, 0x83);  // ADD r/m32, imm8
            EmitByte(code, 0xC0);  // ModRM: EAX
            EmitByte(code, 0x0F);  // imm8: 15

            EmitByte(code, 0x83);  // AND r/m32, imm8
            EmitByte(code, 0xE0);  // ModRM: EAX
            EmitByte(code, 0xF0);  // imm8: -16

            EmitByte(code, 0x0F);  // FXSAVE / FXRSTOR
            EmitByte(code, 0xAE);
            EmitByte(code, restore ? 0x08 : 0x00);  // ModRM: [EAX], /1 restore or /0 save

            EmitByte(code, 0x58);  // POP EAX
        }

        uint32_t WrapperGenerator_x86::CalculateRelativeOffset(void* from, void* to) {
            // For relative JMP/CALL: offset = target - (source + 5)
            // The +5 accounts for the instruction size (1 byte opcode + 4 byte offset)
            return reinterpret_cast<uint32_t>(to) - (reinterpret_cast<uint32_t>(from) + 5);
        }

        void* WrapperGenerator_x86::GenerateWrapper(const WrapperConfig& config) {
            return GenerateDetourWrapper(config);
        }

        void* WrapperGenerator_x86::GenerateDetourWrapper(const WrapperConfig& config) {
            // Estimate wrapper size
            // Base: ~100 bytes, +10 per excluded register, +16 for the
            // consumed-exit conditional (12 bytes emitted, padded to 16)
            // Original bytes are copied into the stub verbatim, so they are counted
            // here rather than left to the base's headroom
            // +40 for the two FXSAVE sequences and the two frame adjustments
            size_t estimatedSize = 168 + config.originalBytes.size() +
                                   (config.excludeFromRestore.size() * 10);
            if (config.consumedExitAddress != 0) {
                estimatedSize += 16;
            }

            // The hook site jumps here and the wrapper jumps back, so the stub has to
            // land within a relative jump of the code it is hooking.
            uint8_t* wrapperMem = static_cast<uint8_t*>(
                AllocateExecutableMemory(estimatedSize, config.hookAddress));
            if (!wrapperMem) {
                Platform::Log("[Wrapper] Failed to allocate wrapper memory\n");
                return nullptr;
            }

            uint8_t* code = wrapperMem;  // Current write position

            // ===== PROLOGUE: Save CPU State =====
            // Room for the floating-point state first, before anything is pushed.
            // FXSAVE writes 512 bytes to a 16-byte aligned address; nothing is known
            // about the game's ESP, so the area carries 15 bytes of slack and the
            // aligned address inside it is worked out below, once EAX is free.
            // LEA ESP, [ESP - kFpAreaSize]
            EmitByte(code, 0x8D);  // LEA r32, m
            EmitByte(code, 0xA4);  // ModRM: ESP, [ESP + disp32] (SIB follows)
            EmitByte(code, 0x24);  // SIB: [ESP]
            EmitDword(code, static_cast<uint32_t>(-kFpAreaSize));

            // We always save, whatever preserveRegisters and preserveFlags say. Those
            // decide what gets written back, not what gets kept:
            // 1. A parameter sourced from a register reads the saved copy, so it has
            //    to exist even when nothing is restored
            // 2. EBX below becomes our anchor, so the game's EBX must be somewhere

            // PUSHAD: Push all general-purpose registers
            // Order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
            EmitByte(code, 0x60);  // PUSHAD

            // PUSHFD: Push EFLAGS register
            EmitByte(code, 0x9C);  // PUSHFD

            // ===== CALCULATE STACK LAYOUT =====
            // At this point, the stack layout is:
            // [ESP+0]  = EFLAGS
            // [ESP+4]  = EDI    \
            // [ESP+8]  = ESI     |
            // [ESP+12] = EBP     |
            // [ESP+16] = (ESP)   | PUSHAD saved these (32 bytes total)
            // [ESP+20] = EBX     |
            // [ESP+24] = EDX     |
            // [ESP+28] = ECX     |
            // [ESP+32] = EAX    /
            // [ESP+36] = FXSAVE area, plus its alignment slack
            // [ESP+564] = Original stack data (parameters, etc.)
            // The hook site is reached by a JMP, not a CALL, so there is no return
            // address in between

            // Total bytes pushed onto the stack
            const int savedStateSize = 4 + 32;  // PUSHFD + PUSHAD

            // Save current ESP to EBX (points to our saved state)
            // We'll use this to read saved registers and restore ESP later
            // MOV EBX, ESP
            EmitByte(code, 0x89);  // MOV r/m32, r32
            EmitByte(code, 0xE3);  // ModRM: EBX = ESP

            // ===== SAVE FLOATING-POINT STATE =====
            // Everything FXSAVE covers is the patch function's to destroy: the x87
            // stack these games do their float work on, its control word, MMX, and
            // XMM with MXCSR. None of it is in PUSHAD.
            EmitFpStateAccess(code, savedStateSize, /*restore=*/false);

            // ===== ALIGN THE STACK FOR THE CALL =====
            // IMPORTANT: this is where ESP starts moving.
            // AND only rounds down, so the saved state stays above ESP and the
            // parameter pushes land below it. ExtractAndPushParameter reads from
            // EBX for that reason: ESP no longer points at the saved state.
            //
            // We align unconditionally because we cannot know what compiled the
            // patch DLL. MSVC and MinGW realign in their own prologue; GCC
            // targeting Linux or macOS assumes the caller did it and emits an
            // aligned move regardless. A hook site is an arbitrary instruction in
            // the game, so it promises neither.
            //
            // AND and SUB write EFLAGS. That is already given up here: with
            // preserveFlags the state was saved above and is restored below, and
            // without it the CALL clobbers the flags anyway.
            const int paramBytes = static_cast<int>(config.parameters.size()) * 4;
            const int alignmentPad = (16 - (paramBytes % 16)) % 16;

            // AND ESP, -16
            EmitByte(code, 0x83);  // AND r/m32, imm8
            EmitByte(code, 0xE4);  // ModRM: ESP
            EmitByte(code, 0xF0);  // imm8: -16

            if (alignmentPad != 0) {
                // SUB ESP, alignmentPad: leaves the pushes ending on a boundary
                EmitByte(code, 0x83);  // SUB r/m32, imm8
                EmitByte(code, 0xEC);  // ModRM: ESP
                EmitByte(code, static_cast<uint8_t>(alignmentPad));
            }

            // ===== EXTRACT AND PUSH PARAMETERS =====
            // If the hook has parameters defined, extract them and push onto stack
            // Parameters are pushed in reverse order for __cdecl (right-to-left)

            for (int i = static_cast<int>(config.parameters.size()) - 1; i >= 0; i--) {
                ExtractAndPushParameter(code, config.parameters[i], savedStateSize);
            }

            // ===== CALL PATCH FUNCTION =====
            // Now ESP points to the original stack layout (if no params)
            // Or has parameters pushed (if params specified)

            // CALL patch_function
            EmitByte(code, 0xE8);  // CALL rel32
            // Note: code now points one byte AFTER the 0xE8 opcode
            // CalculateRelativeOffset needs the address of the opcode itself
            uint32_t callOffset = CalculateRelativeOffset(code - 1, config.patchFunction);
            EmitDword(code, callOffset);

            // ===== RESTORE WRAPPER ESP =====
            // Restore ESP back to point to our saved state
            // This also undoes the alignment, the pad and the pushed parameters,
            // so __cdecl's caller-side cleanup is not emitted separately
            // MOV ESP, EBX
            EmitByte(code, 0x89);  // MOV r/m32, r32
            EmitByte(code, 0xDC);  // ModRM: ESP = EBX

            // ===== EPILOGUE: Restore CPU State =====

            // Ahead of the flags, because addressing the area writes them, and ahead
            // of POPAD, because it needs EAX
            if (config.preserveRegisters) {
                EmitFpStateAccess(code, savedStateSize, /*restore=*/true);
            }

            if (config.preserveFlags) {
                // POPFD: Restore EFLAGS
                EmitByte(code, 0x9D);  // POPFD
            } else {
                // Step past the saved EFLAGS without applying them
                // LEA ESP, [ESP+4]: ADD would set the flags we are declining to restore
                EmitByte(code, 0x8D);  // LEA r32, m
                EmitByte(code, 0x64);  // ModRM: ESP, [ESP+disp8] (SIB follows)
                EmitByte(code, 0x24);  // SIB: [ESP]
                EmitByte(code, 0x04);  // disp8 = 4
            }

            {
                // Handle register exclusions
                if (config.preserveRegisters && config.excludeFromRestore.empty()) {
                    // Simple case: restore all registers
                    EmitByte(code, 0x61);  // POPAD
                } else {
                    // Complex case: selectively restore registers
                    // POPAD pops in order: EDI, ESI, EBP, (ESP), EBX, EDX, ECX, EAX

                    // We need to manually pop each register
                    const char* regOrder[] = { "edi", "esi", "ebp", "esp", "ebx", "edx", "ecx", "eax" };
                    const uint8_t popOpcodes[] = { 0x5F, 0x5E, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58 };
                    constexpr int kEspSlot = 3;
                    constexpr int kEbxSlot = 4;

                    for (int i = 0; i < 8; i++) {
                        // Hardware POPAD never restores ESP — it just advances
                        // past the saved-ESP slot. Emitting `POP ESP` (0x5C)
                        // here would write ESP from the saved value, jumping
                        // past the remaining EBX/EDX/ECX/EAX slots and reading
                        // them from random stack memory. Always skip the ESP
                        // slot regardless of exclude_from_restore.
                        // EBX is always restored. The wrapper commandeered it as its
                        // anchor, so the patch function never had a say in its value:
                        // leaving it out would hand the game one of our stack addresses,
                        // not anything the patch chose. Excluding "ebx" cannot mean
                        // anything, so it is ignored rather than obeyed.
                        if (i != kEspSlot &&
                            (i == kEbxSlot || config.ShouldRestoreRegister(regOrder[i]))) {
                            EmitByte(code, popOpcodes[i]);  // POP reg
                        } else {
                            // Skip this register (matches POPAD's ESP semantics
                            // for the ESP slot, or honors the user's exclusion
                            // for other slots).
                            //
                            // Use LEA ESP, [ESP+4] instead of ADD ESP, 4: ADD
                            // sets EFLAGS (ZF/SF/CF/OF) from the result, which
                            // would clobber flag state the restored context or
                            // downstream target code depends on. LEA is the
                            // standard flag-preserving stack adjustment.
                            // 4 bytes vs 3 — a minor size cost for correctness.
                            EmitByte(code, 0x8D);  // LEA r32, m
                            EmitByte(code, 0x64);  // ModRM: ESP, [ESP+disp8] (SIB follows)
                            EmitByte(code, 0x24);  // SIB: [ESP]
                            EmitByte(code, 0x04);  // disp8 = 4
                        }
                    }
                }
            }

            // ===== CLOSE THE FRAME =====
            // Step past the FXSAVE area, back to the game's own ESP
            // LEA ESP, [ESP + kFpAreaSize]
            EmitByte(code, 0x8D);  // LEA r32, m
            EmitByte(code, 0xA4);  // ModRM: ESP, [ESP + disp32] (SIB follows)
            EmitByte(code, 0x24);  // SIB: [ESP]
            EmitDword(code, kFpAreaSize);

            // ===== EXECUTE STOLEN ORIGINAL BYTES =====
            // Run the original instructions BEFORE the conditional consumed-exit
            // jump so both paths leave the stack and registers in the same state.
            // Equivalent to the cut bytes having executed natively in-place. The
            // consumed_exit_address contract assumes this state, since the caller
            // chose the target by inspecting where execution would naturally land
            // after the cut.
            if (!config.skipOriginalBytes) {
                if (config.originalBytes.empty()) {
                    Platform::Log("[Wrapper] ERROR: No original bytes provided for DETOUR hook\n");
                    return nullptr;
                }
                EmitBytes(code, config.originalBytes.data(), config.originalBytes.size());
            }

            // ===== CONDITIONAL CONSUMED-EVENT EXIT =====
            // If the hook config specifies a consumed_exit_address, emit:
            //   PUSHFD                    ; preserve cut-bytes' EFLAGS state
            //   TEST EAX, EAX             ; clobbers ZF/SF/PF/CF/OF
            //   JZ +6                     ; skip POPFD + JMP rel32, fall through
            //   POPFD                     ; consumed path: restore EFLAGS
            //   JMP rel32 consumed_exit   ; handler returned non-zero -> consumed
            //   POPFD                     ; fall-through path: restore EFLAGS
            //
            // PUSHFD/POPFD wraps the TEST so that the cut bytes' flag state
            // (e.g. a CMP whose ZF the target's downstream Jcc reads) survives
            // through to the natural-resume JMP. Without it, a hook whose cut
            // ends in or directly precedes a flag-consuming Jcc would be
            // silently misrouted, since TEST EAX,EAX overwrites those flags.
            //
            // Caller is responsible for excluding "eax" from POPAD restoration
            // so the handler's return value reaches the TEST.
            if (config.consumedExitAddress != 0) {
                EmitByte(code, 0x9C);  // PUSHFD — save cut bytes' EFLAGS
                EmitByte(code, 0x85);  // TEST r/m32, r32
                EmitByte(code, 0xC0);  // ModRM: EAX, EAX
                EmitByte(code, 0x74);  // JZ rel8
                EmitByte(code, 0x06);  // skip POPFD (1) + JMP rel32 (5) = 6 bytes
                EmitByte(code, 0x9D);  // POPFD — restore EFLAGS for consumed path
                EmitByte(code, 0xE9);  // JMP rel32
                uint32_t consumedOffset = CalculateRelativeOffset(
                    code - 1,
                    reinterpret_cast<void*>(config.consumedExitAddress));
                EmitDword(code, consumedOffset);
                EmitByte(code, 0x9D);  // POPFD — restore EFLAGS for fall-through

                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg), "[Wrapper] Conditional consumed-exit -> 0x%08X emitted\n",
                    config.consumedExitAddress);
                Platform::Log(debugMsg);
            }

            // ===== JUMP BACK TO NATURAL FALL-THROUGH =====
            // hookAddress + originalBytes.size() is the resume point for the
            // non-consumed path. With cut bytes already emitted above, the
            // stack/register state at the target matches what the engine would
            // see after natively executing the cut at the hook site.
            {
                void* returnAddress = reinterpret_cast<void*>(
                    config.hookAddress + static_cast<uint32_t>(config.originalBytes.size())
                );
                EmitByte(code, 0xE9);  // JMP rel32
                uint32_t returnOffset = CalculateRelativeOffset(code - 1, returnAddress);
                EmitDword(code, returnOffset);

                if (config.skipOriginalBytes) {
                    char debugMsg[256];
                    snprintf(debugMsg, sizeof(debugMsg), "[Wrapper] Skipping original bytes, jumping directly to 0x%08X\n",
                        reinterpret_cast<uint32_t>(returnAddress));
                    Platform::Log(debugMsg);
                }
            }

            // The stub was written through a writable mapping, so it only becomes
            // code once it is sealed. ProtectExec flushes the instruction cache.
            if (!Platform::ProtectExec(wrapperMem, estimatedSize)) {
                Platform::Log("[Wrapper] Failed to make the wrapper executable\n");
                return nullptr;
            }

            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg), "[Wrapper] Generated DETOUR wrapper at 0x%08X (%d bytes)\n",
                reinterpret_cast<uint32_t>(wrapperMem), static_cast<int>(code - wrapperMem));
            Platform::Log(debugMsg);

            return wrapperMem;
        }

        // ===== Parameter Extraction =====

        void WrapperGenerator_x86::ExtractAndPushParameter(uint8_t*& code, const ParameterInfo& param, int savedStateSize) {
            // Stack layout constants (relative to EBX, which points to saved state)
            // EBX points to where ESP was after PUSHAD/PUSHFD
            //
            // Saved state structure at [EBX]:
            const int OFFSET_EDI    = 4;   // [EBX+4]  = EDI
            const int OFFSET_ESI    = 8;   // [EBX+8]  = ESI
            const int OFFSET_EBP    = 12;  // [EBX+12] = EBP
            const int OFFSET_EBX    = 20;  // [EBX+20] = EBX
            const int OFFSET_EDX    = 24;  // [EBX+24] = EDX
            const int OFFSET_ECX    = 28;  // [EBX+28] = ECX
            const int OFFSET_EAX    = 32;  // [EBX+32] = EAX

            // Original stack data (parameters, etc.) is at:
            // [EBX + savedStateSize + kFpAreaSize]
            // The FXSAVE area sits between the pushed state and the game's own stack.
            // The hook site is reached by a JMP, not a CALL, so there is no return
            // address in between
            const int STACK_OFFSET_TO_ORIGINAL_DATA = savedStateSize + kFpAreaSize;

            std::string source = param.source;

            // Convert to lowercase for comparison
            std::transform(source.begin(), source.end(), source.begin(), ::tolower);

            // We use ECX as our temp register for reading values
            // ECX is caller-saved in __cdecl, so it's safe to clobber

            // Check if source is a register (read from saved state)
            if (source == "eax") {
                // MOV ECX, [EBX + OFFSET_EAX]
                EmitByte(code, 0x8B);  // MOV r32, r/m32
                EmitByte(code, 0x4B);  // ModRM: ECX, [EBX + disp8]
                EmitByte(code, OFFSET_EAX);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ebx") {
                // MOV ECX, [EBX + OFFSET_EBX]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_EBX);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ecx") {
                // MOV ECX, [EBX + OFFSET_ECX]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_ECX);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "edx") {
                // MOV ECX, [EBX + OFFSET_EDX]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_EDX);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "esi") {
                // MOV ECX, [EBX + OFFSET_ESI]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_ESI);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "edi") {
                // MOV ECX, [EBX + OFFSET_EDI]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_EDI);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ebp") {
                // MOV ECX, [EBX + OFFSET_EBP]
                EmitByte(code, 0x8B);
                EmitByte(code, 0x4B);
                EmitByte(code, OFFSET_EBP);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            // Check if source is a stack offset like "esp+0", "esp+4", etc.
            else if (source.find("esp+") == 0 || source.find("esp-") == 0) {
                // Parse the user-specified offset from the parameter source
                int userOffset = 0;
                try {
                    userOffset = std::stoi(source.substr(4));
                } catch (...) {
                    Platform::Log(("[Wrapper] Invalid stack offset: " + source + "\n").c_str());
                    return;
                }

                // Calculate the actual offset from EBX, not ESP
                // ESP has moved by the alignment, its pad, and every push before
                // this one. EBX still points at the saved state, so:
                // 1. The saved state size (PUSHAD + PUSHFD)
                // 2. The user's requested offset
                int actualOffset = STACK_OFFSET_TO_ORIGINAL_DATA + userOffset;

                // Generate LEA ECX, [EBX + actualOffset]
                if (actualOffset == 0) {
                    // LEA ECX, [EBX]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x0B);  // ModRM: ECX, [EBX]
                } else if (actualOffset >= -128 && actualOffset <= 127) {
                    // LEA ECX, [EBX + imm8]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x4B);  // ModRM: ECX, [EBX + disp8]
                    EmitByte(code, static_cast<uint8_t>(actualOffset));
                } else {
                    // LEA ECX, [EBX + imm32]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x8B);  // ModRM: ECX, [EBX + disp32]
                    EmitDword(code, actualOffset);
                }
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else {
                Platform::Log(("[Wrapper] Unsupported parameter source: " + source + "\n").c_str());
            }
        }

        // ===== Factory Function =====

        // Global instance
        static WrapperGenerator_x86 g_wrapperGenerator;

        WrapperGeneratorBase* GetWrapperGenerator() {
            return &g_wrapperGenerator;
        }

    } // namespace Wrappers
} // namespace KotorPatcher
