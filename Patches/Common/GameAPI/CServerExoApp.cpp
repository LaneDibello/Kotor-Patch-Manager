#include "CServerExoApp.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CSWSCreature.h"

CServerExoApp::GetObjectArrayFn CServerExoApp::getObjectArray = nullptr;
CServerExoApp::GetPlayerCreatureIdFn CServerExoApp::getPlayerCreatureId = nullptr;
CServerExoApp::GetCreatureByGameObjectIDFn CServerExoApp::getCreatureByGameObjectID = nullptr;
bool CServerExoApp::functionsInitialized = false;

void** appManagerGlobalPtr = nullptr;

void CServerExoApp::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CServerExoApp] ERROR: GameVersion not initialized\n");
        return;
    }

    appManagerGlobalPtr = static_cast<void**>(GameVersion::GetGlobalPointer("APP_MANAGER_PTR"));
    if (!appManagerGlobalPtr) {
        OutputDebugStringA("[CServerExoApp] ERROR: APP_MANAGER_PTR not found\n");
        return;
    }

    try {
        getObjectArray = reinterpret_cast<GetObjectArrayFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetObjectArray")
        );

        getPlayerCreatureId = reinterpret_cast<GetPlayerCreatureIdFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetPlayerCreatureId")
        );

        getCreatureByGameObjectID = reinterpret_cast<GetCreatureByGameObjectIDFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetCreatureByGameObjectID")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CServerExoApp] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

CServerExoApp* CServerExoApp::GetInstance() {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!appManagerGlobalPtr || !*appManagerGlobalPtr) {
        OutputDebugStringA("[CServerExoApp] ERROR: App manager pointer is null\n");
        return nullptr;
    }

    void* appManager = *appManagerGlobalPtr;

    try {
        int serverOffset = GameVersion::GetOffset("CAppManager", "Server");
        void* serverExoApp = getObjectProperty<void*>(appManager, serverOffset);

        if (!serverExoApp) {
            OutputDebugStringA("[CServerExoApp] ERROR: CServerExoApp pointer is null\n");
            return nullptr;
        }

        return new CServerExoApp(serverExoApp);
    }
    catch (const GameVersionException& e) {
        debugLog("[CServerExoApp] ERROR: %s\n", e.what());
        return nullptr;
    }
}

CServerExoApp::CServerExoApp(void* serverPtr)
    : serverPtr(serverPtr)
{
}

CServerExoApp::~CServerExoApp() {
    serverPtr = nullptr;
}

void* CServerExoApp::GetObjectArray() {
    if (!serverPtr || !getObjectArray) {
        return nullptr;
    }

    return getObjectArray(serverPtr);
}

DWORD CServerExoApp::GetPlayerCreatureId() {
    if (!serverPtr || !getPlayerCreatureId) {
        return 0x7F000000;
    }

    return getPlayerCreatureId(serverPtr);
}

CSWSCreature* CServerExoApp::GetCreatureByGameObjectID(DWORD objectId) {
    if (!serverPtr || !getCreatureByGameObjectID) {
        debugLog("[CServerExoApp] Error: no serverPtr or no getCreatureByGameObjectID");
        return nullptr;
    }

    void* creaturePtr = getCreatureByGameObjectID(serverPtr, objectId);
    if (!creaturePtr) {
        debugLog("[CServerExoApp] Error: Bad creaturePtr");
        return nullptr;
    }

    return new CSWSCreature(creaturePtr);
}
