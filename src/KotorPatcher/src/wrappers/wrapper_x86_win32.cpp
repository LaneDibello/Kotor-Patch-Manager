#include "wrapper_x86_win32.h"
#include "patcher.h"
#include <cstring>
#include <algorithm>

namespace KotorPatcher {
    namespace Wrappers {

        WrapperGenerator_x86_Win32::WrapperGenerator_x86_Win32() {
        }

        WrapperGenerator_x86_Win32::~WrapperGenerator_x86_Win32() {
            FreeAllWrappers();
        }

        void* WrapperGenerator_x86_Win32::AllocateExecutableMemory(size_t size) {
            void* mem = VirtualAlloc(
                nullptr,
                size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            );

            if (mem) {
                m_allocatedWrappers.push_back({ mem, size });
            }

            return mem;
        }

        void WrapperGenerator_x86_Win32::FreeAllWrappers() {
            for (const auto& wrapper : m_allocatedWrappers) {
                VirtualFree(wrapper.address, 0, MEM_RELEASE);
            }
            m_allocatedWrappers.clear();
        }

        void WrapperGenerator_x86_Win32::EmitBytes(BYTE*& code, const BYTE* bytes, size_t count) {
            memcpy(code, bytes, count);
            code += count;
        }

        void WrapperGenerator_x86_Win32::EmitByte(BYTE*& code, BYTE value) {
            *code++ = value;
        }

        void WrapperGenerator_x86_Win32::EmitDword(BYTE*& code, DWORD value) {
            *reinterpret_cast<DWORD*>(code) = value;
            code += 4;
        }

        DWORD WrapperGenerator_x86_Win32::CalculateRelativeOffset(void* from, void* to) {
            // For relative JMP/CALL: offset = target - (source + 5)
            // The +5 accounts for the instruction size (1 byte opcode + 4 byte offset)
            return reinterpret_cast<DWORD>(to) - (reinterpret_cast<DWORD>(from) + 5);
        }

        void* WrapperGenerator_x86_Win32::GenerateWrapper(const WrapperConfig& config) {
            return GenerateDetourWrapper(config);
        }

