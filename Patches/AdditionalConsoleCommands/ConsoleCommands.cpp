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

void __cdecl walkmeshrender() {
    int* renderAABB = (int*)0x007fbf5c;
    *renderAABB = (*renderAABB) ^ 1;
}

void __cdecl guirender() {
    int* renderGUI = (int*)0x007bb4d0;
    *renderGUI = (*renderGUI) ^ 1;
}

void __cdecl wireframerender() {
    int* renderWireframe = (int*)0x007bb4f0;
    *renderWireframe = (*renderWireframe) ^ 1;
}

void __cdecl triggersrender() {
    int* renderQATriggers = (int*)0x0083285c;
    *renderQATriggers = (*renderQATriggers) ^ 1;
    int* renderTriggers = (int*)0x007b92e4;
    *renderTriggers = (*renderTriggers) ^ 1;
}

void __cdecl personalspacerender() {
    int* renderPersonalSpace = (int*)0x007b9314;
    *renderPersonalSpace = (*renderPersonalSpace) ^ 1;
}

void __cdecl boundingboxesrender() {
    int* renderGobBBs = (int*)0x0082805c;
    *renderGobBBs = (*renderGobBBs) ^ 1;
}

extern "C" void __cdecl InitializeAdditionalCommands()
{
    new ConsoleFunc("runscript", (void*)&runscript, STRING_PARAM);
    new ConsoleFunc("teleport", (void*)&teleport, STRING_PARAM);
    new ConsoleFunc("walkmeshrender", (void*)&walkmeshrender, NO_PARAMS);
    new ConsoleFunc("guirender", (void*)&guirender, NO_PARAMS);
    new ConsoleFunc("wireframerender", (void*)&wireframerender, NO_PARAMS);
    new ConsoleFunc("triggersrender", (void*)&triggersrender, NO_PARAMS);
    new ConsoleFunc("personalspacerender", (void*)&personalspacerender, NO_PARAMS);
    new ConsoleFunc("boundingboxesrender", (void*)&boundingboxesrender, NO_PARAMS);
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