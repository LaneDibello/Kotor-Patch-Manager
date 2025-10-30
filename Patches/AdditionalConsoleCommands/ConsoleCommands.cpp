#include "Common.h"
#include "Kotor1Functions.h"
#include "ConsoleFunc.h"



extern "C" void __cdecl InitializeAdditionalCommands()
{
    
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