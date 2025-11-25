#include "CClientExoApp.h"
#include "GameVersion.h"
#include "../Common.h"

CClientExoApp::GetClientOptionsFn CClientExoApp::getClientOptions = nullptr;
bool CClientExoApp::functionsInitialized = false;
bool CClientExoApp::offsetsInitialized = false;

extern void** appManagerGlobalPtr;

void CClientExoApp::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CClientExoApp] ERROR: GameVersion not initialized\n");
        return;
    }

    appManagerGlobalPtr = static_cast<void**>(GameVersion::GetGlobalPointer("APP_MANAGER_PTR"));
    if (!appManagerGlobalPtr) {
        OutputDebugStringA("[CServerExoApp] ERROR: APP_MANAGER_PTR not found\n");
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
    if (!appManagerGlobalPtr || !*appManagerGlobalPtr) {
        OutputDebugStringA("[CClientExoApp] ERROR: App manager pointer is null\n");
        return nullptr;
    }

    void* appManager = *appManagerGlobalPtr;

    try {
        int clientOffset = GameVersion::GetOffset("CAppManager", "Client");
        void* clientExoApp = getObjectProperty<void*>(appManager, clientOffset);

        if (!clientExoApp) {
            OutputDebugStringA("[CClientExoApp] ERROR: CClientExoApp pointer is null\n");
            return nullptr;
        }

        // Initialization will happen in constructor
        return new CClientExoApp(clientExoApp);
    }
    catch (const GameVersionException& e) {
        debugLog("[CClientExoApp] ERROR: %s\n", e.what());
        return nullptr;
    }
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
