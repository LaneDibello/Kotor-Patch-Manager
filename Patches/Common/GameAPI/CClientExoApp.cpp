#include "CClientExoApp.h"
#include "GameVersion.h"
#include "../Common.h"

CClientExoApp::GetClientOptionsFn CClientExoApp::getClientOptions = nullptr;
bool CClientExoApp::functionsInitialized = false;

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

CClientExoApp* CClientExoApp::GetInstance() {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

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

        return new CClientExoApp(clientExoApp);
    }
    catch (const GameVersionException& e) {
        debugLog("[CClientExoApp] ERROR: %s\n", e.what());
        return nullptr;
    }
}

CClientExoApp::CClientExoApp(void* clientPtr)
    : clientPtr(clientPtr)
{
}

CClientExoApp::~CClientExoApp() {
    clientPtr = nullptr;
}

CClientOptions* CClientExoApp::GetClientOptions() {
    if (!clientPtr || !getClientOptions) {
        return nullptr;
    }

    void* clientOptionsPtr = getClientOptions(clientPtr);

    if (clientOptionsPtr)
        return new CClientOptions(clientOptionsPtr);

    debugLog("[CClientExoApp] ERROR: Failed to get CClientOptions");
    return nullptr;
}
