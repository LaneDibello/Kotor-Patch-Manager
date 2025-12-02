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
            // Base: ~100 bytes, +10 per excluded register
            size_t estimatedSize = 128 + (config.excludeFromRestore.size() * 10);

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

                    for (int i = 0; i < 8; i++) {
                        if (config.ShouldRestoreRegister(regOrder[i])) {
                            EmitByte(code, popOpcodes[i]);  // POP reg
                        } else {
                            // Skip this register (pop to nowhere)
                            EmitByte(code, 0x83);  // ADD ESP, 4
                            EmitByte(code, 0xC4);
                            EmitByte(code, 0x04);
                        }
                    }
                }
            }

            // ===== RETURN TO ORIGINAL CODE =====

            if (config.skipOriginalBytes) {
                // Skip executing original bytes - jump directly back to continue execution
                // This is used when fully replacing behavior rather than augmenting it
                void* returnAddress = reinterpret_cast<void*>(
                    config.hookAddress + static_cast<DWORD>(config.originalBytes.size())
                );
                EmitByte(code, 0xE9);  // JMP rel32
                DWORD returnOffset = CalculateRelativeOffset(code - 1, returnAddress);
                EmitDword(code, returnOffset);

                char debugMsg[256];
                sprintf_s(debugMsg, "[Wrapper] Skipping original bytes, jumping directly to 0x%08X\n",
                    reinterpret_cast<DWORD>(returnAddress));
                OutputDebugStringA(debugMsg);
            } else {
                // Execute the original bytes (instructions we overwrote with JMP)
                // These were specified in the patch config to align with instruction boundaries
                if (config.originalBytes.empty()) {
                    OutputDebugStringA("[Wrapper] ERROR: No original bytes provided for DETOUR hook\n");
                    return nullptr;
                }

                // Emit the original bytes to execute the overwritten instructions
                EmitBytes(code, config.originalBytes.data(), config.originalBytes.size());

                // Jump back to hookAddress + original_bytes_size to continue normal execution
                void* returnAddress = reinterpret_cast<void*>(
                    config.hookAddress + static_cast<DWORD>(config.originalBytes.size())
                );
                EmitByte(code, 0xE9);  // JMP rel32
                DWORD returnOffset = CalculateRelativeOffset(code - 1, returnAddress);
                EmitDword(code, returnOffset);
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

                // Generate MOV ECX, [ESP + actualOffset]
                if (actualOffset == 0) {
                    // MOV ECX, [ESP]
                    EmitByte(code, 0x8B);  // MOV r32, r/m32
                    EmitByte(code, 0x0C);  // ModRM: ECX, [ESP]
                    EmitByte(code, 0x24);  // SIB: [ESP]
                } else if (actualOffset >= -128 && actualOffset <= 127) {
                    // MOV ECX, [ESP + imm8]
                    EmitByte(code, 0x8B);  // MOV r32, r/m32
                    EmitByte(code, 0x4C);  // ModRM: ECX, [ESP + disp8]
                    EmitByte(code, 0x24);  // SIB: [ESP]
                    EmitByte(code, static_cast<BYTE>(actualOffset));
                } else {
                    // MOV ECX, [ESP + imm32]
                    EmitByte(code, 0x8B);  // MOV r32, r/m32
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
