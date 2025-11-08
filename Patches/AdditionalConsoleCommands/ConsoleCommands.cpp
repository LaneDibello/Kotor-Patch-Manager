#include "Common.h"
#include "Kotor1Functions.h"  // Still needed for sWSObjectAddActionToFront
#include "ConsoleFunc.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CExoString.h"

void __cdecl runscript(char* script) {
    CExoString scriptFile(script);

    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) return;

    DWORD playerId = server->GetPlayerCreatureId();

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (vm) {
        vm->RunScript(&scriptFile, playerId, 1);
        delete vm;
    }

    delete server;
}

void __cdecl teleport(char* location) {
    // Location formatted like "x y"
    int takeStraightLine = 1;

    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) {
        debugLog("[teleport] Server is %p", server);
        return;
    }

    DWORD playerId = server->GetPlayerCreatureId();
    CSWSCreature* serverPlayer = server->GetCreatureByGameObjectID(playerId);
    if (!serverPlayer) {
        delete server;
        debugLog("[teleport] serverPlayer is null");
        return;
    }

    Vector position = serverPlayer->GetPosition();
    Vector orientation = serverPlayer->GetOrientationVector();
    DWORD areaId = serverPlayer->GetAreaId();

    debugLog("[teleport] areaId is %d", areaId);

    float x = position.x;
    float y = position.y;
    sscanf_s(location, "%f %f", &x, &y);

    int action = 0x41a00000;

    sWSObjectAddActionToFront(serverPlayer->GetPtr(), 5, 0xffff, 2, &x, 2, &y, 2, &position.z, 3, &areaId, 1, &takeStraightLine, 2, (void *)&action, 2, &orientation.x, 2, &orientation.y, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

    debugLog("[teleport] Done");

    delete serverPlayer;
    delete server;
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

    // Note we never free these values, as they're present up until the game closes anyway, so 
    // memory is of minimal practical concern. May consider hooking an additional function to free
    // these in the function
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize GameVersion system (reads from KOTOR_VERSION_SHA env var and addresses.toml)
        if (!GameVersion::Initialize()) {
            OutputDebugStringA("[AdditionalConsoleCommands] ERROR: GameVersion::Initialize() failed\n");
            return FALSE;
        }
        OutputDebugStringA("[AdditionalConsoleCommands] GameVersion initialized successfully\n");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}