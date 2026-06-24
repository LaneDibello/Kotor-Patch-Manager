#include "CSWGuiControl.h"
#include "GameVersion.h"

bool CSWGuiControl::functionsInitialized = false;
bool CSWGuiControl::offsetsInitialized = false;

int CSWGuiControl::offsetParentControl = -1;

void CSWGuiControl::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiControl] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiControl] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiControl::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiControl] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetParentControl = GameVersion::GetOffset("CSWGuiControl", "parent_control");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiControl] ERROR: %s\n", e.what());
    }
}

CSWGuiControl::CSWGuiControl(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiControl::~CSWGuiControl()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiControl* CSWGuiControl::GetParentControl() {
    if (!objectPtr || offsetParentControl < 0) {
        return nullptr;
    }
    void* parentPtr = getObjectProperty<void*>(objectPtr, offsetParentControl);
    if (!parentPtr) {
        return nullptr;
    }
    return new CSWGuiControl(parentPtr);
}
