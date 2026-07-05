#include "CSWGui3DSceneView.h"
#include "GameVersion.h"
#include "CSWGuiScene.h"

bool CSWGui3DSceneView::functionsInitialized = false;
bool CSWGui3DSceneView::offsetsInitialized = false;

int CSWGui3DSceneView::offsetScene = -1;

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
        offsetScene = GameVersion::GetOffset("CSWGui3DSceneView", "scene");

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

CSWGuiScene* CSWGui3DSceneView::GetScene() {
    if (!objectPtr || offsetScene < 0) {
        return nullptr;
    }
    // scene is an embedded CSWGuiScene sub-object, not a pointer.
    return new CSWGuiScene((char*)objectPtr + offsetScene);
}
