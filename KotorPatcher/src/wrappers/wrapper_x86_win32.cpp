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
            switch (config.type) {
            case WrapperConfig::HookType::INLINE:
                return GenerateInlineWrapper(config);
            case WrapperConfig::HookType::REPLACE:
                return GenerateReplaceWrapper(config);
            case WrapperConfig::HookType::WRAP:
                return GenerateWrapWrapper(config);
            default:
                OutputDebugStringA("[Wrapper] Unknown hook type\n");
                return nullptr;
            }
        }

        void* WrapperGenerator_x86_Win32::GenerateInlineWrapper(const WrapperConfig& config) {
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

            // ===== CALCULATE ORIGINAL ESP =====
            // The patch function expects to see the original stack layout
            // We need to calculate where ESP was before our wrapper modified it

            int totalPushed = 0;
            if (config.preserveFlags) totalPushed += 4;
            if (config.preserveRegisters) totalPushed += 32;
            // +4 for the return address that was pushed when game called the hook

            // Save current ESP to EBX (we'll restore it later)
            // MOV EBX, ESP
            EmitByte(code, 0x89);  // MOV r/m32, r32
            EmitByte(code, 0xE3);  // ModRM: EBX = ESP

            // Restore ESP to original value before wrapper
            // ADD ESP, totalPushed
            if (totalPushed <= 127) {
                EmitByte(code, 0x83);  // ADD ESP, imm8
                EmitByte(code, 0xC4);
                EmitByte(code, static_cast<BYTE>(totalPushed));
            } else {
                EmitByte(code, 0x81);  // ADD ESP, imm32
                EmitByte(code, 0xC4);
                EmitDword(code, totalPushed);
            }

            // ===== EXTRACT AND PUSH PARAMETERS =====
            // If the hook has parameters defined, extract them and push onto stack
            // Parameters are pushed in reverse order for __cdecl (right-to-left)

            if (!config.parameters.empty()) {
                // Push parameters in reverse order (last parameter first)
                for (int i = static_cast<int>(config.parameters.size()) - 1; i >= 0; i--) {
                    const auto& param = config.parameters[i];
                    ExtractAndPushParameter(code, param, totalPushed);
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

            // Execute the stolen bytes (original instructions we overwrote)
            // These were specified in the patch config to align with instruction boundaries
            if (config.stolenBytes.empty()) {
                OutputDebugStringA("[Wrapper] ERROR: No stolen bytes provided for INLINE hook\n");
                return nullptr;
            }

            // Emit the stolen bytes to execute the original instructions
            EmitBytes(code, config.stolenBytes.data(), config.stolenBytes.size());

            // Jump back to hookAddress + stolen_bytes_size to continue normal execution
            void* returnAddress = reinterpret_cast<void*>(
                config.hookAddress + static_cast<DWORD>(config.stolenBytes.size())
            );
            EmitByte(code, 0xE9);  // JMP rel32
            DWORD returnOffset = CalculateRelativeOffset(code - 1, returnAddress);
            EmitDword(code, returnOffset);

            // Flush instruction cache
            FlushInstructionCache(GetCurrentProcess(), wrapperMem, code - wrapperMem);

            char debugMsg[256];
            sprintf_s(debugMsg, "[Wrapper] Generated INLINE wrapper at 0x%08X (%d bytes)\n",
                reinterpret_cast<DWORD>(wrapperMem), static_cast<int>(code - wrapperMem));
            OutputDebugStringA(debugMsg);

            return wrapperMem;
        }

        void* WrapperGenerator_x86_Win32::GenerateReplaceWrapper(const WrapperConfig& config) {
            // REPLACE type doesn't need a wrapper - just JMP directly to patch
            // This is the old behavior for compatibility

            // However, we still allocate a tiny stub for consistency
            // Just in case we want to add logging or other features later

            BYTE* wrapperMem = static_cast<BYTE*>(AllocateExecutableMemory(16));
            if (!wrapperMem) return nullptr;

            BYTE* code = wrapperMem;

            // JMP to patch function
            EmitByte(code, 0xE9);  // JMP rel32
            // Note: code now points one byte AFTER the 0xE9 opcode
            DWORD jmpOffset = CalculateRelativeOffset(code - 1, config.patchFunction);
            EmitDword(code, jmpOffset);

            FlushInstructionCache(GetCurrentProcess(), wrapperMem, code - wrapperMem);

            char debugMsg[128];
            sprintf_s(debugMsg, "[Wrapper] Generated REPLACE wrapper (direct JMP) at 0x%08X\n",
                reinterpret_cast<DWORD>(wrapperMem));
            OutputDebugStringA(debugMsg);

            return wrapperMem;
        }

        void* WrapperGenerator_x86_Win32::GenerateWrapWrapper(const WrapperConfig& config) {
            // WRAP type: Call patch, then call original function
            // This will be fully implemented with detour trampolines in Phase 2

            // For now, generate a simple version that calls patch then returns
            BYTE* wrapperMem = static_cast<BYTE*>(AllocateExecutableMemory(64));
            if (!wrapperMem) return nullptr;

            BYTE* code = wrapperMem;

            // Save registers if requested
            if (config.preserveRegisters) {
                EmitByte(code, 0x60);  // PUSHAD
            }
            if (config.preserveFlags) {
                EmitByte(code, 0x9C);  // PUSHFD
            }

            // CALL patch function
            EmitByte(code, 0xE8);  // CALL rel32
            // Note: code now points one byte AFTER the 0xE8 opcode
            DWORD callOffset = CalculateRelativeOffset(code - 1, config.patchFunction);
            EmitDword(code, callOffset);

            // Restore state
            if (config.preserveFlags) {
                EmitByte(code, 0x9D);  // POPFD
            }
            if (config.preserveRegisters) {
                EmitByte(code, 0x61);  // POPAD
            }

            // TODO: Call original function when detours implemented
            // For now, just return
            EmitByte(code, 0xC3);  // RET

            FlushInstructionCache(GetCurrentProcess(), wrapperMem, code - wrapperMem);

            OutputDebugStringA("[Wrapper] Generated WRAP wrapper (partial implementation)\n");

            return wrapperMem;
        }

        // ===== Parameter Extraction =====

        void WrapperGenerator_x86_Win32::ExtractAndPushParameter(BYTE*& code, const ParameterInfo& param, int savedStateOffset) {
            std::string source = param.source;

            // Convert to lowercase for comparison
            std::transform(source.begin(), source.end(), source.begin(), ::tolower);

            // ECX is our temp register for reading values
            // We use ECX because it's caller-saved and won't affect the hook function

            // Check if source is a register
            if (source == "eax") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+48]
                EmitByte(code, 0x4B);
                EmitByte(code, 48);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ebx") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+36]
                EmitByte(code, 0x4B);
                EmitByte(code, 36);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ecx") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+44]
                EmitByte(code, 0x4B);
                EmitByte(code, 44);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "edx") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+40]
                EmitByte(code, 0x4B);
                EmitByte(code, 40);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "esi") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+24]
                EmitByte(code, 0x4B);
                EmitByte(code, 24);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "edi") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+20]
                EmitByte(code, 0x4B);
                EmitByte(code, 20);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            else if (source == "ebp") {
                EmitByte(code, 0x8B);  // MOV ECX, [EBX+28]
                EmitByte(code, 0x4B);
                EmitByte(code, 28);
                EmitByte(code, 0x51);  // PUSH ECX
            }
            // Check if source is a stack offset like "esp+0", "esp+4", etc.
            else if (source.find("esp+") == 0 || source.find("esp-") == 0) {
                // Parse the offset
                int offset = 0;
                try {
                    offset = std::stoi(source.substr(4));
                } catch (...) {
                    OutputDebugStringA(("[Wrapper] Invalid stack offset: " + source + "\n").c_str());
                    return;
                }

                // Read from [ESP + offset]
                // Remember: ESP was restored to original value before this code runs
                if (offset == 0) {
                    // MOV ECX, [ESP]
                    EmitByte(code, 0x8B);
                    EmitByte(code, 0x0C);
                    EmitByte(code, 0x24);
                } else if (offset >= -128 && offset <= 127) {
                    // MOV ECX, [ESP + imm8]
                    EmitByte(code, 0x8B);
                    EmitByte(code, 0x4C);
                    EmitByte(code, 0x24);
                    EmitByte(code, static_cast<BYTE>(offset));
                } else {
                    // MOV ECX, [ESP + imm32]
                    EmitByte(code, 0x8B);
                    EmitByte(code, 0x8C);
                    EmitByte(code, 0x24);
                    EmitDword(code, offset);
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
