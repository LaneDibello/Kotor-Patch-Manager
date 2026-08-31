#include "CSWSTrigger.h"
#include "CExoString.h"
#include "GameVersion.h"
#include "CExoLocString.h"

CSWSTrigger::GetCanFireMineOnObjectFn CSWSTrigger::getCanFireMineOnObject = nullptr;
CSWSTrigger::OnEnterMineFn CSWSTrigger::onEnterMine = nullptr;
CSWSTrigger::GetFirstNameFn CSWSTrigger::getFirstName = nullptr;
CSWSTrigger::GetTargetAreaFn CSWSTrigger::getTargetArea = nullptr;
CSWSTrigger::InTriggerFn CSWSTrigger::inTrigger = nullptr;
CSWSTrigger::SetCreatorFn CSWSTrigger::setCreator = nullptr;
CSWSTrigger::RemoveFromAreaFn CSWSTrigger::removeFromArea = nullptr;

int CSWSTrigger::offsetLocalizedName = -1;
int CSWSTrigger::offsetLinkedTo = -1;
int CSWSTrigger::offsetLinkedToModule = -1;
int CSWSTrigger::offsetScriptHeartbeat = -1;
int CSWSTrigger::offsetScriptOnEnter = -1;
int CSWSTrigger::offsetScriptOnExit = -1;
int CSWSTrigger::offsetScriptUserDefined = -1;
int CSWSTrigger::offsetScriptOnTrapTriggered = -1;
int CSWSTrigger::offsetScriptOnDisarm = -1;
int CSWSTrigger::offsetScriptOnClick = -1;
int CSWSTrigger::offsetKeyName = -1;
int CSWSTrigger::offsetGeometryCount = -1;
int CSWSTrigger::offsetGeometry = -1;
int CSWSTrigger::offsetGeometryIndices = -1;
int CSWSTrigger::offsetFaction = -1;
int CSWSTrigger::offsetTrapDetectable = -1;
int CSWSTrigger::offsetTrapDisarmable = -1;
int CSWSTrigger::offsetTrapType = -1;
int CSWSTrigger::offsetBBoxMinX = -1;
int CSWSTrigger::offsetBBoxMinY = -1;
int CSWSTrigger::offsetBBoxMaxX = -1;
int CSWSTrigger::offsetBBoxMaxY = -1;
int CSWSTrigger::offsetCursor = -1;
int CSWSTrigger::offsetLoadScreenId = -1;
int CSWSTrigger::offsetTransitionDestination = -1;

bool CSWSTrigger::functionsInitialized = false;
bool CSWSTrigger::offsetsInitialized = false;

void CSWSTrigger::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSTrigger] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getCanFireMineOnObject = reinterpret_cast<GetCanFireMineOnObjectFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetCanFireMineOnObject"));
        onEnterMine = reinterpret_cast<OnEnterMineFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "OnEnterMine"));
        getFirstName = reinterpret_cast<GetFirstNameFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetFirstName"));
        getTargetArea = reinterpret_cast<GetTargetAreaFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetTargetArea"));
        inTrigger = reinterpret_cast<InTriggerFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "InTrigger"));
        setCreator = reinterpret_cast<SetCreatorFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "SetCreator"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "RemoveFromArea"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSTrigger] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSTrigger::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSTrigger] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetLocalizedName = GameVersion::GetOffset("CSWSTrigger", "localized_name");
        offsetLinkedTo = GameVersion::GetOffset("CSWSTrigger", "linked_to");
        offsetLinkedToModule = GameVersion::GetOffset("CSWSTrigger", "linked_to_module");
        offsetScriptHeartbeat = GameVersion::GetOffset("CSWSTrigger", "script_heartbeat");
        offsetScriptOnEnter = GameVersion::GetOffset("CSWSTrigger", "script_on_enter");
        offsetScriptOnExit = GameVersion::GetOffset("CSWSTrigger", "script_on_exit");
        offsetScriptUserDefined = GameVersion::GetOffset("CSWSTrigger", "script_user_defined");
        offsetScriptOnTrapTriggered = GameVersion::GetOffset("CSWSTrigger", "script_on_trap_triggered");
        offsetScriptOnDisarm = GameVersion::GetOffset("CSWSTrigger", "script_on_disarm");
        offsetScriptOnClick = GameVersion::GetOffset("CSWSTrigger", "script_on_click");
        offsetKeyName = GameVersion::GetOffset("CSWSTrigger", "key_name");
        offsetGeometryCount = GameVersion::GetOffset("CSWSTrigger", "geometry_count");
        offsetGeometry = GameVersion::GetOffset("CSWSTrigger", "geometry");
        offsetGeometryIndices = GameVersion::GetOffset("CSWSTrigger", "geometry_indices");
        offsetFaction = GameVersion::GetOffset("CSWSTrigger", "faction");
        offsetTrapDetectable = GameVersion::GetOffset("CSWSTrigger", "trap_detectable");
        offsetTrapDisarmable = GameVersion::GetOffset("CSWSTrigger", "trap_disarmable");
        offsetTrapType = GameVersion::GetOffset("CSWSTrigger", "trap_type");
        offsetBBoxMinX = GameVersion::GetOffset("CSWSTrigger", "bbox_min_x");
        offsetBBoxMinY = GameVersion::GetOffset("CSWSTrigger", "bbox_min_y");
        offsetBBoxMaxX = GameVersion::GetOffset("CSWSTrigger", "bbox_max_x");
        offsetBBoxMaxY = GameVersion::GetOffset("CSWSTrigger", "bbox_max_y");
        offsetCursor = GameVersion::GetOffset("CSWSTrigger", "cursor");
        offsetLoadScreenId = GameVersion::GetOffset("CSWSTrigger", "load_screen_id");
        offsetTransitionDestination = GameVersion::GetOffset("CSWSTrigger", "transition_destination");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSTrigger] ERROR: %s\n", e.what());
    }
}

