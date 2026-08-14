#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ShaderSwapProvider.h"

namespace {
    using GlProgramStringArbFn = void (WINAPI*)(unsigned int target, unsigned int format, int len, const void* source);
    using WglGetProcAddressFn = PROC (WINAPI*)(LPCSTR name);

    constexpr unsigned int kVertexProgramArb = 0x8620;
    constexpr unsigned int kFragmentProgramArb = 0x8804;
    constexpr unsigned int kProgramFormatAsciiArb = 0x8875;

    WglGetProcAddressFn g_originalWglGetProcAddress = nullptr;
    GlProgramStringArbFn g_originalGlProgramStringArb = nullptr;

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

    PROC WINAPI HookedWglGetProcAddress(LPCSTR name) {
        if (!g_originalWglGetProcAddress) {
            return nullptr;
        }

        PROC resolved = g_originalWglGetProcAddress(name);
        if (name && (std::strcmp(name, "glProgramStringARB") == 0 || std::strcmp(name, "glProgramString") == 0)) {
            g_originalGlProgramStringArb = reinterpret_cast<GlProgramStringArbFn>(resolved);
            return reinterpret_cast<PROC>(&HookedGlProgramStringArb);
        }
        return resolved;
    }

    bool PatchMainModuleImport(const char* importedModule, const char* functionName, void* replacement, void** original) {
        HMODULE module = GetModuleHandleA(nullptr);
        if (!module) {
            return false;
        }

        auto* base = reinterpret_cast<unsigned char*>(module);
        auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
        const auto& importDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!importDirectory.VirtualAddress) {
            return false;
        }

        auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + importDirectory.VirtualAddress);
        for (; descriptor->Name; ++descriptor) {
            const char* moduleName = reinterpret_cast<const char*>(base + descriptor->Name);
            if (_stricmp(moduleName, importedModule) != 0) {
                continue;
            }

            auto* thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->FirstThunk);
            auto* originalThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->OriginalFirstThunk);
            for (; originalThunk->u1.AddressOfData; ++originalThunk, ++thunk) {
                if (IMAGE_SNAP_BY_ORDINAL(originalThunk->u1.Ordinal)) {
                    continue;
                }

                auto* imported = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + originalThunk->u1.AddressOfData);
                if (std::strcmp(reinterpret_cast<const char*>(imported->Name), functionName) != 0) {
                    continue;
                }

                DWORD oldProtection = 0;
                if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), PAGE_READWRITE, &oldProtection)) {
                    return false;
                }
                *original = reinterpret_cast<void*>(thunk->u1.Function);
                thunk->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
                DWORD ignored = 0;
                VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), oldProtection, &ignored);
                return true;
            }
        }
        return false;
    }

    DWORD WINAPI InstallHook(LPVOID) {
        for (int attempt = 0; attempt < 240 && !GetModuleHandleA("opengl32.dll"); ++attempt) {
            Sleep(250);
        }

        if (!GetModuleHandleA("opengl32.dll")) {
            return 0;
        }

        PatchMainModuleImport(
            "opengl32.dll",
            "wglGetProcAddress",
            reinterpret_cast<void*>(&HookedWglGetProcAddress),
            reinterpret_cast<void**>(&g_originalWglGetProcAddress));
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
