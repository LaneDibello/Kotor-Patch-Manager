#include "CSWGuiObject.h"

bool CSWGuiObject::functionsInitialized = false;
bool CSWGuiObject::offsetsInitialized = false;

int CSWGuiObject::offsetExtent = -1;


void CSWGuiObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiObject] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetExtent = GameVersion::GetOffset("CSWGuiObject", "extent");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiObject] ERROR: %s\n", e.what());
    }
}

CSWGuiObject::CSWGuiObject(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    InitializeFunctions();
    InitializeOffsets();
}

CSWGuiObject::~CSWGuiObject()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiExtent CSWGuiObject::GetExtent() {
    CSWGuiExtent result = {0, 0, 0, 0};
    if (!objectPtr || offsetExtent < 0) {
        return result;
    }
    return getObjectProperty<CSWGuiExtent>(objectPtr, offsetExtent);
}

void CSWGuiObject::SetExtent(const CSWGuiExtent& extent) {
    if (!objectPtr || offsetExtent < 0) {
        return;
    }
    setObjectProperty<CSWGuiExtent>(objectPtr, offsetExtent, extent);
}
