#define WIN32_LEAN_AND_MEAN 

// dllmain.cpp : Entry point for the KOTOR patcher runtime
#include <windows.h>
#include "patcher.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        OutputDebugStringA("[KotorPatcher] DLL_PROCESS_ATTACH - DLL loaded!\n");
        DisableThreadLibraryCalls(hModule);

        OutputDebugStringA("[KotorPatcher] Calling InitializePatcher()...\n");
        if (!KotorPatcher::InitializePatcher()) {
            OutputDebugStringA("[KotorPatcher] ERROR: Failed to initialize\n");
        } else {
            OutputDebugStringA("[KotorPatcher] SUCCESS: Patcher initialized\n");
        }
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringA("[KotorPatcher] DLL_PROCESS_DETACH - Cleaning up\n");
        KotorPatcher::CleanupPatcher();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}