        void* WrapperGenerator_x86_Win32::GenerateDetourWrapper(const WrapperConfig& config) {
            // Estimate wrapper size
            // Base: ~100 bytes, +10 per excluded register, +10 for consumed-exit conditional jump
            size_t estimatedSize = 128 + (config.excludeFromRestore.size() * 10);
            if (config.consumedExitAddress != 0) {
                estimatedSize += 16;
            }

            BYTE* wrapperMem = static_cast<BYTE*>(AllocateExecutableMemory(estimatedSize));
            if (!wrapperMem) {
                OutputDebugStringA("[Wrapper] Failed to allocate wrapper memory\n");
                return nullptr;
            }

            BYTE* code = wrapperMem;  // Current write position

            // ===== PROLOGUE: Save CPU State =====

            if (config.preserveRegisters) {
                // PUSHAD: Push all general-purpose registers
                // Order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
                EmitByte(code, 0x60);  // PUSHAD
            }

            if (config.preserveFlags) {
                // PUSHFD: Push EFLAGS register
                EmitByte(code, 0x9C);  // PUSHFD
            }

            // ===== CALCULATE STACK LAYOUT =====
            // At this point, the stack layout is:
            // [ESP+0]  = EFLAGS (if preserveFlags)
            // [ESP+4]  = EDI    \
            // [ESP+8]  = ESI     |
            // [ESP+12] = EBP     |
            // [ESP+16] = (ESP)   | PUSHAD saved these (32 bytes total)
            // [ESP+20] = EBX     |
            // [ESP+24] = EDX     |
            // [ESP+28] = ECX     |
            // [ESP+32] = EAX    /
            // [ESP+36] = Return address (from game's CALL to hook)
            // [ESP+40] = Original stack data (parameters, etc.)

            // Calculate total bytes we've pushed onto the stack
            int savedStateSize = 0;
            if (config.preserveFlags) savedStateSize += 4;      // PUSHFD
            if (config.preserveRegisters) savedStateSize += 32; // PUSHAD

            // Save current ESP to EBX (points to our saved state)
            // We'll use this to read saved registers and restore ESP later
            // MOV EBX, ESP
            EmitByte(code, 0x89);  // MOV r/m32, r32
            EmitByte(code, 0xE3);  // ModRM: EBX = ESP

            // IMPORTANT: We do NOT modify ESP here!
            // If we did, PUSH instructions would overwrite our saved registers.
            // Instead, we'll read parameters with adjusted offsets (see ExtractAndPushParameter)

            // ===== EXTRACT AND PUSH PARAMETERS =====
            // If the hook has parameters defined, extract them and push onto stack
            // Parameters are pushed in reverse order for __cdecl (right-to-left)

            if (!config.parameters.empty()) {
                // Push parameters in reverse order (last parameter first)
                int pushCount = 0;
                for (int i = static_cast<int>(config.parameters.size()) - 1; i >= 0; i--) {
                    const auto& param = config.parameters[i];
                    ExtractAndPushParameter(code, param, savedStateSize, pushCount);
                    pushCount++;
                }
            }

            // ===== CALL PATCH FUNCTION =====
            // Now ESP points to the original stack layout (if no params)
            // Or has parameters pushed (if params specified)

            // CALL patch_function
            EmitByte(code, 0xE8);  // CALL rel32
            // Note: code now points one byte AFTER the 0xE8 opcode
            // CalculateRelativeOffset needs the address of the opcode itself
            DWORD callOffset = CalculateRelativeOffset(code - 1, config.patchFunction);
            EmitDword(code, callOffset);

            // ===== CLEAN UP PARAMETERS =====
            // For __cdecl, caller cleans up the stack
            if (!config.parameters.empty()) {
                int paramBytes = static_cast<int>(config.parameters.size()) * 4;
                if (paramBytes <= 127) {
                    EmitByte(code, 0x83);  // ADD ESP, imm8
                    EmitByte(code, 0xC4);
                    EmitByte(code, static_cast<BYTE>(paramBytes));
                } else {
                    EmitByte(code, 0x81);  // ADD ESP, imm32
                    EmitByte(code, 0xC4);
                    EmitDword(code, paramBytes);
                }
            }

            // ===== RESTORE WRAPPER ESP =====
            // Restore ESP back to point to our saved state
            // MOV ESP, EBX
            EmitByte(code, 0x89);  // MOV r/m32, r32
            EmitByte(code, 0xDC);  // ModRM: ESP = EBX

            // ===== EPILOGUE: Restore CPU State =====

            if (config.preserveFlags) {
                // POPFD: Restore EFLAGS
                EmitByte(code, 0x9D);  // POPFD
            }

            if (config.preserveRegisters) {
                // Handle register exclusions
                if (config.excludeFromRestore.empty()) {
                    // Simple case: restore all registers
                    EmitByte(code, 0x61);  // POPAD
                } else {
                    // Complex case: selectively restore registers
                    // POPAD pops in order: EDI, ESI, EBP, (ESP), EBX, EDX, ECX, EAX

                    // We need to manually pop each register
                    const char* regOrder[] = { "edi", "esi", "ebp", "esp", "ebx", "edx", "ecx", "eax" };
                    const BYTE popOpcodes[] = { 0x5F, 0x5E, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58 };
                    constexpr int kEspSlot = 3;

                    for (int i = 0; i < 8; i++) {
                        // Hardware POPAD never restores ESP — it just advances
                        // past the saved-ESP slot. Emitting `POP ESP` (0x5C)
                        // here would write ESP from the saved value, jumping
                        // past the remaining EBX/EDX/ECX/EAX slots and reading
                        // them from random stack memory. Always skip the ESP
                        // slot regardless of exclude_from_restore.
                        if (i != kEspSlot && config.ShouldRestoreRegister(regOrder[i])) {
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

            // ===== EXECUTE STOLEN ORIGINAL BYTES =====
            // Run the original instructions BEFORE the conditional consumed-exit
            // jump so both paths leave the stack and registers in the same state
            // — equivalent to the cut bytes having executed natively in-place.
            // The consumed_exit_address contract assumes this state, since the
            // caller chose the target by inspecting where execution would
            // naturally land *after* the cut.
            if (!config.skipOriginalBytes) {
                if (config.originalBytes.empty()) {
                    OutputDebugStringA("[Wrapper] ERROR: No original bytes provided for DETOUR hook\n");
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
                DWORD consumedOffset = CalculateRelativeOffset(
                    code - 1,
                    reinterpret_cast<void*>(config.consumedExitAddress));
                EmitDword(code, consumedOffset);
                EmitByte(code, 0x9D);  // POPFD — restore EFLAGS for fall-through

                char debugMsg[256];
                sprintf_s(debugMsg, "[Wrapper] Conditional consumed-exit -> 0x%08X emitted\n",
                    config.consumedExitAddress);
                OutputDebugStringA(debugMsg);
            }

            // ===== JUMP BACK TO NATURAL FALL-THROUGH =====
            // hookAddress + originalBytes.size() is the resume point for the
            // non-consumed path. With cut bytes already emitted above, the
            // stack/register state at the target matches what the engine would
            // see after natively executing the cut at the hook site.
            {
                void* returnAddress = reinterpret_cast<void*>(
                    config.hookAddress + static_cast<DWORD>(config.originalBytes.size())
                );
                EmitByte(code, 0xE9);  // JMP rel32
                DWORD returnOffset = CalculateRelativeOffset(code - 1, returnAddress);
                EmitDword(code, returnOffset);

                if (config.skipOriginalBytes) {
                    char debugMsg[256];
                    sprintf_s(debugMsg, "[Wrapper] Skipping original bytes, jumping directly to 0x%08X\n",
                        reinterpret_cast<DWORD>(returnAddress));
                    OutputDebugStringA(debugMsg);
                }
            }

            // Flush instruction cache
            FlushInstructionCache(GetCurrentProcess(), wrapperMem, code - wrapperMem);

            char debugMsg[256];
            sprintf_s(debugMsg, "[Wrapper] Generated DETOUR wrapper at 0x%08X (%d bytes)\n",
                reinterpret_cast<DWORD>(wrapperMem), static_cast<int>(code - wrapperMem));
            OutputDebugStringA(debugMsg);

            return wrapperMem;
        }

        // ===== Parameter Extraction =====

        void WrapperGenerator_x86_Win32::ExtractAndPushParameter(BYTE*& code, const ParameterInfo& param, int savedStateSize, int pushCount = 0) {
            // Stack layout constants (relative to EBX, which points to saved state)
            // EBX points to where ESP was after PUSHAD/PUSHFD
            //
            // Saved state structure at [EBX]:
            const int OFFSET_EFLAGS = 0;   // [EBX+0]  = EFLAGS (if preserved)
            const int OFFSET_EDI    = 4;   // [EBX+4]  = EDI
            const int OFFSET_ESI    = 8;   // [EBX+8]  = ESI
            const int OFFSET_EBP    = 12;  // [EBX+12] = EBP
            const int OFFSET_ESP    = 16;  // [EBX+16] = (original ESP value saved by PUSHAD)
            const int OFFSET_EBX    = 20;  // [EBX+20] = EBX
            const int OFFSET_EDX    = 24;  // [EBX+24] = EDX
            const int OFFSET_ECX    = 28;  // [EBX+28] = ECX
            const int OFFSET_EAX    = 32;  // [EBX+32] = EAX

            // Original stack data (parameters, return address, etc.) is at:
            // [ESP + savedStateSize + 4]
            // The +4 accounts for the return address pushed by the game's CALL instruction
            const int STACK_OFFSET_TO_ORIGINAL_DATA = savedStateSize;

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
                    OutputDebugStringA(("[Wrapper] Invalid stack offset: " + source + "\n").c_str());
                    return;
                }

                // Calculate the actual offset from current ESP
                // ESP still points to saved state, so we need to account for:
                // 1. The saved state size (PUSHAD + PUSHFD)
                // 2. The return address (+4)
                // 3. The user's requested offset
                // 4. 4 times the number of push instructions before this one
                int actualOffset = STACK_OFFSET_TO_ORIGINAL_DATA + userOffset + pushCount * 4;

                // Generate LEA ECX, [ESP + actualOffset]
                if (actualOffset == 0) {
                    // LEA ECX, [ESP]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x0C);  // ModRM: ECX, [ESP]
                    EmitByte(code, 0x24);  // SIB: [ESP]
                } else if (actualOffset >= -128 && actualOffset <= 127) {
                    // LEA ECX, [ESP + imm8]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x4C);  // ModRM: ECX, [ESP + disp8]
                    EmitByte(code, 0x24);  // SIB: [ESP]
                    EmitByte(code, static_cast<BYTE>(actualOffset));
                } else {
                    // LEA ECX, [ESP + imm32]
                    EmitByte(code, 0x8D);  // LEA r32, m
                    EmitByte(code, 0x8C);  // ModRM: ECX, [ESP + disp32]
                    EmitByte(code, 0x24);  // SIB: [ESP]
                    EmitDword(code, actualOffset);
                }
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else {
                OutputDebugStringA(("[Wrapper] Unsupported parameter source: " + source + "\n").c_str());
            }
        }

        // ===== Factory Function =====

        // Global instance
        static WrapperGenerator_x86_Win32 g_wrapperGenerator;

        WrapperGeneratorBase* GetWrapperGenerator() {
            return &g_wrapperGenerator;
        }

    } // namespace Wrappers
} // namespace KotorPatcher
