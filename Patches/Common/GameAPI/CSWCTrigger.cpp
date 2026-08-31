#include "CSWCTrigger.h"
#include "CExoString.h"
#include "GameVersion.h"

int CSWCTrigger::offsetName = -1;
int CSWCTrigger::offsetCursorId = -1;
int CSWCTrigger::offsetGeometry = -1;

bool CSWCTrigger::functionsInitialized = false;
bool CSWCTrigger::offsetsInitialized = false;

void CSWCTrigger::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCTrigger functions wrapped yet
    functionsInitialized = true;
}

void CSWCTrigger::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCTrigger] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetName = GameVersion::GetOffset("CSWCTrigger", "name");
        offsetCursorId = GameVersion::GetOffset("CSWCTrigger", "cursor_id");
        offsetGeometry = GameVersion::GetOffset("CSWCTrigger", "geometry");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCTrigger] ERROR: %s\n", e.what());
    }
}

CSWCTrigger::CSWCTrigger(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCTrigger::~CSWCTrigger() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

CExoString* CSWCTrigger::GetName() {
    if (!objectPtr || offsetName < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetName);
}

int CSWCTrigger::GetCursorId() {
    if (!objectPtr || offsetCursorId < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCursorId);
}

void CSWCTrigger::SetCursorId(int value) {
    if (!objectPtr || offsetCursorId < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCursorId, value);
}

Vector* CSWCTrigger::GetGeometry() {
    if (!objectPtr || offsetGeometry < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetGeometry);
}

void CSWCTrigger::SetGeometry(Vector* value) {
    if (!objectPtr || offsetGeometry < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetGeometry, value);
}
