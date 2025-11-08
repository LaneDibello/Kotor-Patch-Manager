#include "CClientExoApp.h"
#include "GameVersion.h"
#include "../Common.h"

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
