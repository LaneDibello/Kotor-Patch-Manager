#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ShaderSwapProvider.h"
#include "X86InlineDetour.h"

namespace {
    using GlProgramStringArbFn = void (WINAPI*)(unsigned int target, unsigned int format, int len, const void* source);
    using WglGetProcAddressFn = PROC (WINAPI*)(LPCSTR name);

    constexpr unsigned int kProgramFormatAsciiArb = 0x8875;

    enum class HookMode {
        Uninitialized,
        Detoured,
        WrapperFallback,
    };

    WglGetProcAddressFn g_originalWglGetProcAddress = nullptr;
    GlProgramStringArbFn g_originalGlProgramStringArb = nullptr;
    x86hook::InlineDetour g_wglGetProcAddressDetour;
    x86hook::InlineDetour g_programStringDetour;
    HookMode g_programStringHookMode = HookMode::Uninitialized;
    SRWLOCK g_detourLock = SRWLOCK_INIT;

    struct ShaderSwapProvider {
        const ShaderSwapReplacement* replacements;
        unsigned int count;
    };

    constexpr unsigned int kMaxProviders = 64;
    ShaderSwapProvider g_providers[kMaxProviders]{};
    unsigned int g_providerCount = 0;
    SRWLOCK g_providerLock = SRWLOCK_INIT;
    LONG g_hookStarted = 0;

    std::uint64_t HashShaderSource(const char* source, std::size_t size) {
        std::uint64_t hash = 1469598103934665603ull;
        for (std::size_t i = 0; i < size; ++i) {
            hash ^= static_cast<unsigned char>(source[i]);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    const ShaderSwapReplacement* FindReplacement(unsigned int target, const char* source, std::size_t size) {
        const std::uint64_t hash = HashShaderSource(source, size);
        const ShaderSwapReplacement* match = nullptr;
        AcquireSRWLockShared(&g_providerLock);
        for (unsigned int providerIndex = 0; providerIndex < g_providerCount && !match; ++providerIndex) {
            const auto& provider = g_providers[providerIndex];
            for (unsigned int replacementIndex = 0; replacementIndex < provider.count; ++replacementIndex) {
                const auto& replacement = provider.replacements[replacementIndex];
                if (replacement.target == target && replacement.originalHash == hash) {
                    match = &replacement;
                    break;
                }
            }
        }
        ReleaseSRWLockShared(&g_providerLock);
        return match;
    }

    void WINAPI HookedGlProgramStringArb(unsigned int target, unsigned int format, int len, const void* source) {
        if (!g_originalGlProgramStringArb) {
            return;
        }

        if (format == kProgramFormatAsciiArb && source && len > 0) {
            const auto* replacement = FindReplacement(
                target,
                static_cast<const char*>(source),
                static_cast<std::size_t>(len));
            if (replacement) {
                g_originalGlProgramStringArb(
                    target,
                    format,
                    static_cast<int>(replacement->sourceSize),
                    replacement->source);
                return;
            }
        }

        g_originalGlProgramStringArb(target, format, len, source);
    }

    PROC ResolveProgramStringEntry(PROC resolved) {
        if (!resolved) {
            return nullptr;
        }

        AcquireSRWLockExclusive(&g_detourLock);
        PROC result = resolved;
        if (g_programStringHookMode == HookMode::Uninitialized) {
            if (g_programStringDetour.Install(
                    reinterpret_cast<void*>(resolved),
                    reinterpret_cast<void*>(&HookedGlProgramStringArb))) {
                g_originalGlProgramStringArb =
                    reinterpret_cast<GlProgramStringArbFn>(g_programStringDetour.Original());
                g_programStringHookMode = HookMode::Detoured;
            } else {
                g_originalGlProgramStringArb = reinterpret_cast<GlProgramStringArbFn>(resolved);
                g_programStringHookMode = HookMode::WrapperFallback;
                result = reinterpret_cast<PROC>(&HookedGlProgramStringArb);
            }
        } else if (g_programStringHookMode == HookMode::WrapperFallback &&
                   reinterpret_cast<PROC>(g_originalGlProgramStringArb) == resolved) {
            result = reinterpret_cast<PROC>(&HookedGlProgramStringArb);
        }
        ReleaseSRWLockExclusive(&g_detourLock);
        return result;
    }

    PROC WINAPI HookedWglGetProcAddress(LPCSTR name) {
        if (!g_originalWglGetProcAddress) {
            return nullptr;
        }

        PROC resolved = g_originalWglGetProcAddress(name);
        if (name && (std::strcmp(name, "glProgramStringARB") == 0 || std::strcmp(name, "glProgramString") == 0)) {
            return ResolveProgramStringEntry(resolved);
        }
        return resolved;
    }

    DWORD WINAPI InstallHook(LPVOID) {
        HMODULE openGl = nullptr;
        for (int attempt = 0; attempt < 240 && !openGl; ++attempt) {
            openGl = GetModuleHandleA("opengl32.dll");
            if (!openGl) {
                Sleep(250);
            }
        }
        if (!openGl) {
            return 0;
        }

        void* wglGetProcAddress = reinterpret_cast<void*>(GetProcAddress(openGl, "wglGetProcAddress"));
        if (!g_wglGetProcAddressDetour.Install(
                wglGetProcAddress, reinterpret_cast<void*>(&HookedWglGetProcAddress))) {
            return 0;
        }
        g_originalWglGetProcAddress =
            reinterpret_cast<WglGetProcAddressFn>(g_wglGetProcAddressDetour.Original());
        return 0;
    }
}

extern "C" BOOL __cdecl ShaderSwap_RegisterProvider(
    const ShaderSwapReplacement* replacements,
    unsigned int count) {
    if (!replacements || count == 0) {
        return FALSE;
    }

    for (unsigned int index = 0; index < count; ++index) {
        if (!replacements[index].source || replacements[index].sourceSize == 0 ||
            replacements[index].sourceSize > 0x7fffffffu) {
            return FALSE;
        }
    }

    AcquireSRWLockExclusive(&g_providerLock);
    if (g_providerCount >= kMaxProviders) {
        ReleaseSRWLockExclusive(&g_providerLock);
        return FALSE;
    }
    g_providers[g_providerCount++] = {replacements, count};
    ReleaseSRWLockExclusive(&g_providerLock);
    return TRUE;
}

extern "C" BOOL __cdecl KPatch_Initialize() {
    if (InterlockedCompareExchange(&g_hookStarted, 1, 0) != 0) {
        return TRUE;
    }

    HANDLE thread = CreateThread(nullptr, 0, &InstallHook, nullptr, 0, nullptr);
    if (!thread) {
        InterlockedExchange(&g_hookStarted, 0);
        return FALSE;
    }
    CloseHandle(thread);
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        return KPatch_Initialize();
    }
    return TRUE;
}
