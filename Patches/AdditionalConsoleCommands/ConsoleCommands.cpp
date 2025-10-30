#include "Common.h"
#include "Kotor1Functions.h"
#include "ConsoleFunc.h"

void __cdecl runscript(char* script) {
    CExoString scriptFile(script);

    DWORD playerId = serverExoAppGetPlayerCreatureId(getServerExoApp());

    virtualMachineRunScript(*VIRTUAL_MACHINE_PTR, &scriptFile, playerId, 1);
}

void __cdecl teleport(char* location) {
    // Location formatted like "x y"
    int takeStraightLine = 1;
    void* server = getServerExoApp();
    void* serverPlayer = serverExoAppGetCreatureByGameObjectID(server, serverExoAppGetPlayerCreatureId(server));
    Vector position = getObjectProperty<Vector>(serverPlayer, 0x90);
    Vector orientation = getObjectProperty<Vector>(serverPlayer, 0x9c);
    DWORD areaId = getObjectProperty<DWORD>(serverPlayer, 0x8c);

    float x = position.x;
    float y = position.y;
    sscanf_s(location, "%f %f", &x, &y);

    int action = 0x41a00000;

    sWSObjectAddActionToFront(serverPlayer, 5, 0xffff, 2, &x, 2, &y, 2, &position.z, 3, &areaId, 1, &takeStraightLine, 2, (void *)&action, 2, &orientation.x, 2, &orientation.y, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

}

extern "C" void __cdecl InitializeAdditionalCommands()
{
    ConsoleFunc* command_runscript = new ConsoleFunc("runscript", (void*)&runscript, STRING_PARAM);
    ConsoleFunc* command_teleport = new ConsoleFunc("teleport", (void*)&teleport, STRING_PARAM);
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