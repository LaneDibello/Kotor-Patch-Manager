#include <windows.h>
#include <cstdio>

extern "C" void __cdecl InitializeExtensionCommands(DWORD * commands)
{
    char buffer[128];
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Commands array at %p", commands);
    OutputDebugStringA(buffer);
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}