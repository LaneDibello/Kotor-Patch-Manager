#include "patcher.h"
#include "config_reader.h"
#include "trampoline.h"
#include <fstream>

namespace KotorPatcher {
    static std::vector<HMODULE> g_loadedPatches;
    static std::vector<PatchInfo> g_patches;
    static bool g_initialized = false;

    bool InitializePatcher() {
        if (g_initialized) return true;

        // Get DLL directory
        char dllPath[MAX_PATH];
        HMODULE hModule = GetModuleHandleA("kotor_patcher.dll");
        if (!hModule || GetModuleFileNameA(hModule, dllPath, MAX_PATH) == 0) {
            return false;
        }

        std::string dllDir(dllPath);
        size_t lastSlash = dllDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dllDir = dllDir.substr(0, lastSlash);
        }

        // Load configuration
        std::string configPath = dllDir + "\\patch_config.toml";
        if (!Config::ParseConfig(configPath, g_patches)) {
            return false;
        }

        // Apply patches
        if (!ApplyPatches()) {
            return false;
        }

        g_initialized = true;
        return true;
    }

    void CleanupPatcher() {
        for (HMODULE h : g_loadedPatches) {
            if (h) FreeLibrary(h);
        }
        g_loadedPatches.clear();
        g_patches.clear();
        g_initialized = false;
    }

    bool ApplyPatches() {
        for (const auto& patch : g_patches) {
            if (!ApplyPatch(patch)) {
                return false;
            }
        }
        return true;
    }

    bool ApplyPatch(const PatchInfo& patch) {
        // Load patch DLL
        HMODULE hPatch = LoadLibraryA(patch.dllPath.c_str());
        if (!hPatch) {
            OutputDebugStringA(("[KotorPatcher] Failed to load: " + patch.dllPath + "\n").c_str());
            return false;
        }
        g_loadedPatches.push_back(hPatch);

        // Get function address
        void* funcAddr = GetProcAddress(hPatch, patch.functionName.c_str());
        if (!funcAddr) {
            OutputDebugStringA(("[KotorPatcher] Function not found: " + patch.functionName + "\n").c_str());
            return false;
        }

        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            OutputDebugStringA("[KotorPatcher] Original bytes mismatch - wrong game version?\n");
            return false;
        }

        // Write trampoline
        if (!Trampoline::WriteJump(patch.hookAddress, funcAddr)) {
            OutputDebugStringA("[KotorPatcher] Failed to write trampoline\n");
            return false;
        }

        return true;
    }

    const std::vector<PatchInfo>& GetLoadedPatches() {
        return g_patches;
    }
}