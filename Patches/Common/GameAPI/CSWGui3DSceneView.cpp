#include "CSWGui3DSceneView.h"
#include "GameVersion.h"
#include "CSWGuiScene.h"

CSWGui3DSceneView::DestructorFn CSWGui3DSceneView::destructor = nullptr;

bool CSWGui3DSceneView::functionsInitialized = false;
bool CSWGui3DSceneView::offsetsInitialized = false;

int CSWGui3DSceneView::offsetScene = -1;
int CSWGui3DSceneView::classSize = -1;
void* CSWGui3DSceneView::vtable = nullptr;

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
        // Note: CSWGui3DSceneView has no Constructor entry (inlined to the
        // CSWGuiControl constructor); the default constructor reuses that one.
        destructor = reinterpret_cast<DestructorFn>(GameVersion::GetFunctionAddress("CSWGui3DSceneView", "Destructor"));

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
        classSize = GameVersion::GetClassSize("CSWGui3DSceneView");
        vtable = GameVersion::GetClassVtable("CSWGui3DSceneView"); // may be null; non-throwing

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

CSWGui3DSceneView::CSWGui3DSceneView()
    : CSWGuiControl(nullptr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // The 3D scene view has no constructor of its own; the game builds one by
    // calling the CSWGuiControl constructor over a larger (classSize) allocation.
    if (classSize > 0 && CSWGuiControl::constructor) {
        objectPtr = malloc(classSize);
        if (objectPtr) {
            CSWGuiControl::constructor(objectPtr);
            shouldFree = true;

            // The control constructor stamped the CSWGuiControl vtable at offset
            // 0. The original (inlined) CSWGui3DSceneView constructor then
            // overwrote it with the 3D-scene-view vtable; replicate that here.
            // Skip if unavailable (older DB), leaving the control vtable in place.
            if (vtable) {
                setObjectProperty<void*>(objectPtr, 0, vtable);
            }

            // In the disassembly the control constructor is immediately followed
            // by the embedded scene's constructor -- both lived in the original
            // (inlined) CSWGui3DSceneView constructor. Replicate that by
            // constructing the scene field in place (its own constructor stamps
            // the embedded scene's vtable).
            if (offsetScene >= 0) {
                CSWGuiScene scene((char*)objectPtr + offsetScene);
                scene.Construct();
            }
        }
    }
}

CSWGui3DSceneView::~CSWGui3DSceneView()
{
    // Use the class's own destructor (it tears down the embedded scene) rather
    // than deferring to CSWGuiControl. Clearing shouldFree stops the base
    // destructor from freeing a second time.
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

CSWGuiScene* CSWGui3DSceneView::GetScene() {
    if (!objectPtr || offsetScene < 0) {
        return nullptr;
    }
    // scene is an embedded CSWGuiScene sub-object, not a pointer.
    return new CSWGuiScene((char*)objectPtr + offsetScene);
}
