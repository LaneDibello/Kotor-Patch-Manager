#include "Common.h"
#include "ConsoleFunc.h"

void __cdecl setresolution(char* res) {
    DWORD width, height;
    sscanf_s(res, "%d %d", &width, &height);

    *(DWORD*)GameVersion::GetGlobalPointer("SCREEN_HEIGHT") = height;
    *(DWORD*)GameVersion::GetGlobalPointer("SCREEN_WIDTH") = width;

    typedef void(__cdecl* ReInitAuroraFn)(DWORD width, DWORD height, BYTE bitsPerPixel, int fullscreen, int checkForMovies);
    ReInitAuroraFn reInitAurora = reinterpret_cast<ReInitAuroraFn>(
        GameVersion::GetFunctionAddress("Global", "ReInitAurora"));

    reInitAurora(width, height, 32, 0, 1);

    CClientExoApp* client = CClientExoApp::GetInstance();
    if (!client) {
        return;
    }

    void* clientInternal = getObjectProperty<void*>(client->GetPtr(), 0x4);
    if (!clientInternal) {
        return;
    }

    typedef void(__thiscall* SetSizeFn)(void* thisPtr, DWORD width, DWORD height);
    SetSizeFn setSize = reinterpret_cast<SetSizeFn>(
        GameVersion::GetFunctionAddress("CSWGuiManager", "SetSize"));

    void* guiManager = getObjectProperty<void*>(clientInternal, GameVersion::GetOffset("CClientExoAppInternal", "GuiManager"));
    if (!guiManager) return;

    setSize(guiManager, width, height);

    void* mainMenu = getObjectProperty<void*>(clientInternal, GameVersion::GetOffset("CClientExoAppInternal", "MainMenu"));

    typedef int(__thiscall* PanelExistsFn)(void* thisPtr, void* panel);
    PanelExistsFn panelExists = reinterpret_cast<PanelExistsFn>(GameVersion::GetFunctionAddress("CSWGuiManager", "PanelExists"));

    if (mainMenu && panelExists(guiManager, mainMenu)) {
        typedef void(__thiscall* ConstructorFn)(void* thisPtr);
        ConstructorFn CSWGuiMainMenu = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiMainMenu", "Constructor"));

        CSWGuiMainMenu(mainMenu);
    }

    void* loadScreen = getObjectProperty<void*>(clientInternal, GameVersion::GetOffset("CClientExoAppInternal", "LoadScreen"));

    if (loadScreen) {
        callVirtualFunctionVoid<BYTE>(loadScreen, 0x0, 1); //destructor
        void* newLoadScreen = malloc(0x6b8);

        typedef void(__thiscall* ConstructorFn)(void* thisPtr, void* manager);
        ConstructorFn CSWGuiLoadScreen = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiLoadScreen", "Constructor"));

        CSWGuiLoadScreen(newLoadScreen, guiManager);

        setObjectProperty<void*>(clientInternal, GameVersion::GetOffset("CClientExoAppInternal", "LoadScreen"), newLoadScreen);
    }

    void* guiInGame = getObjectProperty<void*>(clientInternal, GameVersion::GetOffset("CClientExoAppInternal", "GuiInGame"));

    if (guiInGame) {
        typedef void(__thiscall* ResetInterfaceForSize)(void* thisPtr);
        ResetInterfaceForSize resetInterfaceForSize = reinterpret_cast<ResetInterfaceForSize>(GameVersion::GetFunctionAddress("CGuiInGame", "ResetInterfaceForSize"));

        resetInterfaceForSize(guiInGame);
    }

}

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
        return;
    }

    DWORD playerId = server->GetPlayerCreatureId();
    CSWSCreature* serverPlayer = server->GetCreatureByGameObjectID(playerId);
    if (!serverPlayer) {
        delete server;
        return;
    }

    Vector position = serverPlayer->GetPosition();
    Vector orientation = serverPlayer->GetOrientation();
    DWORD areaId = serverPlayer->GetAreaId();

    float x = position.x;
    float y = position.y;
    sscanf_s(location, "%f %f", &x, &y);

    debugLog("[teleport] serverPlayer pointer is %p", serverPlayer->GetPtr());

    int action = 0x41a00000;
    serverPlayer->AddActionToFront(5, 0xffff, 2, &x, 2, &y, 2, &position.z, 3, &areaId, 1, &takeStraightLine, 2, (void *)&action, 2, &orientation.x, 2, &orientation.y, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

    debugLog("[teleport] Done");

    delete serverPlayer;
    delete server;
}

static int* GetRenderPointer(const char* pointerName) {
    void* ptr = GameVersion::GetGlobalPointer(pointerName);
    if (!ptr) {
        debugLog("[ConsoleCommands] ERROR: Failed to get pointer for %s", pointerName);
        return nullptr;
    }
    return static_cast<int*>(ptr);
}

void __cdecl walkmeshrender() {
    static int* renderAABB = GetRenderPointer("RENDER_AABB");
    if (renderAABB) {
        *renderAABB = (*renderAABB) ^ 1;
    }
}

void __cdecl guirender() {
    static int* renderGUI = GetRenderPointer("RENDER_GUI");
    if (renderGUI) {
        *renderGUI = (*renderGUI) ^ 1;
    }
}

void __cdecl wireframerender() {
    static int* renderWireframe = GetRenderPointer("RENDER_WIREFRAME");
    if (renderWireframe) {
        *renderWireframe = (*renderWireframe) ^ 1;
    }
}

void __cdecl triggersrender() {
    static int* renderQATriggers = GetRenderPointer("RENDER_QA_TRIGGERS");
    static int* renderTriggers = GetRenderPointer("RENDER_TRIGGERS");
    if (renderQATriggers) {
        *renderQATriggers = (*renderQATriggers) ^ 1;
    }
    if (renderTriggers) {
        *renderTriggers = (*renderTriggers) ^ 1;
    }
}

void __cdecl personalspacerender() {
    static int* renderPersonalSpace = GetRenderPointer("RENDER_PERSONAL_SPACE");
    if (renderPersonalSpace) {
        *renderPersonalSpace = (*renderPersonalSpace) ^ 1;
    }
}

void __cdecl boundingboxesrender() {
    static int* renderGobBBs = GetRenderPointer("RENDER_GOB_BBS");
    if (renderGobBBs) {
        *renderGobBBs = (*renderGobBBs) ^ 1;
    }
}

void __cdecl freecam() {
    CClientExoApp* client = CClientExoApp::GetInstance();
    if (!client) {
        return;
    }

    CClientOptions* options = client->GetClientOptions();
    if (!options) {
        return;
    }

    options->SetCameraMode(7); // Mode 7 is freecam
}

void __cdecl addfeat(int feat) {
    CServerExoApp* server = CServerExoApp::GetInstance();
    if (!server) {
        return;
    }
    
    CSWSCreature* serverCreature = server->GetPlayerCreature();
    if (!serverCreature) {
        delete server;
        return;
    }

    CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
    if (!stats) {
        delete serverCreature;
        delete server;
        return;
    }

    stats->AddFeat((WORD)feat);

    delete serverCreature;
    delete server;
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
    //new ConsoleFunc("setresolution", (void*)&setresolution, STRING_PARAM);
    new ConsoleFunc("freecam", (void*)&freecam, NO_PARAMS);
    new ConsoleFunc("addfeat", (void*)&addfeat, INT_PARAM);

    // Note we never free these values, as they're present up until the game closes anyway, so 
    // memory is of minimal practical concern. May consider hooking an additional function to free
    // these in the future
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