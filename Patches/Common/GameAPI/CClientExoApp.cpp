#include "CClientExoApp.h"
#include "CAppManager.h"
#include "GameVersion.h"
#include "../Common.h"

CClientExoApp::GetClientOptionsFn CClientExoApp::getClientOptions = nullptr;
bool CClientExoApp::functionsInitialized = false;
bool CClientExoApp::offsetsInitialized = false;

void CClientExoApp::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CClientExoApp] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getClientOptions = reinterpret_cast<GetClientOptionsFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetClientOptions")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CClientExoApp] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CClientExoApp::InitializeOffsets() {
    // CClientExoApp has no offsets
    offsetsInitialized = true;
}

CClientExoApp* CClientExoApp::GetInstance() {
    CAppManager* appManager = CAppManager::GetInstance();
    if (!appManager) {
        OutputDebugStringA("[CClientExoApp] ERROR: Failed to get CAppManager instance\n");
        return nullptr;
    }

    CClientExoApp* client = appManager->GetClient();
    delete appManager;  // Clean up the temporary CAppManager instance

    return client;
}

CClientExoApp::CClientExoApp(void* clientPtr)
    : GameAPIObject(clientPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CClientExoApp::~CClientExoApp() {
    // Base class destructor handles objectPtr cleanup
}

CClientOptions* CClientExoApp::GetClientOptions() {
    if (!objectPtr || !getClientOptions) {
        return nullptr;
    }

    void* clientOptionsPtr = getClientOptions(objectPtr);

    if (clientOptionsPtr)
        return new CClientOptions(clientOptionsPtr);

    debugLog("[CClientExoApp] ERROR: Failed to get CClientOptions");
    return nullptr;
}
