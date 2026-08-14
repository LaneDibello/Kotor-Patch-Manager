#include <windows.h>

#include "../ShaderSwap/ShaderSwapProvider.h"

#include "ShaderReplacements.generated.h"

namespace {
    LONG g_registered = 0;
}

extern "C" BOOL __cdecl KPatch_Initialize() {
    if (InterlockedCompareExchange(&g_registered, 1, 0) != 0) {
        return TRUE;
    }

    HMODULE shaderSwap = GetModuleHandleA("shader-swap.dll");
    if (!shaderSwap) {
        InterlockedExchange(&g_registered, 0);
        return FALSE;
    }

    auto registerProvider = reinterpret_cast<ShaderSwapRegisterProviderFn>(
        GetProcAddress(shaderSwap, "ShaderSwap_RegisterProvider"));
    if (!registerProvider || !registerProvider(kShaderReplacements, kShaderReplacementCount)) {
        InterlockedExchange(&g_registered, 0);
        return FALSE;
    }
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        return KPatch_Initialize();
    }
    return TRUE;
}
