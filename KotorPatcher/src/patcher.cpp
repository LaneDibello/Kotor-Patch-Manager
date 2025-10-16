#include "patcher.h"
#include "config_reader.h"
#include "trampoline.h"
#include "wrapper_base.h"
#include <fstream>

namespace KotorPatcher {
    static std::vector<HMODULE> g_loadedPatches;
    static std::vector<PatchInfo> g_patches;
    static bool g_initialized = false;
    static Wrappers::WrapperGeneratorBase* g_wrapperGenerator = nullptr;

    bool InitializePatcher() {
        if (g_initialized) return true;

        // Initialize wrapper generator
        g_wrapperGenerator = Wrappers::GetWrapperGenerator();
        if (!g_wrapperGenerator) {
            OutputDebugStringA("[KotorPatcher] Failed to get wrapper generator\n");
            return false;
        }

        char platformMsg[128];
        sprintf_s(platformMsg, "[KotorPatcher] Using wrapper generator: %s\n",
            g_wrapperGenerator->GetPlatformName());
        OutputDebugStringA(platformMsg);

        // Get DLL directory
        char dllPath[MAX_PATH];
        HMODULE hModule = GetModuleHandleA("KotorPatcher.dll");
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
        // Free wrapper stubs
        if (g_wrapperGenerator) {
            g_wrapperGenerator->FreeAllWrappers();
        }

        // Unload patch DLLs
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

        // Generate wrapper (or use direct JMP for REPLACE type)
        void* targetAddress = funcAddr;  // Default to direct function address

        // For INLINE and WRAP types, generate a wrapper stub
        if (patch.type == HookType::INLINE || patch.type == HookType::WRAP) {
            Wrappers::WrapperConfig wrapperConfig;
            wrapperConfig.patchFunction = funcAddr;
            wrapperConfig.hookAddress = patch.hookAddress;

            // Map our HookType to WrapperConfig::HookType
            switch (patch.type) {
            case HookType::INLINE:
                wrapperConfig.type = Wrappers::WrapperConfig::HookType::INLINE;
                break;
            case HookType::WRAP:
                wrapperConfig.type = Wrappers::WrapperConfig::HookType::WRAP;
                break;
            case HookType::REPLACE:
                wrapperConfig.type = Wrappers::WrapperConfig::HookType::REPLACE;
                break;
            }

            wrapperConfig.preserveRegisters = patch.preserveRegisters;
            wrapperConfig.preserveFlags = patch.preserveFlags;
            wrapperConfig.excludeFromRestore = patch.excludeFromRestore;
            wrapperConfig.originalFunction = patch.originalFunction;

            void* wrapper = g_wrapperGenerator->GenerateWrapper(wrapperConfig);
            if (!wrapper) {
                OutputDebugStringA("[KotorPatcher] Failed to generate wrapper\n");
                return false;
            }

            targetAddress = wrapper;  // Jump to wrapper instead of patch function
        }

        // Write trampoline to target (either wrapper or direct function)
        if (!Trampoline::WriteJump(patch.hookAddress, targetAddress)) {
            OutputDebugStringA("[KotorPatcher] Failed to write trampoline\n");
            return false;
        }

        const char* typeNames[] = { "INLINE", "REPLACE", "WRAP" };
        char successMsg[256];
        sprintf_s(successMsg, "[KotorPatcher] Applied %s hook at 0x%08X -> %s\n",
            typeNames[static_cast<int>(patch.type)],
            patch.hookAddress,
            patch.functionName.c_str());
        OutputDebugStringA(successMsg);

        return true;
    }

    const std::vector<PatchInfo>& GetLoadedPatches() {
        return g_patches;
    }
}