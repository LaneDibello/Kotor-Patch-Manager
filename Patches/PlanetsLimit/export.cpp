#include "Common.h"

extern "C" void __cdecl ClearPlanets(void* partyTableThis) {
    int* availablePlanets = getObjectProperty<int*>(partyTableThis, 0x60);
    int* selectablePlanets = getObjectProperty<int*>(partyTableThis, 0xa0);

    memset(availablePlanets, 0, 4 * 0x7f);
    memset(selectablePlanets, 0, 4 * 0x7f);
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