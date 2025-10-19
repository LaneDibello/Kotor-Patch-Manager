#pragma once
#include <windows.h>
#include <cstdint>

// Platform-independent context structure passed to patch functions
// Contains saved CPU state that patches can inspect and modify

namespace KotorPatcher {
    namespace Wrappers {

        // x86 32-bit CPU context
        // Layout matches PUSHAD instruction order for efficient assembly generation
        struct PatchContext_x86 {
            // General-purpose registers (in PUSHAD order)
            DWORD edi;
            DWORD esi;
            DWORD ebp;
            DWORD esp_at_pushad;  // ESP value at time of PUSHAD
            DWORD ebx;
            DWORD edx;
            DWORD ecx;
            DWORD eax;

            // Flags register
            DWORD eflags;

            // Original stack pointer (before wrapper modified it)
            DWORD original_esp;

            // Return address (where game called the hooked function)
            DWORD return_address;

            // Pointer to original function (for detour trampolines)
            // Will be nullptr for simple hooks
            void* original_function;

            // ===== Helper Methods =====

            // Get function parameter by index (0-based)
            // Assumes __stdcall or __cdecl convention
            DWORD GetParameter(int index) const {
                // Parameters are above return address on stack
                // Stack layout: [...params...][return_addr][saved_state]
                const DWORD* stack = reinterpret_cast<const DWORD*>(original_esp);
                return stack[index + 1];  // +1 to skip return address
            }

            // Set function return value (modifies EAX)
            void SetReturnValue(DWORD value) {
                eax = value;
            }

            // Get current return value
            DWORD GetReturnValue() const {
                return eax;
            }

            // Set specific register values
            void SetRegister(const char* name, DWORD value) {
                if (_stricmp(name, "eax") == 0) eax = value;
                else if (_stricmp(name, "ebx") == 0) ebx = value;
                else if (_stricmp(name, "ecx") == 0) ecx = value;
                else if (_stricmp(name, "edx") == 0) edx = value;
                else if (_stricmp(name, "esi") == 0) esi = value;
                else if (_stricmp(name, "edi") == 0) edi = value;
                else if (_stricmp(name, "ebp") == 0) ebp = value;
                else if (_stricmp(name, "esp") == 0) esp_at_pushad = value;
            }

            // Get specific register value
            DWORD GetRegister(const char* name) const {
                if (_stricmp(name, "eax") == 0) return eax;
                else if (_stricmp(name, "ebx") == 0) return ebx;
                else if (_stricmp(name, "ecx") == 0) return ecx;
                else if (_stricmp(name, "edx") == 0) return edx;
                else if (_stricmp(name, "esi") == 0) return esi;
                else if (_stricmp(name, "edi") == 0) return edi;
                else if (_stricmp(name, "ebp") == 0) return ebp;
                else if (_stricmp(name, "esp") == 0) return esp_at_pushad;
                return 0;
            }

            // Check if a CPU flag is set
            bool IsFlagSet(DWORD flag_mask) const {
                return (eflags & flag_mask) != 0;
            }

            // Set/clear a CPU flag
            void SetFlag(DWORD flag_mask, bool value) {
                if (value) {
                    eflags |= flag_mask;
                } else {
                    eflags &= ~flag_mask;
                }
            }
        };

        // Common EFLAGS bit masks
        constexpr DWORD FLAG_CARRY      = 0x0001;
        constexpr DWORD FLAG_PARITY     = 0x0004;
        constexpr DWORD FLAG_ADJUST     = 0x0010;
        constexpr DWORD FLAG_ZERO       = 0x0040;
        constexpr DWORD FLAG_SIGN       = 0x0080;
        constexpr DWORD FLAG_DIRECTION  = 0x0400;
        constexpr DWORD FLAG_OVERFLOW   = 0x0800;

        // Type alias for current platform
        // On x86 Windows, this is PatchContext_x86
        // Future: Add conditional compilation for other platforms
        using PatchContext = PatchContext_x86;

        // Function signature that all patches must follow
        // Patch receives context pointer and can modify it
        using PatchFunction = void(*)(PatchContext*);

    } // namespace Wrappers
} // namespace KotorPatcher
