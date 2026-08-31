#include "CSWSAreaOfEffectObject.h"
#include "CExoString.h"
#include "GameVersion.h"

int CSWSAreaOfEffectObject::offsetShape = -1;
int CSWSAreaOfEffectObject::offsetSpellId = -1;
int CSWSAreaOfEffectObject::offsetCorners = -1;
int CSWSAreaOfEffectObject::offsetScriptOnHeartbeat = -1;
int CSWSAreaOfEffectObject::offsetScriptOnUserDefined = -1;
int CSWSAreaOfEffectObject::offsetScriptOnEnter = -1;
int CSWSAreaOfEffectObject::offsetScriptOnExit = -1;
int CSWSAreaOfEffectObject::offsetDurationType = -1;

bool CSWSAreaOfEffectObject::functionsInitialized = false;
bool CSWSAreaOfEffectObject::offsetsInitialized = false;

void CSWSAreaOfEffectObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSAreaOfEffectObject functions wrapped yet
    functionsInitialized = true;
}

void CSWSAreaOfEffectObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSAreaOfEffectObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetShape = GameVersion::GetOffset("CSWSAreaOfEffectObject", "shape");
        offsetSpellId = GameVersion::GetOffset("CSWSAreaOfEffectObject", "spell_id");
        offsetCorners = GameVersion::GetOffset("CSWSAreaOfEffectObject", "corners");
        offsetScriptOnHeartbeat = GameVersion::GetOffset("CSWSAreaOfEffectObject", "script_on_heartbeat");
        offsetScriptOnUserDefined = GameVersion::GetOffset("CSWSAreaOfEffectObject", "script_on_user_defined");
        offsetScriptOnEnter = GameVersion::GetOffset("CSWSAreaOfEffectObject", "script_on_enter");
        offsetScriptOnExit = GameVersion::GetOffset("CSWSAreaOfEffectObject", "script_on_exit");
        offsetDurationType = GameVersion::GetOffset("CSWSAreaOfEffectObject", "duration_type");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSAreaOfEffectObject] ERROR: %s\n", e.what());
    }
}

CSWSAreaOfEffectObject::CSWSAreaOfEffectObject(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSAreaOfEffectObject::~CSWSAreaOfEffectObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

BYTE CSWSAreaOfEffectObject::GetShape() {
    if (!objectPtr || offsetShape < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetShape);
}

void CSWSAreaOfEffectObject::SetShape(BYTE value) {
    if (!objectPtr || offsetShape < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetShape, value);
}

DWORD CSWSAreaOfEffectObject::GetSpellId() {
    if (!objectPtr || offsetSpellId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetSpellId);
}

void CSWSAreaOfEffectObject::SetSpellId(DWORD value) {
    if (!objectPtr || offsetSpellId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetSpellId, value);
}

Vector* CSWSAreaOfEffectObject::GetCorners() {
    if (!objectPtr || offsetCorners < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetCorners);
}

void CSWSAreaOfEffectObject::SetCorners(Vector* value) {
    if (!objectPtr || offsetCorners < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetCorners, value);
}

CExoString* CSWSAreaOfEffectObject::GetScriptOnHeartbeat() {
    if (!objectPtr || offsetScriptOnHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnHeartbeat);
}

CExoString* CSWSAreaOfEffectObject::GetScriptOnUserDefined() {
    if (!objectPtr || offsetScriptOnUserDefined < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUserDefined);
}

CExoString* CSWSAreaOfEffectObject::GetScriptOnEnter() {
    if (!objectPtr || offsetScriptOnEnter < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnEnter);
}

CExoString* CSWSAreaOfEffectObject::GetScriptOnExit() {
    if (!objectPtr || offsetScriptOnExit < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnExit);
}

BYTE CSWSAreaOfEffectObject::GetDurationType() {
    if (!objectPtr || offsetDurationType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetDurationType);
}

void CSWSAreaOfEffectObject::SetDurationType(BYTE value) {
    if (!objectPtr || offsetDurationType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetDurationType, value);
}
