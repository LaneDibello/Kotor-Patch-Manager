#define WIN32_LEAN_AND_MEAN 

// dllmain.cpp : Entry point for the KOTOR patcher runtime
#include <windows.h>
#include "patcher.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        if (!KotorPatcher::InitializePatcher()) {
            OutputDebugStringA("[KotorPatcher] Failed to initialize\n");
        }
        break;

    case DLL_PROCESS_DETACH:
        KotorPatcher::CleanupPatcher();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}