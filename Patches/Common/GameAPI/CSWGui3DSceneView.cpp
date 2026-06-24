#include "CSWGui3DSceneView.h"
#include "GameVersion.h"

bool CSWGui3DSceneView::functionsInitialized = false;
bool CSWGui3DSceneView::offsetsInitialized = false;

void CSWGui3DSceneView::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGui3DSceneView] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGui3DSceneView] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGui3DSceneView::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGui3DSceneView] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGui3DSceneView] ERROR: %s\n", e.what());
    }
}

CSWGui3DSceneView::CSWGui3DSceneView(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGui3DSceneView::~CSWGui3DSceneView()
{
    // Base class destructor handles objectPtr cleanup
}
