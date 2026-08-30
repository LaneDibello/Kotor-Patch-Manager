#include "CServerExoApp.h"
#include "CAppManager.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CSWSCreature.h"
#include "CWorldTimer.h"
#include "CGameObject.h"

CServerExoApp::GetObjectArrayFn CServerExoApp::getObjectArray = nullptr;
CServerExoApp::GetPlayerCreatureIdFn CServerExoApp::getPlayerCreatureId = nullptr;
CServerExoApp::GetCreatureByGameObjectIDFn CServerExoApp::getCreatureByGameObjectID = nullptr;
CServerExoApp::GetPlayerCreatureFn CServerExoApp::getPlayerCreature = nullptr;
CServerExoApp::GetGlobalVariableTableFn CServerExoApp::getGlobalVariableTable = nullptr;
CServerExoApp::ClientToServerObjectIdFn CServerExoApp::clientToServerObjectId = nullptr;
CServerExoApp::GetWorldTimerFn CServerExoApp::getWorldTimer = nullptr;
CServerExoApp::GetActiveTimerFn CServerExoApp::getActiveTimer = nullptr;
CServerExoApp::GetGameObjectFn CServerExoApp::getGameObject = nullptr;
bool CServerExoApp::functionsInitialized = false;
bool CServerExoApp::offsetsInitialized = false;

void** appManagerGlobalPtr = nullptr;

void CServerExoApp::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CServerExoApp] ERROR: GameVersion not initialized\n");
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

        getPlayerCreature = reinterpret_cast<GetPlayerCreatureFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetPlayerCreature")
        );

        getGlobalVariableTable = reinterpret_cast<GetGlobalVariableTableFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetGlobalVariableTable")
        );

        clientToServerObjectId = reinterpret_cast<ClientToServerObjectIdFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "ClientToServerObjectId")
        );

        getWorldTimer = reinterpret_cast<GetWorldTimerFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetWorldTimer")
        );

        getActiveTimer = reinterpret_cast<GetActiveTimerFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetActiveTimer")
        );

        getGameObject = reinterpret_cast<GetGameObjectFn>(
            GameVersion::GetFunctionAddress("CServerExoApp", "GetGameObject")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CServerExoApp] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CServerExoApp::InitializeOffsets() {
    // CServerExoApp has no offsets
    offsetsInitialized = true;
}

CServerExoApp* CServerExoApp::GetInstance() {
    CAppManager* appManager = CAppManager::GetInstance();
    if (!appManager) {
        OutputDebugStringA("[CServerExoApp] ERROR: Failed to get CAppManager instance\n");
        return nullptr;
    }

    CServerExoApp* server = appManager->GetServer();
    delete appManager;  // Clean up the temporary CAppManager instance

    return server;
}

CServerExoApp::CServerExoApp(void* serverPtr)
    : GameAPIObject(serverPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CServerExoApp::~CServerExoApp() {
    // Base class destructor handles objectPtr cleanup
}

void* CServerExoApp::GetObjectArray() {
    if (!objectPtr || !getObjectArray) {
        return nullptr;
    }

    return getObjectArray(objectPtr);
}

DWORD CServerExoApp::GetPlayerCreatureId() {
    if (!objectPtr || !getPlayerCreatureId) {
        return 0x7F000000;
    }

    return getPlayerCreatureId(objectPtr);
}

CSWSCreature* CServerExoApp::GetCreatureByGameObjectID(DWORD objectId) {
    if (!objectPtr || !getCreatureByGameObjectID) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getCreatureByGameObjectID");
        return nullptr;
    }

    void* creaturePtr = getCreatureByGameObjectID(objectPtr, objectId);
    if (!creaturePtr) {
        debugLog("[CServerExoApp] Error: Bad creaturePtr");
        return nullptr;
    }

    return new CSWSCreature(creaturePtr);
}

CSWSCreature* CServerExoApp::GetPlayerCreature() {
    if (!objectPtr || !getPlayerCreature) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getPlayerCreature");
        return nullptr;
    }

    void* creaturePtr = getPlayerCreature(objectPtr);
    if (!creaturePtr) {
        debugLog("[CServerExoApp] Error: Bad creaturePtr");
        return nullptr;
    }

    return new CSWSCreature(creaturePtr);
}

void* CServerExoApp::GetGlobalVariableTable() {
    if (!objectPtr || !getGlobalVariableTable) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getCreatureByGameObjectID");
        return nullptr;
    }

    return getGlobalVariableTable(objectPtr);
}

DWORD CServerExoApp::ClientToServerObjectId(DWORD clientId) {
    if (!objectPtr || !clientToServerObjectId) {
        debugLog("[CServerExoApp] Error: no objectPtr or no clientToServerObjectId");
        return 0x7F000000;
    }

    return clientToServerObjectId(objectPtr, clientId);
}

CWorldTimer* CServerExoApp::GetWorldTimer() {
    if (!objectPtr || !getWorldTimer) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getWorldTimer");
        return nullptr;
    }

    void* worldTimerPtr = getWorldTimer(objectPtr);
    if (!worldTimerPtr) {
        debugLog("[CServerExoApp] Error: Bad worldTimerPtr");
        return nullptr;
    }

    return new CWorldTimer(worldTimerPtr);
}

CWorldTimer* CServerExoApp::GetActiveTimer(int objectId) {
    if (!objectPtr || !getActiveTimer) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getActiveTimer");
        return nullptr;
    }

    void* worldTimerPtr = getActiveTimer(objectPtr, objectId);
    if (!worldTimerPtr) {
        debugLog("[CServerExoApp] Error: Bad worldTimerPtr");
        return nullptr;
    }

    return new CWorldTimer(worldTimerPtr);
}

CGameObject* CServerExoApp::GetGameObject(DWORD objectId) {
    if (!objectPtr || !getGameObject) {
        debugLog("[CServerExoApp] Error: no objectPtr or no getGameObject");
        return nullptr;
    }

    void* gameObjectPtr = getGameObject(objectPtr, objectId);
    if (!gameObjectPtr) {
        debugLog("[CServerExoApp] Error: Bad gameObjectPtr");
        return nullptr;
    }

    return new CGameObject(gameObjectPtr);
}
