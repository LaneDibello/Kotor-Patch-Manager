#include "CSWSWaypoint.h"
#include "CExoLocString.h"
#include "GameVersion.h"

int CSWSWaypoint::offsetHasMapNote = -1;
int CSWSWaypoint::offsetMapNoteEnabled = -1;
int CSWSWaypoint::offsetMapNote = -1;
int CSWSWaypoint::offsetLocalizedName = -1;

bool CSWSWaypoint::functionsInitialized = false;
bool CSWSWaypoint::offsetsInitialized = false;

void CSWSWaypoint::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSWaypoint functions wrapped yet
    functionsInitialized = true;
}

void CSWSWaypoint::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSWaypoint] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetHasMapNote = GameVersion::GetOffset("CSWSWaypoint", "has_map_note");
        offsetMapNoteEnabled = GameVersion::GetOffset("CSWSWaypoint", "map_note_enabled");
        offsetMapNote = GameVersion::GetOffset("CSWSWaypoint", "map_note");
        offsetLocalizedName = GameVersion::GetOffset("CSWSWaypoint", "localized_name");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSWaypoint] ERROR: %s\n", e.what());
    }
}

CSWSWaypoint::CSWSWaypoint(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSWaypoint::~CSWSWaypoint() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

int CSWSWaypoint::GetHasMapNote() {
    if (!objectPtr || offsetHasMapNote < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHasMapNote);
}

void CSWSWaypoint::SetHasMapNote(int value) {
    if (!objectPtr || offsetHasMapNote < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHasMapNote, value);
}

int CSWSWaypoint::GetMapNoteEnabled() {
    if (!objectPtr || offsetMapNoteEnabled < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMapNoteEnabled);
}

void CSWSWaypoint::SetMapNoteEnabled(int value) {
    if (!objectPtr || offsetMapNoteEnabled < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMapNoteEnabled, value);
}

CExoLocString* CSWSWaypoint::GetMapNote() {
    if (!objectPtr || offsetMapNote < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetMapNote);
}

CExoLocString* CSWSWaypoint::GetLocalizedName() {
    if (!objectPtr || offsetLocalizedName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocalizedName);
}
