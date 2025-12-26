#include "patcher.h"
#include "config_reader.h"
#include "trampoline.h"
#include "wrapper_base.h"
#include <fstream>

namespace KotorPatcher {
    static std::vector<HMODULE> g_loadedPatches;
    static std::vector<PatchInfo> g_patches;
    static std::vector<void*> g_allocatedCodeBuffers;  // Track allocated buffers for REPLACE hooks
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

        std::string versionSha;
        if (!Config::ParseConfig(configPath, g_patches, versionSha)) {
            OutputDebugStringA("[KotorPatcher] ERROR: Failed to parse config\n");
            return false;
        }

        sprintf_s(configMsg, "[KotorPatcher] Loaded %zu patches from config\n", g_patches.size());
        OutputDebugStringA(configMsg);

        // Set environment variable for patch DLLs to read
        if (!versionSha.empty()) {
            if (SetEnvironmentVariableA("KOTOR_VERSION_SHA", versionSha.c_str())) {
                sprintf_s(configMsg, "[KotorPatcher] Set KOTOR_VERSION_SHA = %s...\n", versionSha.substr(0, 16).c_str());
                OutputDebugStringA(configMsg);
            } else {
                OutputDebugStringA("[KotorPatcher] WARNING: Failed to set KOTOR_VERSION_SHA environment variable\n");
            }
        } else {
            OutputDebugStringA("[KotorPatcher] WARNING: No version SHA found in config\n");
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

        // Free allocated code buffers from REPLACE hooks
        for (void* buffer : g_allocatedCodeBuffers) {
            if (buffer) {
                VirtualFree(buffer, 0, MEM_RELEASE);
            }
        }
        g_allocatedCodeBuffers.clear();

        // Unload patch DLLs
        for (HMODULE h : g_loadedPatches) {
            if (h) FreeLibrary(h);
        }
        g_loadedPatches.clear();
        g_patches.clear();
        g_initialized = false;
    }

    // Forward declarations
    static bool ApplySimpleHook(const PatchInfo& patch);
    static bool ApplyReplaceHook(const PatchInfo& patch);

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

        // Handle REPLACE hooks (JMP to code block, no DLL loading)
        if (patch.type == HookType::REPLACE) {
            return ApplyReplaceHook(patch);
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

        sprintf_s(addrMsg, "[KotorPatcher] Function at %X\n", funcAddr);
        OutputDebugStringA(addrMsg);

        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            char errorMsg[256];
            sprintf_s(errorMsg, "[KotorPatcher] Original bytes mismatch at hookAddress %X - wrong game version?\n", patch.hookAddress);
            OutputDebugStringA(errorMsg);
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
        wrapperConfig.skipOriginalBytes = patch.skipOriginalBytes;
        wrapperConfig.originalFunction = patch.originalFunction;

        char skipMsg[128];
        sprintf_s(skipMsg, "[KotorPatcher] skipOriginalBytes = %s\n",
            patch.skipOriginalBytes ? "true" : "false");
        OutputDebugStringA(skipMsg);

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
            char errorMsg[256];
            sprintf_s(errorMsg, "[KotorPatcher] Original bytes mismatch at hookAddress %X - wrong game version?\n", patch.hookAddress);
            OutputDebugStringA(errorMsg);
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

    // Apply a REPLACE hook (JMP to code block, execute raw assembly, JMP back)
    static bool ApplyReplaceHook(const PatchInfo& patch) {
        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            char errorMsg[256];
            sprintf_s(errorMsg, "[KotorPatcher] Original bytes mismatch at hookAddress %X - wrong game version?\n", patch.hookAddress);
            OutputDebugStringA(errorMsg);
            return false;
        }

        // Calculate buffer size: replacement_bytes + 5-byte return JMP
        size_t bufferSize = patch.replacementBytes.size() + 5;

        // Allocate executable memory for replacement code + return JMP
        void* codeBuf = VirtualAlloc(nullptr, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!codeBuf) {
            OutputDebugStringA("[KotorPatcher] Failed to allocate memory for REPLACE hook\n");
            return false;
        }

        // Track allocated buffer for cleanup
        g_allocatedCodeBuffers.push_back(codeBuf);

        // Write replacement bytes to allocated memory
        memcpy(codeBuf, patch.replacementBytes.data(), patch.replacementBytes.size());

        // Calculate return address (after original bytes)
        void* returnAddr = reinterpret_cast<void*>(patch.hookAddress + patch.originalBytes.size());

        // Write JMP back to game code at end of replacement bytes
        BYTE* returnJmp = static_cast<BYTE*>(codeBuf) + patch.replacementBytes.size();
        *returnJmp = 0xE9;  // JMP opcode
        DWORD offset = reinterpret_cast<DWORD>(returnAddr) - (reinterpret_cast<DWORD>(returnJmp) + 5);
        memcpy(returnJmp + 1, &offset, 4);

        // Flush instruction cache for code buffer
        FlushInstructionCache(GetCurrentProcess(), codeBuf, bufferSize);

        // Write JMP at hook address to code buffer
        if (!Trampoline::WriteJump(patch.hookAddress, codeBuf)) {
            OutputDebugStringA("[KotorPatcher] Failed to write REPLACE hook JMP\n");
            return false;
        }

        // Write NOPs for remaining bytes (if original_bytes > 5)
        if (patch.originalBytes.size() > 5) {
            if (!Trampoline::WriteNoOps(patch.hookAddress + 5, patch.originalBytes.size() - 5)) {
                OutputDebugStringA("[KotorPatcher] Failed to write NOPs for REPLACE hook\n");
                return false;
            }
        }

        char successMsg[256];
        sprintf_s(successMsg, "[KotorPatcher] Applied REPLACE hook at 0x%08X (%d bytes code, %d bytes replaced)\n",
            patch.hookAddress,
            static_cast<int>(patch.replacementBytes.size()),
            static_cast<int>(patch.originalBytes.size()));
        OutputDebugStringA(successMsg);

        return true;
    }

    const std::vector<PatchInfo>& GetLoadedPatches() {
        return g_patches;
    }
}