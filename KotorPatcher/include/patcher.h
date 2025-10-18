#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace KotorPatcher {
    // Initialization and cleanup
    bool InitializePatcher();
    void CleanupPatcher();

    // Hook type determines how the patch is applied
    enum class HookType {
        INLINE,     // Default: Save state, call patch with context, restore state (safest)
        REPLACE,    // Legacy: Direct JMP to patch, patch handles everything (no wrapper)
        WRAP        // Future: Call patch, then execute original code (requires detours)
    };

    // Convert string to HookType
    HookType ParseHookType(const std::string& typeStr);

    // Parameter type for hook function parameters
    enum class ParameterType {
        INT,        // 32-bit integer
        UINT,       // Unsigned 32-bit integer
        POINTER,    // 32-bit pointer
        FLOAT,      // 32-bit float
        BYTE,       // 8-bit value
        SHORT       // 16-bit value
    };

    // Parameter source location
    struct ParameterInfo {
        std::string source;     // e.g., "eax", "esp+0", "[esp+4]"
        ParameterType type;     // Data type of the parameter
    };

    // Configuration for a single hook point
    struct PatchInfo {
        // Basic patch information
        std::string dllPath;           // Path to patch DLL
        std::string functionName;      // Exported function name in DLL
        DWORD hookAddress;             // Address in game code to hook
        std::vector<BYTE> originalBytes;  // Expected bytes for verification
        std::vector<BYTE> stolenBytes;    // Original bytes to execute in wrapper (for INLINE)
                                           // Must be >= 5 bytes and align with instruction boundaries

        // Hook behavior configuration
        HookType type = HookType::INLINE;  // Default to safest option

        // State preservation options (for INLINE and WRAP types)
        bool preserveRegisters = true;     // Auto-save/restore all registers
        bool preserveFlags = true;         // Auto-save/restore EFLAGS

        // Registers to exclude from restoration
        // Allows patches to modify specific registers (e.g., "eax", "edx")
        std::vector<std::string> excludeFromRestore;

        // Parameters to extract and pass to hook function (for INLINE hooks)
        std::vector<ParameterInfo> parameters;

        // Original function pointer (future: for detour trampolines)
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

    bool LoadPatchConfig(const std::string& configPath);
    const std::vector<PatchInfo>& GetLoadedPatches();

    // Patch application
    bool ApplyPatches();
    bool ApplyPatch(const PatchInfo& patch);
}