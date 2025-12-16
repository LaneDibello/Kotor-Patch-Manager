#include "Common.h"

extern "C" void __cdecl ClearPlanets(void* partyTableThis) {
    int* availablePlanets = getObjectProperty<int*>(partyTableThis, 0x60);
    int* selectablePlanets = getObjectProperty<int*>(partyTableThis, 0xa0);

    memset(availablePlanets, 0, 4 * 0x7f);
    memset(selectablePlanets, 0, 4 * 0x7f);
}

extern "C" void __cdecl WritePlanetMask(void* gff, void* strct, void* partyTable) {
    //TODO write this out:
    /*
    * CResGFF::AddList -> "AvailablePlanets"
    * CResGFF::AddList -> "SelectablePlanets"
    * 
    * foreach PartyTable->availablePlanet -> CResGFF::AddListElement
    * foreach PartyTable->selectablePlanet -> CResGFF::AddListElement
    */
}

extern "C" void __cdecl ReadPlanetMask(void* gff, void* strct, void* partyTable) {
    //TODO: write this out:
    /*
    * Check if AvailablePlanets/SelectablePlanets exists
    * If not, use GlxyMapPlntMsk and return
    * Otherwise:
    * CResGFF::GetList/GetListCount -> "AvailablePlanets"
    * CResGFF::GetList/GetListCount -> "SelectablePlanets"
    * Loop through each list and populate PartyTable planets
    */
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