#include "wrapper_x86_win32.h"
#include <cstring>

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

            // ===== BUILD CONTEXT STRUCTURE =====

            // At this point, stack layout is:
            // [ESP+0] = EFLAGS (if preserved)
            // [ESP+4] = EDI (from PUSHAD)
            // [ESP+8] = ESI
            // ... etc

            // Push additional context fields

            // Push original_function pointer
            EmitByte(code, 0x68);  // PUSH imm32
            EmitDword(code, reinterpret_cast<DWORD>(config.originalFunction));

            // Push return_address (caller's return address)
            // It's above our saved state on the stack
            // Calculate offset based on what we pushed
            int returnAddrOffset = 0;
            if (config.preserveFlags) returnAddrOffset += 4;
            if (config.preserveRegisters) returnAddrOffset += 32;  // PUSHAD saves 8 regs * 4 bytes

            EmitByte(code, 0xFF);  // PUSH [ESP + offset]
            EmitByte(code, 0xB4);
            EmitByte(code, 0x24);
            EmitDword(code, returnAddrOffset + 4);  // +4 for the original_function we just pushed

            // Push original_esp
            // ESP before we modified it = current ESP + all our pushes + 4 (for return addr)
            int totalPushed = 8;  // original_function + return_address
            if (config.preserveFlags) totalPushed += 4;
            if (config.preserveRegisters) totalPushed += 32;

            EmitByte(code, 0x8D);  // LEA EAX, [ESP + offset]
            EmitByte(code, 0x84);
            EmitByte(code, 0x24);
            EmitDword(code, totalPushed + 4);  // +4 for return address
            EmitByte(code, 0x50);  // PUSH EAX

            // Now ESP points to complete PatchContext structure
            // Context layout matches PatchContext_x86 struct

            // ===== CALL PATCH FUNCTION =====

            // Pass context pointer as parameter
            EmitByte(code, 0x54);  // PUSH ESP (context pointer)

            // CALL patch_function
            EmitByte(code, 0xE8);  // CALL rel32
            // Note: code now points one byte AFTER the 0xE8 opcode
            // CalculateRelativeOffset needs the address of the opcode itself
            DWORD callOffset = CalculateRelativeOffset(code - 1, config.patchFunction);
            EmitDword(code, callOffset);

            // Clean up parameter (4 bytes)
            EmitByte(code, 0x83);  // ADD ESP, 4
            EmitByte(code, 0xC4);
            EmitByte(code, 0x04);

            // ===== RESTORE CONTEXT FIELDS =====

            // Pop original_esp (we don't need to restore this)
            EmitByte(code, 0x83);  // ADD ESP, 4
            EmitByte(code, 0xC4);
            EmitByte(code, 0x04);

            // Pop return_address (discard)
            EmitByte(code, 0x83);  // ADD ESP, 4
            EmitByte(code, 0xC4);
            EmitByte(code, 0x04);

            // Pop original_function (discard)
            EmitByte(code, 0x83);  // ADD ESP, 4
            EmitByte(code, 0xC4);
            EmitByte(code, 0x04);

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

            // For now, just return to caller
            // TODO: When detours are implemented, jump to continuation point
            EmitByte(code, 0xC3);  // RET

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

        // ===== Factory Function =====

        // Global instance
        static WrapperGenerator_x86_Win32 g_wrapperGenerator;

        WrapperGeneratorBase* GetWrapperGenerator() {
            return &g_wrapperGenerator;
        }

    } // namespace Wrappers
} // namespace KotorPatcher
