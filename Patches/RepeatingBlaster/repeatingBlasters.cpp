#include "Common.h"
#include "Kotor1Functions.h"

extern "C" DWORD __cdecl RepeatingBlasterFix(DWORD esi, DWORD ebx) {
    void* baseItem = sWItemGetBaseItem((void*)esi);

    BYTE weaponType = getObjectProperty<BYTE>(baseItem, 0x8);

    if (weaponType == 6) {
        ebx = 1;
    }
    return ebx;
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