#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "wrapper_context.h"

// Abstract interface for platform-specific wrapper generators
// Allows supporting multiple platforms/architectures

namespace KotorPatcher {
    // Forward declare ParameterInfo to avoid circular dependency
    struct ParameterInfo;

    namespace Wrappers {

        // Configuration for a wrapper stub
        struct WrapperConfig {
            // Target patch function to call
            void* patchFunction;

            // Hook address in game code
            DWORD hookAddress;

            // Original bytes that were overwritten by the hook (for INLINE type)
            // These will be executed in the wrapper before returning to original code
            // Must be >= 5 bytes and align with instruction boundaries
            std::vector<BYTE> stolenBytes;

            // Hook type determines wrapper behavior
            enum class HookType {
                INLINE,     // Save state, call patch, restore state, continue original
                REPLACE,    // Jump to patch, patch handles everything (no wrapper)
                WRAP        // Call patch, then execute original function (future: detours)
            };
            HookType type = HookType::INLINE;

            // State preservation options
            bool preserveRegisters = true;
            bool preserveFlags = true;

            // Registers to exclude from restoration
            // Allows patch to modify specific registers
            std::vector<std::string> excludeFromRestore;

            // Parameters to extract and pass to hook function (for INLINE hooks)
            std::vector<ParameterInfo> parameters;

            // Original function pointer (for WRAP type with detours)
            void* originalFunction = nullptr;

            // Helper: Check if a register should be restored
            bool ShouldRestoreRegister(const std::string& regName) const {
                if (!preserveRegisters) return false;

                for (const auto& excluded : excludeFromRestore) {
                    if (_stricmp(excluded.c_str(), regName.c_str()) == 0) {
                        return false;
                    }
                }
                return true;
            }
        };

        // Abstract base class for wrapper generators
        class WrapperGeneratorBase {
        public:
            virtual ~WrapperGeneratorBase() = default;

            // Generate a wrapper stub and return its address
            // Returns nullptr on failure
            virtual void* GenerateWrapper(const WrapperConfig& config) = 0;

            // Free all allocated wrappers
            virtual void FreeAllWrappers() = 0;

            // Get platform name for debugging
            virtual const char* GetPlatformName() const = 0;
        };

        // Factory function to get the appropriate wrapper generator for current platform
        WrapperGeneratorBase* GetWrapperGenerator();

    } // namespace Wrappers
} // namespace KotorPatcher
