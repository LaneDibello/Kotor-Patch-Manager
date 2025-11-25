#include "CAppManager.h"
#include "CServerExoApp.h"
#include "CClientExoApp.h"
#include "GameVersion.h"
#include "../Common.h"

bool CAppManager::functionsInitialized = false;
int CAppManager::offsetClient = -1;
int CAppManager::offsetServer = -1;
bool CAppManager::offsetsInitialized = false;

extern void** appManagerGlobalPtr;

void CAppManager::InitializeFunctions() {
    // CAppManager has no function pointers
    functionsInitialized = true;
}

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
    if (!appManagerGlobalPtr || !*appManagerGlobalPtr) {
        OutputDebugStringA("[CAppManager] ERROR: APP_MANAGER_PTR is null\n");
        return nullptr;
    }

    void* appManager = *appManagerGlobalPtr;
    if (!appManager) {
        return nullptr;
    }

    // Initialization will happen in constructor
    return new CAppManager(appManager);
}

CAppManager::CAppManager(void* appManagerPtr)
    : GameAPIObject(appManagerPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CAppManager::~CAppManager() {
    // Base class destructor handles objectPtr cleanup
}

CServerExoApp* CAppManager::GetServer() {
    if (!objectPtr || offsetServer < 0) {
        return nullptr;
    }

    void* serverPtr = getObjectProperty<void*>(objectPtr, offsetServer);
    if (!serverPtr) {
        return nullptr;
    }

    return new CServerExoApp(serverPtr);
}

CClientExoApp* CAppManager::GetClient() {
    if (!objectPtr || offsetClient < 0) {
        return nullptr;
    }

    void* clientPtr = getObjectProperty<void*>(objectPtr, offsetClient);
    if (!clientPtr) {
        return nullptr;
    }

    return new CClientExoApp(clientPtr);
}
