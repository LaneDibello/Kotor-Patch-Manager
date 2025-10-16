#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace KotorPatcher {
    // Initialization and cleanup
    bool InitializePatcher();
    void CleanupPatcher();

    // Configuration
    struct PatchInfo {
        std::string dllPath;
        std::string functionName;
        DWORD hookAddress;
        std::vector<BYTE> originalBytes;
    };

    bool LoadPatchConfig(const std::string& configPath);
    const std::vector<PatchInfo>& GetLoadedPatches();

    // Patch application
    bool ApplyPatches();
    bool ApplyPatch(const PatchInfo& patch);
}