CSWSTrigger::CSWSTrigger(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSTrigger::~CSWSTrigger() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

int CSWSTrigger::GetCanFireMineOnObject(DWORD objectId, int skipFactionCheck) {
    if (!objectPtr || !getCanFireMineOnObject) {
        return 0;
    }
    return getCanFireMineOnObject(objectPtr, objectId, skipFactionCheck);
}

void CSWSTrigger::OnEnterMine(int skipFactionCheck) {
    if (!objectPtr || !onEnterMine) {
        return;
    }
    onEnterMine(objectPtr, skipFactionCheck);
}

CExoLocString* CSWSTrigger::GetFirstName() {
    if (!objectPtr || !getFirstName) {
        return nullptr;
    }

    void* resultPtr = getFirstName(objectPtr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoLocString(resultPtr);
}

DWORD CSWSTrigger::GetTargetArea() {
    if (!objectPtr || !getTargetArea) {
        return OBJECT_DEFAULT;
    }
    return getTargetArea(objectPtr);
}

int CSWSTrigger::InTrigger(Vector* point) {
    if (!objectPtr || !inTrigger) {
        return 0;
    }
    return inTrigger(objectPtr, point);
}

void CSWSTrigger::SetCreator(DWORD creatorId) {
    if (!objectPtr || !setCreator) {
        return;
    }
    setCreator(objectPtr, creatorId);
}

void CSWSTrigger::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}

// ===== Offsets =====

CExoLocString* CSWSTrigger::GetLocalizedName() {
    if (!objectPtr || offsetLocalizedName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocalizedName);
}

CExoString* CSWSTrigger::GetLinkedTo() {
    if (!objectPtr || offsetLinkedTo < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetLinkedTo);
}

CExoString* CSWSTrigger::GetLinkedToModule() {
    if (!objectPtr || offsetLinkedToModule < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetLinkedToModule);
}

CExoString* CSWSTrigger::GetScriptHeartbeat() {
    if (!objectPtr || offsetScriptHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptHeartbeat);
}

CExoString* CSWSTrigger::GetScriptOnEnter() {
    if (!objectPtr || offsetScriptOnEnter < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnEnter);
}

CExoString* CSWSTrigger::GetScriptOnExit() {
    if (!objectPtr || offsetScriptOnExit < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnExit);
}

CExoString* CSWSTrigger::GetScriptUserDefined() {
    if (!objectPtr || offsetScriptUserDefined < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptUserDefined);
}

CExoString* CSWSTrigger::GetScriptOnTrapTriggered() {
    if (!objectPtr || offsetScriptOnTrapTriggered < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnTrapTriggered);
}

CExoString* CSWSTrigger::GetScriptOnDisarm() {
    if (!objectPtr || offsetScriptOnDisarm < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDisarm);
}

CExoString* CSWSTrigger::GetScriptOnClick() {
    if (!objectPtr || offsetScriptOnClick < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnClick);
}

CExoString* CSWSTrigger::GetKeyName() {
    if (!objectPtr || offsetKeyName < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetKeyName);
}

int CSWSTrigger::GetGeometryCount() {
    if (!objectPtr || offsetGeometryCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetGeometryCount);
}

void CSWSTrigger::SetGeometryCount(int value) {
    if (!objectPtr || offsetGeometryCount < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetGeometryCount, value);
}

Vector* CSWSTrigger::GetGeometry() {
    if (!objectPtr || offsetGeometry < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetGeometry);
}

void CSWSTrigger::SetGeometry(Vector* value) {
    if (!objectPtr || offsetGeometry < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetGeometry, value);
}

DWORD* CSWSTrigger::GetGeometryIndices() {
    if (!objectPtr || offsetGeometryIndices < 0) {
        return nullptr;
    }
    return getObjectProperty<DWORD*>(objectPtr, offsetGeometryIndices);
}

void CSWSTrigger::SetGeometryIndices(DWORD* value) {
    if (!objectPtr || offsetGeometryIndices < 0) {
        return;
    }
    setObjectProperty<DWORD*>(objectPtr, offsetGeometryIndices, value);
}

DWORD CSWSTrigger::GetFaction() {
    if (!objectPtr || offsetFaction < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetFaction);
}

void CSWSTrigger::SetFaction(DWORD value) {
    if (!objectPtr || offsetFaction < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetFaction, value);
}

DWORD CSWSTrigger::GetTrapDetectable() {
    if (!objectPtr || offsetTrapDetectable < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetTrapDetectable);
}

void CSWSTrigger::SetTrapDetectable(DWORD value) {
    if (!objectPtr || offsetTrapDetectable < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetTrapDetectable, value);
}

DWORD CSWSTrigger::GetTrapDisarmable() {
    if (!objectPtr || offsetTrapDisarmable < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetTrapDisarmable);
}

void CSWSTrigger::SetTrapDisarmable(DWORD value) {
    if (!objectPtr || offsetTrapDisarmable < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetTrapDisarmable, value);
}

BYTE CSWSTrigger::GetTrapType() {
    if (!objectPtr || offsetTrapType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetTrapType);
}

void CSWSTrigger::SetTrapType(BYTE value) {
    if (!objectPtr || offsetTrapType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetTrapType, value);
}

float CSWSTrigger::GetBBoxMinX() {
    if (!objectPtr || offsetBBoxMinX < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetBBoxMinX);
}

void CSWSTrigger::SetBBoxMinX(float value) {
    if (!objectPtr || offsetBBoxMinX < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetBBoxMinX, value);
}

float CSWSTrigger::GetBBoxMinY() {
    if (!objectPtr || offsetBBoxMinY < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetBBoxMinY);
}

void CSWSTrigger::SetBBoxMinY(float value) {
    if (!objectPtr || offsetBBoxMinY < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetBBoxMinY, value);
}

float CSWSTrigger::GetBBoxMaxX() {
    if (!objectPtr || offsetBBoxMaxX < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetBBoxMaxX);
}

void CSWSTrigger::SetBBoxMaxX(float value) {
    if (!objectPtr || offsetBBoxMaxX < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetBBoxMaxX, value);
}

float CSWSTrigger::GetBBoxMaxY() {
    if (!objectPtr || offsetBBoxMaxY < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetBBoxMaxY);
}

void CSWSTrigger::SetBBoxMaxY(float value) {
    if (!objectPtr || offsetBBoxMaxY < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetBBoxMaxY, value);
}

BYTE CSWSTrigger::GetCursor() {
    if (!objectPtr || offsetCursor < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetCursor);
}

void CSWSTrigger::SetCursor(BYTE value) {
    if (!objectPtr || offsetCursor < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetCursor, value);
}

WORD CSWSTrigger::GetLoadScreenId() {
    if (!objectPtr || offsetLoadScreenId < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetLoadScreenId);
}

void CSWSTrigger::SetLoadScreenId(WORD value) {
    if (!objectPtr || offsetLoadScreenId < 0) {
        return;
    }
    setObjectProperty<WORD>(objectPtr, offsetLoadScreenId, value);
}

CExoLocString* CSWSTrigger::GetTransitionDestination() {
    if (!objectPtr || offsetTransitionDestination < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetTransitionDestination);
}
