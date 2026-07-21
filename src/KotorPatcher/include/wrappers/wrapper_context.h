#pragma once
#include <cstdint>
#include "platform.h"

// Platform-independent context structure passed to patch functions
// Contains saved CPU state that patches can inspect and modify

namespace KotorPatcher {
    namespace Wrappers {

        // x86 32-bit CPU context
        // Layout matches PUSHAD instruction order for efficient assembly generation
        struct PatchContext_x86 {
            // General-purpose registers (in PUSHAD order)
            uint32_t edi;
            uint32_t esi;
            uint32_t ebp;
            uint32_t esp_at_pushad;  // ESP value at time of PUSHAD
            uint32_t ebx;
            uint32_t edx;
            uint32_t ecx;
            uint32_t eax;

            // Flags register
            uint32_t eflags;

            // Original stack pointer (before wrapper modified it)
            uint32_t original_esp;

            // Return address (where game called the hooked function)
            uint32_t return_address;

            // Pointer to original function (for detour trampolines)
            // Will be nullptr for simple hooks
            void* original_function;

            // ===== Helper Methods =====

            // Get function parameter by index (0-based)
            // Assumes __stdcall or __cdecl convention
            uint32_t GetParameter(int index) const {
                // Parameters are above return address on stack
                // Stack layout: [...params...][return_addr][saved_state]
                const uint32_t* stack = reinterpret_cast<const uint32_t*>(original_esp);
                return stack[index + 1];  // +1 to skip return address
            }

            // Set function return value (modifies EAX)
            void SetReturnValue(uint32_t value) {
                eax = value;
            }

            // Get current return value
            uint32_t GetReturnValue() const {
                return eax;
            }

            // Set specific register values
            void SetRegister(const char* name, uint32_t value) {
                if (StrICmp(name, "eax") == 0) eax = value;
                else if (StrICmp(name, "ebx") == 0) ebx = value;
                else if (StrICmp(name, "ecx") == 0) ecx = value;
                else if (StrICmp(name, "edx") == 0) edx = value;
                else if (StrICmp(name, "esi") == 0) esi = value;
                else if (StrICmp(name, "edi") == 0) edi = value;
                else if (StrICmp(name, "ebp") == 0) ebp = value;
                else if (StrICmp(name, "esp") == 0) esp_at_pushad = value;
            }

            // Get specific register value
            uint32_t GetRegister(const char* name) const {
                if (StrICmp(name, "eax") == 0) return eax;
                else if (StrICmp(name, "ebx") == 0) return ebx;
                else if (StrICmp(name, "ecx") == 0) return ecx;
                else if (StrICmp(name, "edx") == 0) return edx;
                else if (StrICmp(name, "esi") == 0) return esi;
                else if (StrICmp(name, "edi") == 0) return edi;
                else if (StrICmp(name, "ebp") == 0) return ebp;
                else if (StrICmp(name, "esp") == 0) return esp_at_pushad;
                return 0;
            }

            // Check if a CPU flag is set
            bool IsFlagSet(uint32_t flag_mask) const {
                return (eflags & flag_mask) != 0;
            }

            // Set/clear a CPU flag
            void SetFlag(uint32_t flag_mask, bool value) {
                if (value) {
                    eflags |= flag_mask;
                } else {
                    eflags &= ~flag_mask;
                }
            }
        };

        // Common EFLAGS bit masks
        constexpr uint32_t FLAG_CARRY      = 0x0001;
        constexpr uint32_t FLAG_PARITY     = 0x0004;
        constexpr uint32_t FLAG_ADJUST     = 0x0010;
        constexpr uint32_t FLAG_ZERO       = 0x0040;
        constexpr uint32_t FLAG_SIGN       = 0x0080;
        constexpr uint32_t FLAG_DIRECTION  = 0x0400;
        constexpr uint32_t FLAG_OVERFLOW   = 0x0800;

        // Type alias for current platform
        // On x86 Windows, this is PatchContext_x86
        // Future: Add conditional compilation for other platforms
        using PatchContext = PatchContext_x86;

        // Function signature that all patches must follow
        // Patch receives context pointer and can modify it
        using PatchFunction = void(*)(PatchContext*);

    } // namespace Wrappers
} // namespace KotorPatcher
