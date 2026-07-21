#include "patcher.h"
#include "config_reader.h"
#include "platform.h"
#include "trampoline.h"
#include "wrapper_base.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace KotorPatcher {
    static std::vector<void*> g_loadedPatches;
    static std::vector<PatchInfo> g_patches;

    // REPLACE hooks allocate a code block that must be released at cleanup, and
    // FreeExec needs the length, so the size is tracked alongside the pointer.
    struct CodeBuffer { void* addr; std::size_t size; };
    static std::vector<CodeBuffer> g_allocatedCodeBuffers;

    static bool g_initialized = false;
    static Wrappers::WrapperGeneratorBase* g_wrapperGenerator = nullptr;

    // KOTOR1 on Steam ships behind SteamStub DRM: its .text is encrypted on disk and
    // only decrypted in memory by the stub, which runs after our proxy has already
    // loaded us. At load time a hook site therefore still reads as ciphertext and
    // every VerifyBytes would fail. When we detect that, the apply is handed to a worker
    // that waits for decryption instead of giving up. GOG/retail, the non-stubbed
    // Steam builds, and the native Linux ELF decrypt nothing, so their hook sites read
    // correctly at init and this path is never taken.
    static constexpr long kDecryptPollIntervalMs = 15;
    static constexpr long kDecryptTimeoutMs = 30000;

    // First patch that carries a code hook, or nullptr if every patch is DLL_ONLY. Its
    // original bytes double as the "is the code readable yet" sentinel: they only match
    // once any on-disk encryption has been undone in memory.
    static const PatchInfo* FindHookSentinel() {
        for (const auto& patch : g_patches) {
            if (patch.type != HookType::DLL_ONLY && !patch.originalBytes.empty()) {
                return &patch;
            }
        }
        return nullptr;
    }

    static bool HookSiteReadable(const PatchInfo* sentinel) {
        return sentinel != nullptr &&
            Trampoline::VerifyBytes(sentinel->hookAddress,
                sentinel->originalBytes.data(), sentinel->originalBytes.size());
    }

    // Runs off the loader path when the code is still encrypted at init. Polls the
    // sentinel until the stub has decrypted .text, then applies normally. The patched
    // functions are gameplay/menu code that runs long after startup, so applying a
    // moment into the game (rather than before its entry point) is safe in practice.
    static void DeferredApply() {
        const PatchInfo* sentinel = FindHookSentinel();
        auto start = std::chrono::steady_clock::now();
        while (!HookSiteReadable(sentinel)) {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsedMs >= kDecryptTimeoutMs) {
                Platform::Log("[KotorPatcher] Timed out waiting for code decryption; game left unpatched\n");
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kDecryptPollIntervalMs));
        }
        Platform::Log("[KotorPatcher] Code decrypted; applying deferred patches\n");
        ApplyPatches();
    }

    bool InitializePatcher() {
        if (g_initialized) return true;

        // Initialize wrapper generator
        g_wrapperGenerator = Wrappers::GetWrapperGenerator();
        if (!g_wrapperGenerator) {
            Platform::Log("[KotorPatcher] Failed to get wrapper generator\n");
            return false;
        }

        char platformMsg[128];
        snprintf(platformMsg, sizeof(platformMsg), "[KotorPatcher] Using wrapper generator: %s\n",
            g_wrapperGenerator->GetPlatformName());
        Platform::Log(platformMsg);

        // Get the directory holding this patcher module; the config sits beside it.
        std::string moduleDir = Platform::SelfModuleDir();
        if (moduleDir.empty()) {
            Platform::Log("[KotorPatcher] Failed to resolve module directory\n");
            return false;
        }

        // moduleDir is a '\' path on Windows, but Win32 file I/O canonicalizes '/'
        // to '\', so appending a '/' separator opens the config on both platforms.
        std::string configPath = moduleDir + "/patch_config.toml";
        char configMsg[512];
        snprintf(configMsg, sizeof(configMsg), "[KotorPatcher] Loading config from: %s\n", configPath.c_str());
        Platform::Log(configMsg);

        std::string versionSha;
        if (!Config::ParseConfig(configPath, g_patches, versionSha)) {
            Platform::Log("[KotorPatcher] ERROR: Failed to parse config\n");
            return false;
        }

        snprintf(configMsg, sizeof(configMsg), "[KotorPatcher] Loaded %zu patches from config\n", g_patches.size());
        Platform::Log(configMsg);

        // Set environment variable for patch DLLs to read
        if (!versionSha.empty()) {
            if (Platform::SetEnv("KOTOR_VERSION_SHA", versionSha.c_str())) {
                snprintf(configMsg, sizeof(configMsg), "[KotorPatcher] Set KOTOR_VERSION_SHA = %s...\n", versionSha.substr(0, 16).c_str());
                Platform::Log(configMsg);
            } else {
                Platform::Log("[KotorPatcher] WARNING: Failed to set KOTOR_VERSION_SHA environment variable\n");
            }
        } else {
            Platform::Log("[KotorPatcher] WARNING: No version SHA found in config\n");
        }

        // If the hook sites are still encrypted (SteamStub), wait for the stub to
        // decrypt them on a worker thread instead of failing every VerifyBytes now.
        const PatchInfo* sentinel = FindHookSentinel();
        if (sentinel != nullptr && !HookSiteReadable(sentinel)) {
            Platform::Log("[KotorPatcher] Hook site still encrypted (SteamStub?); deferring apply\n");
            try {
                std::thread(DeferredApply).detach();
                g_initialized = true;
                return true;
            } catch (...) {
                // No worker available: fall through to a synchronous apply. VerifyBytes
                // still fails safe if the code really is encrypted.
                Platform::Log("[KotorPatcher] WARNING: could not spawn worker; applying synchronously\n");
            }
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
        for (const auto& buffer : g_allocatedCodeBuffers) {
            Platform::FreeExec(buffer.addr, buffer.size);
        }
        g_allocatedCodeBuffers.clear();

        // Unload patch DLLs
        for (void* h : g_loadedPatches) {
            Platform::UnloadModule(h);
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
            void* hPatch = Platform::LoadModule(patch.dllPath.c_str());
            if (!hPatch) {
                Platform::Log(("[KotorPatcher] Failed to load DLL-only patch: " + patch.dllPath +
                    " (" + Platform::LastLoadError() + ")\n").c_str());
                return false;
            }
            g_loadedPatches.push_back(hPatch);

            char successMsg[256];
            snprintf(successMsg, sizeof(successMsg), "[KotorPatcher] Loaded DLL-only patch: %s\n", patch.dllPath.c_str());
            Platform::Log(successMsg);
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
        void* hPatch = Platform::LoadModule(patch.dllPath.c_str());
        if (!hPatch) {
            Platform::Log(("[KotorPatcher] Failed to load: " + patch.dllPath +
                " (" + Platform::LastLoadError() + ")\n").c_str());
            return false;
        }
        g_loadedPatches.push_back(hPatch);
        // Get function address
        void* funcAddr = Platform::GetSymbol(hPatch, patch.functionName.c_str());
        if (!funcAddr) {
            Platform::Log(("[KotorPatcher] Function not found: " + patch.functionName + "\n").c_str());
            return false;
        }

        char addrMsg[256];
        snprintf(addrMsg, sizeof(addrMsg), "[KotorPatcher] Symbol resolved: %s at 0x%08X\n",
            patch.functionName.c_str(), reinterpret_cast<uint32_t>(funcAddr));
        Platform::Log(addrMsg);

        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg), "[KotorPatcher] Original bytes mismatch at hookAddress 0x%08X - wrong game version?\n", patch.hookAddress);
            Platform::Log(errorMsg);
            return false;
        }

        // Generate wrapper for DETOUR type
        Wrappers::WrapperConfig wrapperConfig;
        wrapperConfig.patchFunction = funcAddr;
        wrapperConfig.hookAddress = patch.hookAddress;
        wrapperConfig.originalBytes = patch.originalBytes;
        wrapperConfig.parameters = patch.parameters;

        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), "[KotorPatcher] Got %zu original bytes\n", wrapperConfig.originalBytes.size());
        Platform::Log(debugMsg);

        // Map our HookType to WrapperConfig::HookType
        wrapperConfig.type = Wrappers::WrapperConfig::HookType::DETOUR;

        wrapperConfig.preserveRegisters = patch.preserveRegisters;
        wrapperConfig.preserveFlags = patch.preserveFlags;
        wrapperConfig.excludeFromRestore = patch.excludeFromRestore;
        wrapperConfig.skipOriginalBytes = patch.skipOriginalBytes;
        wrapperConfig.originalFunction = patch.originalFunction;

        char skipMsg[128];
        snprintf(skipMsg, sizeof(skipMsg), "[KotorPatcher] skipOriginalBytes = %s\n",
            patch.skipOriginalBytes ? "true" : "false");
        Platform::Log(skipMsg);

        void* wrapper = g_wrapperGenerator->GenerateWrapper(wrapperConfig);
        if (!wrapper) {
            Platform::Log("[KotorPatcher] Failed to generate wrapper\n");
            return false;
        }

        void* targetAddress = wrapper;

        // Write trampoline to target (either wrapper or direct function)
        if (!Trampoline::WriteJump(patch.hookAddress, targetAddress)) {
            Platform::Log("[KotorPatcher] Failed to write trampoline\n");
            return false;
        }

        // Clear out the remaining bytes with NOPs
        if (!Trampoline::WriteNoOps(patch.hookAddress + 5, patch.originalBytes.size() - 5)) {
            Platform::Log("[KotorPatcher] Failed to write No-Ops after trampoline\n");
            return false;
        }

        char successMsg[256];
        snprintf(successMsg, sizeof(successMsg), "[KotorPatcher] Applied DETOUR hook at 0x%08X -> %s\n",
            patch.hookAddress,
            patch.functionName.c_str());
        Platform::Log(successMsg);

        return true;
    }

    // Apply a SIMPLE hook (direct byte replacement)
    static bool ApplySimpleHook(const PatchInfo& patch) {
        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg), "[KotorPatcher] Original bytes mismatch at hookAddress 0x%08X - wrong game version?\n", patch.hookAddress);
            Platform::Log(errorMsg);
            return false;
        }

        // Overwrite the site with the replacement bytes (same length as originals).
        if (!Platform::WriteCode(reinterpret_cast<void*>(patch.hookAddress),
                patch.replacementBytes.data(), patch.replacementBytes.size())) {
            Platform::Log("[KotorPatcher] Failed to write bytes for SIMPLE hook\n");
            return false;
        }

        char successMsg[256];
        snprintf(successMsg, sizeof(successMsg), "[KotorPatcher] Applied SIMPLE hook at 0x%08X (%zu bytes replaced)\n",
            patch.hookAddress,
            patch.replacementBytes.size());
        Platform::Log(successMsg);

        return true;
    }

    // Apply a REPLACE hook (JMP to code block, execute raw assembly, JMP back)
    static bool ApplyReplaceHook(const PatchInfo& patch) {
        // Verify original bytes
        if (!Trampoline::VerifyBytes(patch.hookAddress, patch.originalBytes.data(), patch.originalBytes.size())) {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg), "[KotorPatcher] Original bytes mismatch at hookAddress 0x%08X - wrong game version?\n", patch.hookAddress);
            Platform::Log(errorMsg);
            return false;
        }

        // Calculate buffer size: replacement_bytes + 5-byte return JMP
        std::size_t bufferSize = patch.replacementBytes.size() + 5;

        // Allocate executable memory for replacement code + return JMP
        void* codeBuf = Platform::AllocExec(bufferSize);
        if (!codeBuf) {
            Platform::Log("[KotorPatcher] Failed to allocate memory for REPLACE hook\n");
            return false;
        }

        // Track allocated buffer for cleanup
        g_allocatedCodeBuffers.push_back({ codeBuf, bufferSize });

        // Write replacement bytes to allocated memory (already writable+executable)
        std::memcpy(codeBuf, patch.replacementBytes.data(), patch.replacementBytes.size());

        // Calculate return address (after original bytes)
        void* returnAddr = reinterpret_cast<void*>(patch.hookAddress + patch.originalBytes.size());

        // Write JMP back to game code at end of replacement bytes
        uint8_t* returnJmp = static_cast<uint8_t*>(codeBuf) + patch.replacementBytes.size();
        *returnJmp = 0xE9;  // JMP opcode
        uint32_t offset = reinterpret_cast<uint32_t>(returnAddr) - (reinterpret_cast<uint32_t>(returnJmp) + 5);
        std::memcpy(returnJmp + 1, &offset, 4);

        // Flush instruction cache for code buffer
        Platform::FlushICache(codeBuf, bufferSize);

        // Write JMP at hook address to code buffer
        if (!Trampoline::WriteJump(patch.hookAddress, codeBuf)) {
            Platform::Log("[KotorPatcher] Failed to write REPLACE hook JMP\n");
            return false;
        }

        // Write NOPs for remaining bytes (if original_bytes > 5)
        if (patch.originalBytes.size() > 5) {
            if (!Trampoline::WriteNoOps(patch.hookAddress + 5, patch.originalBytes.size() - 5)) {
                Platform::Log("[KotorPatcher] Failed to write NOPs for REPLACE hook\n");
                return false;
            }
        }

        char successMsg[256];
        snprintf(successMsg, sizeof(successMsg), "[KotorPatcher] Applied REPLACE hook at 0x%08X (%zu bytes code, %zu bytes replaced)\n",
            patch.hookAddress,
            patch.replacementBytes.size(),
            patch.originalBytes.size());
        Platform::Log(successMsg);

        return true;
    }

    const std::vector<PatchInfo>& GetLoadedPatches() {
        return g_patches;
    }
}
