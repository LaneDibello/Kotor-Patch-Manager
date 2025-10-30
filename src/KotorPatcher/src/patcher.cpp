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
        char configMsg[512];
        sprintf_s(configMsg, "[KotorPatcher] Loading config from: %s\n", configPath.c_str());
        OutputDebugStringA(configMsg);

        if (!Config::ParseConfig(configPath, g_patches)) {
            OutputDebugStringA("[KotorPatcher] ERROR: Failed to parse config\n");
            return false;
        }

        sprintf_s(configMsg, "[KotorPatcher] Loaded %zu patches from config\n", g_patches.size());
        OutputDebugStringA(configMsg);

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

    // Forward declaration
    static bool ApplySimpleHook(const PatchInfo& patch);

    bool ApplyPatches() {
        for (const auto& patch : g_patches) {
            if (!ApplyPatch(patch)) {
                return false;
            }
        }
        return true;
    }

    bool ApplyPatch(const PatchInfo& patch) {
        // Handle DLL_ONLY patches (load DLL, no hooks)
        if (patch.type == HookType::DLL_ONLY) {
            HMODULE hPatch = LoadLibraryA(patch.dllPath.c_str());
            if (!hPatch) {
                DWORD errorCode = GetLastError();

                // Retrieve the system error message for the last-error code
                LPSTR messageBuffer = nullptr;
                size_t size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, errorCode,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR)&messageBuffer, 0, NULL);

                std::string errorMsg = "[KotorPatcher] Failed to load DLL-only patch: " + patch.dllPath +
                    "\nError " + std::to_string(errorCode) + ": " +
                    (messageBuffer ? messageBuffer : "Unknown error");

                OutputDebugStringA(errorMsg.c_str());

                if (messageBuffer) LocalFree(messageBuffer);
                return false;
            }
            g_loadedPatches.push_back(hPatch);

            char successMsg[256];
            sprintf_s(successMsg, "[KotorPatcher] Loaded DLL-only patch: %s\n", patch.dllPath.c_str());
            OutputDebugStringA(successMsg);
            return true;
        }

        // Handle SIMPLE hooks (no DLL loading)
        if (patch.type == HookType::SIMPLE) {
            return ApplySimpleHook(patch);
        }

        // DETOUR hook - load DLL and create wrapper
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

        char addrMsg[256];
        sprintf_s(addrMsg, "[KotorPatcher] GetProcAddress returned: 0x%08X\n", (uintptr_t)funcAddr);
        OutputDebugStringA(addrMsg);

        // Check for hot-patch stub (0xCC int3 instruction at start)
        // If present, skip it to get to the actual function
        unsigned char* pFunc = static_cast<unsigned char*>(funcAddr);
        sprintf_s(addrMsg, "[KotorPatcher] First byte at function: 0x%02X\n", *pFunc);
        OutputDebugStringA(addrMsg);

        if (*pFunc == 0xCC) {
            funcAddr = pFunc + 1;
            sprintf_s(addrMsg, "[KotorPatcher] HOT-PATCH STUB DETECTED! Adjusted to: 0x%08X\n", (uintptr_t)funcAddr);
            OutputDebugStringA(addrMsg);
        } else {
            OutputDebugStringA("[KotorPatcher] No hot-patch stub detected\n");
        }

        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            OutputDebugStringA("[KotorPatcher] Original bytes mismatch - wrong game version?\n");
            return false;
        }

        // Generate wrapper for DETOUR type
        Wrappers::WrapperConfig wrapperConfig;
        wrapperConfig.patchFunction = funcAddr;
        wrapperConfig.hookAddress = patch.hookAddress;
        wrapperConfig.originalBytes = patch.originalBytes;
        wrapperConfig.parameters = patch.parameters;

        char debugMsg[256];
        sprintf_s(debugMsg, "[KotorPatcher] Got %d original bytes\n", wrapperConfig.originalBytes.size());
        OutputDebugStringA(debugMsg);

        // Map our HookType to WrapperConfig::HookType
        wrapperConfig.type = Wrappers::WrapperConfig::HookType::DETOUR;

        wrapperConfig.preserveRegisters = patch.preserveRegisters;
        wrapperConfig.preserveFlags = patch.preserveFlags;
        wrapperConfig.excludeFromRestore = patch.excludeFromRestore;
        wrapperConfig.originalFunction = patch.originalFunction;

        void* wrapper = g_wrapperGenerator->GenerateWrapper(wrapperConfig);
        if (!wrapper) {
            OutputDebugStringA("[KotorPatcher] Failed to generate wrapper\n");
            return false;
        }

        void* targetAddress = wrapper;

        // Write trampoline to target (either wrapper or direct function)
        if (!Trampoline::WriteJump(patch.hookAddress, targetAddress)) {
            OutputDebugStringA("[KotorPatcher] Failed to write trampoline\n");
            return false;
        }

        // Clear out the remaining bytes with NOPs
        if (!Trampoline::WriteNoOps(patch.hookAddress + 5, patch.originalBytes.size() - 5)) {
            OutputDebugStringA("[KotorPatcher] Failed to write No-Ops after trampoline\n");
            return false;
        }


        char successMsg[256];
        sprintf_s(successMsg, "[KotorPatcher] Applied DETOUR hook at 0x%08X -> %s\n",
            patch.hookAddress,
            patch.functionName.c_str());
        OutputDebugStringA(successMsg);

        return true;
    }

    // Apply a SIMPLE hook (direct byte replacement)
    static bool ApplySimpleHook(const PatchInfo& patch) {
        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            OutputDebugStringA("[KotorPatcher] Original bytes mismatch for SIMPLE hook - wrong game version?\n");
            return false;
        }

        // Make memory writable
        DWORD oldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(patch.hookAddress), patch.replacementBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            OutputDebugStringA("[KotorPatcher] Failed to make memory writable for SIMPLE hook\n");
            return false;
        }

        // Write replacement bytes
        memcpy(reinterpret_cast<void*>(patch.hookAddress), patch.replacementBytes.data(), patch.replacementBytes.size());

        // Restore original protection
        DWORD dummy;
        VirtualProtect(reinterpret_cast<void*>(patch.hookAddress), patch.replacementBytes.size(), oldProtect, &dummy);

        // Flush instruction cache
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(patch.hookAddress), patch.replacementBytes.size());

        char successMsg[256];
        sprintf_s(successMsg, "[KotorPatcher] Applied SIMPLE hook at 0x%08X (%d bytes replaced)\n",
            patch.hookAddress,
            static_cast<int>(patch.replacementBytes.size()));
        OutputDebugStringA(successMsg);

        return true;
    }

    const std::vector<PatchInfo>& GetLoadedPatches() {
        return g_patches;
    }
}