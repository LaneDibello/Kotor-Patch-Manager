#include "Common.h"
#include "GameAPI/CResGFF.h"

#define MAX_PLANETS 0x7f
#define OFFSET_AVAILABLE_PLANETS 0x60
#define OFFSET_SELECTABLE_PLANETS 0xa0

extern "C" void __cdecl ClearPlanets(void* partyTable) {
    debugLog("[PlanetsLimits] Running ClearPlanets");
    debugLog("partyTable at %X with vtable %X", partyTable, partyTable ? *(DWORD *)partyTable : 0);

    int* availablePlanets = getObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS);
    int* selectablePlanets = getObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS);

    if (availablePlanets)
        memset(availablePlanets, 0, sizeof(int) * MAX_PLANETS);
    if (selectablePlanets)
        memset(selectablePlanets, 0, sizeof(int) * MAX_PLANETS);

    debugLog("[PlanetsLimits] Finished ClearPlanets");
}

extern "C" void __cdecl WritePlanetMask(void* gff, CResStruct* strct, void* partyTable) {
    debugLog("[PlanetsLimits] Running WritePlanetMask");

    CResGFF res(gff);

    int* availablePlanets = getObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS);
    int* selectablePlanets = getObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS);

    if (!res.WriteFieldVOID(strct, (void*)availablePlanets, sizeof(int) * MAX_PLANETS, "AvailablePlanets")) {
        debugLog("[PlanetsLimits] WARNING: Failed to write AvailablePlanets");
    }
    if (!res.WriteFieldVOID(strct, (void*)selectablePlanets, sizeof(int) * MAX_PLANETS, "SelectablePlanets")) {
        debugLog("[PlanetsLimits] WARNING: Failed to write SelectablePlanets");
    }

    debugLog("[PlanetsLimits] Finished WritePlanetMask");

}

extern "C" void __cdecl ReadPlanetMask(void* gff, CResStruct* strct, void* partyTable) {
    debugLog("[PlanetsLimits] Running ReadPlanetMask");

    CResGFF res(gff);

    int* availablePlanets;
    int* selectablePlanets;

    int success = 0;

    if (!res.ReadFieldVOID(strct, (void*)availablePlanets, sizeof(int) * MAX_PLANETS, "AvailablePlanets", &success, nullptr)) {
        debugLog("[PlanetsLimits] WARNING: Failed to read AvailablePlanets");

    }
    if (!res.ReadFieldVOID(strct, (void*)selectablePlanets, sizeof(int) * MAX_PLANETS, "SelectablePlanets", &success, nullptr)) {
        debugLog("[PlanetsLimits] WARNING: Failed to read SelectablePlanets");

    }

    setObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS, availablePlanets);
    setObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS, availablePlanets);

    debugLog("[PlanetsLimits] Finished ReadPlanetMask");
}

extern "C" void __cdecl InitializePartyTablePlanets(void* partyTable) {
    debugLog("[PlanetsLimits] Running InitializePartyTablePlanets");

    int* availablePlanets = new int[MAX_PLANETS];
    memset((void*)availablePlanets, 0xff, sizeof(int) * MAX_PLANETS);
    int* selectablePlanets = new int[MAX_PLANETS];
    memset((void*)selectablePlanets, 0xff, sizeof(int) * MAX_PLANETS);

    setObjectProperty<int*>(partyTable, OFFSET_AVAILABLE_PLANETS, availablePlanets);
    setObjectProperty<int*>(partyTable, OFFSET_SELECTABLE_PLANETS, selectablePlanets);

    debugLog("[PlanetsLimits] Finished InitializePartyTablePlanets");
}



// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        debugLog("[PlanetsLimits] Attached");
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}