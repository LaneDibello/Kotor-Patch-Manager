#include "Common.h"
#include "GameAPI/CResGFF.h"

#define MAX_PLANETS 0x7f
#define OFFSET_AVAILABLE_PLANETS 0x60
#define OFFSET_SELECTABLE_PLANETS 0xa0

extern "C" void __cdecl ClearPlanets(void* partyTable) {
    int* availablePlanets = getObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS);
    int* selectablePlanets = getObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS);

    memset(availablePlanets, 0, sizeof(int) * MAX_PLANETS);
    memset(selectablePlanets, 0, sizeof(int) * MAX_PLANETS);
}

extern "C" void __cdecl WritePlanetMask(void* gff, CResStruct* strct, void* partyTable) {
    CResGFF res(gff);

    int* availablePlanets = getObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS);
    int* selectablePlanets = getObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS);

    res.WriteFieldVOID(strct, (void*)availablePlanets, sizeof(int) * MAX_PLANETS, "AvailablePlanets");
    res.WriteFieldVOID(strct, (void*)selectablePlanets, sizeof(int) * MAX_PLANETS, "SelectablePlanets");

}

extern "C" void __cdecl ReadPlanetMask(void* gff, CResStruct* strct, void* partyTable) {
    CResGFF res(gff);

    int* availablePlanets;
    int* selectablePlanets;

    int success = 0;

    res.ReadFieldVOID(strct, (void*)availablePlanets, sizeof(int) * MAX_PLANETS, "AvailablePlanets", &success, nullptr);
    res.ReadFieldVOID(strct, (void*)selectablePlanets, sizeof(int) * MAX_PLANETS, "SelectablePlanets", &success, nullptr);

    setObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS, availablePlanets);
    setObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS, availablePlanets);
}

extern "C" void __cdecl InitializePartyTablePlanets(void* partyTable) {
    int* availablePlanets = new int[MAX_PLANETS];
    memset((void*)availablePlanets, 0xff, sizeof(int) * MAX_PLANETS);
    int* selectablePlanets = new int[MAX_PLANETS];
    memset((void*)selectablePlanets, 0xff, sizeof(int) * MAX_PLANETS);

    setObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS, availablePlanets);
    setObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS, selectablePlanets);
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