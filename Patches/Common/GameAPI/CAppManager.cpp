#include "CAppManager.h"
#include "CServerExoApp.h"
#include "CClientExoApp.h"
#include "GameVersion.h"
#include "../Common.h"

int CAppManager::offsetClient = -1;
int CAppManager::offsetServer = -1;
bool CAppManager::offsetsInitialized = false;

extern void** appManagerGlobalPtr;

void CAppManager::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CAppManager] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetClient = GameVersion::GetOffset("CAppManager", "Client");
        offsetServer = GameVersion::GetOffset("CAppManager", "Server");
        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CAppManager] ERROR: %s\n", e.what());
    }
}

CAppManager* CAppManager::GetInstance() {
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    if (!appManagerGlobalPtr || !*appManagerGlobalPtr) {
        OutputDebugStringA("[CAppManager] ERROR: APP_MANAGER_PTR is null\n");
        return nullptr;
    }

    void* appManager = *appManagerGlobalPtr;
    if (!appManager) {
        return nullptr;
    }

    return new CAppManager(appManager);
}

CAppManager::CAppManager(void* appManagerPtr)
    : appManagerPtr(appManagerPtr)
{
}

CAppManager::~CAppManager() {
    appManagerPtr = nullptr;
}

CServerExoApp* CAppManager::GetServer() {
    if (!appManagerPtr || offsetServer < 0) {
        return nullptr;
    }

    void* serverPtr = getObjectProperty<void*>(appManagerPtr, offsetServer);
    if (!serverPtr) {
        return nullptr;
    }

    return new CServerExoApp(serverPtr);
}

CClientExoApp* CAppManager::GetClient() {
    if (!appManagerPtr || offsetClient < 0) {
        return nullptr;
    }

    void* clientPtr = getObjectProperty<void*>(appManagerPtr, offsetClient);
    if (!clientPtr) {
        return nullptr;
    }

    return new CClientExoApp(clientPtr);
}
