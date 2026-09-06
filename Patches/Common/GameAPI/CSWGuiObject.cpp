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
    // Catches wrappers that do not free the game object themselves; the ones that
    // do have already called RestoreVTable() from their own destructor.
    // Base class destructor handles objectPtr cleanup.
    RestoreVTable();
}

void CSWGuiObject::RestoreVTable() {
    if (vtableOverride) {
        delete vtableOverride;
        vtableOverride = nullptr;
    }
}

int CSWGuiObject::VTableSlotCount() {
    // No layout of our own; a class that wants overrides must report its count.
    debugLog("[CSWGuiObject] WARNING: vtable layout unknown for this class; vtable overriding disabled\n");
    return -1;
}

bool CSWGuiObject::EnsureVTableOverride() {
    if (vtableOverride) {
        return vtableOverride->IsActive();
    }
    if (!objectPtr) {
        return false;
    }

    int count = VTableSlotCount();
    if (count < 0) {
        return false;  // Unsupported version/class; overriding disabled (already logged).
    }

    vtableOverride = new VTableOverride(objectPtr, this, count);
    if (!vtableOverride->IsActive()) {
        debugLog("[CSWGuiObject] ERROR: failed to install vtable override\n");
        delete vtableOverride;
        vtableOverride = nullptr;
        return false;
    }
    return true;
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
