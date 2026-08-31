#include "CSWSDoor.h"
#include "CExoString.h"
#include "GameVersion.h"
#include "CResRef.h"
#include "CExoLocString.h"

CSWSDoor::GetDialogResrefFn CSWSDoor::getDialogResref = nullptr;
CSWSDoor::GetFirstNameFn CSWSDoor::getFirstName = nullptr;
CSWSDoor::GetIsLinkedFn CSWSDoor::getIsLinked = nullptr;
CSWSDoor::GetLinkedObjectFn CSWSDoor::getLinkedObject = nullptr;
CSWSDoor::InDoorFn CSWSDoor::inDoor = nullptr;
CSWSDoor::MoveToNextOpenStateFn CSWSDoor::moveToNextOpenState = nullptr;
CSWSDoor::RemoveFromAreaFn CSWSDoor::removeFromArea = nullptr;

int CSWSDoor::offsetScriptOnOpen = -1;
int CSWSDoor::offsetScriptOnClosed = -1;
int CSWSDoor::offsetScriptOnDamaged = -1;
int CSWSDoor::offsetScriptOnDeath = -1;
int CSWSDoor::offsetScriptOnDisarm = -1;
int CSWSDoor::offsetScriptOnHeartbeat = -1;
int CSWSDoor::offsetScriptOnLock = -1;
int CSWSDoor::offsetScriptOnMeleeAttacked = -1;
int CSWSDoor::offsetScriptOnSpellCastAt = -1;
int CSWSDoor::offsetScriptOnTrapTriggered = -1;
int CSWSDoor::offsetScriptOnUnlock = -1;
int CSWSDoor::offsetScriptOnUserDefined = -1;
int CSWSDoor::offsetScriptOnClick = -1;
int CSWSDoor::offsetScriptOnDialog = -1;
int CSWSDoor::offsetScriptOnFailToOpen = -1;
int CSWSDoor::offsetAppearance = -1;
int CSWSDoor::offsetGenericType = -1;
int CSWSDoor::offsetConversation = -1;
int CSWSDoor::offsetFaction = -1;
int CSWSDoor::offsetFortitude = -1;
int CSWSDoor::offsetReflex = -1;
int CSWSDoor::offsetWill = -1;
int CSWSDoor::offsetOpenLockDC = -1;
int CSWSDoor::offsetCloseLockDC = -1;
int CSWSDoor::offsetSecretDoorDC = -1;
int CSWSDoor::offsetHardness = -1;
int CSWSDoor::offsetOpenState = -1;
int CSWSDoor::offsetKeyName = -1;
int CSWSDoor::offsetKeyRequired = -1;
int CSWSDoor::offsetDisarmDC = -1;
int CSWSDoor::offsetDetectDC = -1;
int CSWSDoor::offsetTrapType = -1;
int CSWSDoor::offsetCorner = -1;
int CSWSDoor::offsetLinkedToFlags = -1;
int CSWSDoor::offsetLinkedTo = -1;
int CSWSDoor::offsetLinkedToModule = -1;
int CSWSDoor::offsetLocName = -1;
int CSWSDoor::offsetDescription = -1;
int CSWSDoor::offsetLoadScreenIdLower = -1;
int CSWSDoor::offsetLoadScreenIdUpper = -1;
int CSWSDoor::offsetTransitionDestination = -1;

bool CSWSDoor::functionsInitialized = false;
bool CSWSDoor::offsetsInitialized = false;

void CSWSDoor::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSDoor] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getDialogResref = reinterpret_cast<GetDialogResrefFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetDialogResref"));
        getFirstName = reinterpret_cast<GetFirstNameFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetFirstName"));
        getIsLinked = reinterpret_cast<GetIsLinkedFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetIsLinked"));
        getLinkedObject = reinterpret_cast<GetLinkedObjectFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetLinkedObject"));
        inDoor = reinterpret_cast<InDoorFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "InDoor"));
        moveToNextOpenState = reinterpret_cast<MoveToNextOpenStateFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "MoveToNextOpenState"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "RemoveFromArea"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSDoor] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSDoor::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSDoor] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetScriptOnOpen = GameVersion::GetOffset("CSWSDoor", "script_on_open");
        offsetScriptOnClosed = GameVersion::GetOffset("CSWSDoor", "script_on_closed");
        offsetScriptOnDamaged = GameVersion::GetOffset("CSWSDoor", "script_on_damaged");
        offsetScriptOnDeath = GameVersion::GetOffset("CSWSDoor", "script_on_death");
        offsetScriptOnDisarm = GameVersion::GetOffset("CSWSDoor", "script_on_disarm");
        offsetScriptOnHeartbeat = GameVersion::GetOffset("CSWSDoor", "script_on_heartbeat");
        offsetScriptOnLock = GameVersion::GetOffset("CSWSDoor", "script_on_lock");
        offsetScriptOnMeleeAttacked = GameVersion::GetOffset("CSWSDoor", "script_on_melee_attacked");
        offsetScriptOnSpellCastAt = GameVersion::GetOffset("CSWSDoor", "script_on_spell_cast_at");
        offsetScriptOnTrapTriggered = GameVersion::GetOffset("CSWSDoor", "script_on_trap_tiggered");
        offsetScriptOnUnlock = GameVersion::GetOffset("CSWSDoor", "script_on_unlock");
        offsetScriptOnUserDefined = GameVersion::GetOffset("CSWSDoor", "script_on_user_defined");
        offsetScriptOnClick = GameVersion::GetOffset("CSWSDoor", "script_on_click");
        offsetScriptOnDialog = GameVersion::GetOffset("CSWSDoor", "script_on_dialog");
        offsetScriptOnFailToOpen = GameVersion::GetOffset("CSWSDoor", "script_on_fail_to_open");
        offsetAppearance = GameVersion::GetOffset("CSWSDoor", "appearance");
        offsetGenericType = GameVersion::GetOffset("CSWSDoor", "generic_type");
        offsetConversation = GameVersion::GetOffset("CSWSDoor", "conversation");
        offsetFaction = GameVersion::GetOffset("CSWSDoor", "faction");
        offsetFortitude = GameVersion::GetOffset("CSWSDoor", "fortitude");
        offsetReflex = GameVersion::GetOffset("CSWSDoor", "reflex");
        offsetWill = GameVersion::GetOffset("CSWSDoor", "will");
        offsetOpenLockDC = GameVersion::GetOffset("CSWSDoor", "open_lock_dc");
        offsetCloseLockDC = GameVersion::GetOffset("CSWSDoor", "close_lock_dc");
        offsetSecretDoorDC = GameVersion::GetOffset("CSWSDoor", "secret_door_dc");
        offsetHardness = GameVersion::GetOffset("CSWSDoor", "hardness");
        offsetOpenState = GameVersion::GetOffset("CSWSDoor", "open_state");
        offsetKeyName = GameVersion::GetOffset("CSWSDoor", "key_name");
        offsetKeyRequired = GameVersion::GetOffset("CSWSDoor", "key_required");
        offsetDisarmDC = GameVersion::GetOffset("CSWSDoor", "disarm_dc");
        offsetDetectDC = GameVersion::GetOffset("CSWSDoor", "detect_dc");
        offsetTrapType = GameVersion::GetOffset("CSWSDoor", "trap_type");
        offsetCorner = GameVersion::GetOffset("CSWSDoor", "corners");
        offsetLinkedToFlags = GameVersion::GetOffset("CSWSDoor", "linked_to_flags");
        offsetLinkedTo = GameVersion::GetOffset("CSWSDoor", "linked_to");
        offsetLinkedToModule = GameVersion::GetOffset("CSWSDoor", "linked_to_module");
        offsetLocName = GameVersion::GetOffset("CSWSDoor", "loc_name");
        offsetDescription = GameVersion::GetOffset("CSWSDoor", "description");
        offsetLoadScreenIdLower = GameVersion::GetOffset("CSWSDoor", "load_screen_id_lower");
        offsetLoadScreenIdUpper = GameVersion::GetOffset("CSWSDoor", "load_screen_id_upper");
        offsetTransitionDestination = GameVersion::GetOffset("CSWSDoor", "transition_destination");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSDoor] ERROR: %s\n", e.what());
    }
}

CSWSDoor::CSWSDoor(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSDoor::~CSWSDoor() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

CResRef* CSWSDoor::GetDialogResref(CResRef* outResRef) {
    if (!objectPtr || !getDialogResref) {
        return nullptr;
    }

    void* resultPtr = getDialogResref(objectPtr, outResRef ? outResRef->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

CExoLocString* CSWSDoor::GetFirstName() {
    if (!objectPtr || !getFirstName) {
        return nullptr;
    }

    void* resultPtr = getFirstName(objectPtr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoLocString(resultPtr);
}

int CSWSDoor::GetIsLinked() {
    if (!objectPtr || !getIsLinked) {
        return 0;
    }
    return getIsLinked(objectPtr);
}

CSWSObject* CSWSDoor::GetLinkedObject() {
    if (!objectPtr || !getLinkedObject) {
        return nullptr;
    }

    void* linkedPtr = getLinkedObject(objectPtr);
    if (!linkedPtr) {
        return nullptr;
    }

    return new CSWSObject(linkedPtr);
}

int CSWSDoor::InDoor(Vector point) {
    if (!objectPtr || !inDoor) {
        return 0;
    }
    return inDoor(objectPtr, point);
}

void CSWSDoor::MoveToNextOpenState() {
    if (!objectPtr || !moveToNextOpenState) {
        return;
    }
    moveToNextOpenState(objectPtr);
}

void CSWSDoor::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}

// ===== Offsets =====

CExoString* CSWSDoor::GetScriptOnOpen() {
    if (!objectPtr || offsetScriptOnOpen < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnOpen);
}

CExoString* CSWSDoor::GetScriptOnClosed() {
    if (!objectPtr || offsetScriptOnClosed < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnClosed);
}

CExoString* CSWSDoor::GetScriptOnDamaged() {
    if (!objectPtr || offsetScriptOnDamaged < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDamaged);
}

CExoString* CSWSDoor::GetScriptOnDeath() {
    if (!objectPtr || offsetScriptOnDeath < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDeath);
}

CExoString* CSWSDoor::GetScriptOnDisarm() {
    if (!objectPtr || offsetScriptOnDisarm < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDisarm);
}

CExoString* CSWSDoor::GetScriptOnHeartbeat() {
    if (!objectPtr || offsetScriptOnHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnHeartbeat);
}

CExoString* CSWSDoor::GetScriptOnLock() {
    if (!objectPtr || offsetScriptOnLock < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnLock);
}

CExoString* CSWSDoor::GetScriptOnMeleeAttacked() {
    if (!objectPtr || offsetScriptOnMeleeAttacked < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnMeleeAttacked);
}

CExoString* CSWSDoor::GetScriptOnSpellCastAt() {
    if (!objectPtr || offsetScriptOnSpellCastAt < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnSpellCastAt);
}

CExoString* CSWSDoor::GetScriptOnTrapTriggered() {
    if (!objectPtr || offsetScriptOnTrapTriggered < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnTrapTriggered);
}

CExoString* CSWSDoor::GetScriptOnUnlock() {
    if (!objectPtr || offsetScriptOnUnlock < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUnlock);
}

CExoString* CSWSDoor::GetScriptOnUserDefined() {
    if (!objectPtr || offsetScriptOnUserDefined < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUserDefined);
}

CExoString* CSWSDoor::GetScriptOnClick() {
    if (!objectPtr || offsetScriptOnClick < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnClick);
}

CExoString* CSWSDoor::GetScriptOnDialog() {
    if (!objectPtr || offsetScriptOnDialog < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDialog);
}

CExoString* CSWSDoor::GetScriptOnFailToOpen() {
    if (!objectPtr || offsetScriptOnFailToOpen < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnFailToOpen);
}

BYTE CSWSDoor::GetAppearance() {
    if (!objectPtr || offsetAppearance < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetAppearance);
}

void CSWSDoor::SetAppearance(BYTE value) {
    if (!objectPtr || offsetAppearance < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetAppearance, value);
}

BYTE CSWSDoor::GetGenericType() {
    if (!objectPtr || offsetGenericType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetGenericType);
}

void CSWSDoor::SetGenericType(BYTE value) {
    if (!objectPtr || offsetGenericType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetGenericType, value);
}

CResRef* CSWSDoor::GetConversation() {
    if (!objectPtr || offsetConversation < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<BYTE*>(objectPtr) + offsetConversation);
}

DWORD CSWSDoor::GetFaction() {
    if (!objectPtr || offsetFaction < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetFaction);
}

void CSWSDoor::SetFaction(DWORD value) {
    if (!objectPtr || offsetFaction < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetFaction, value);
}

BYTE CSWSDoor::GetFortitude() {
    if (!objectPtr || offsetFortitude < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetFortitude);
}

void CSWSDoor::SetFortitude(BYTE value) {
    if (!objectPtr || offsetFortitude < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetFortitude, value);
}

BYTE CSWSDoor::GetReflex() {
    if (!objectPtr || offsetReflex < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetReflex);
}

void CSWSDoor::SetReflex(BYTE value) {
    if (!objectPtr || offsetReflex < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetReflex, value);
}

BYTE CSWSDoor::GetWill() {
    if (!objectPtr || offsetWill < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetWill);
}

void CSWSDoor::SetWill(BYTE value) {
    if (!objectPtr || offsetWill < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetWill, value);
}

BYTE CSWSDoor::GetOpenLockDC() {
    if (!objectPtr || offsetOpenLockDC < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetOpenLockDC);
}

void CSWSDoor::SetOpenLockDC(BYTE value) {
    if (!objectPtr || offsetOpenLockDC < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetOpenLockDC, value);
}

BYTE CSWSDoor::GetCloseLockDC() {
    if (!objectPtr || offsetCloseLockDC < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetCloseLockDC);
}

void CSWSDoor::SetCloseLockDC(BYTE value) {
    if (!objectPtr || offsetCloseLockDC < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetCloseLockDC, value);
}

BYTE CSWSDoor::GetSecretDoorDC() {
    if (!objectPtr || offsetSecretDoorDC < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetSecretDoorDC);
}

void CSWSDoor::SetSecretDoorDC(BYTE value) {
    if (!objectPtr || offsetSecretDoorDC < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetSecretDoorDC, value);
}

BYTE CSWSDoor::GetHardness() {
    if (!objectPtr || offsetHardness < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetHardness);
}

void CSWSDoor::SetHardness(BYTE value) {
    if (!objectPtr || offsetHardness < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetHardness, value);
}

BYTE CSWSDoor::GetOpenState() {
    if (!objectPtr || offsetOpenState < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetOpenState);
}

void CSWSDoor::SetOpenState(BYTE value) {
    if (!objectPtr || offsetOpenState < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetOpenState, value);
}

CExoString* CSWSDoor::GetKeyName() {
    if (!objectPtr || offsetKeyName < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetKeyName);
}

int CSWSDoor::GetKeyRequired() {
    if (!objectPtr || offsetKeyRequired < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetKeyRequired);
}

void CSWSDoor::SetKeyRequired(int value) {
    if (!objectPtr || offsetKeyRequired < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetKeyRequired, value);
}

BYTE CSWSDoor::GetDisarmDC() {
    if (!objectPtr || offsetDisarmDC < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetDisarmDC);
}

void CSWSDoor::SetDisarmDC(BYTE value) {
    if (!objectPtr || offsetDisarmDC < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetDisarmDC, value);
}

BYTE CSWSDoor::GetDetectDC() {
    if (!objectPtr || offsetDetectDC < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetDetectDC);
}

void CSWSDoor::SetDetectDC(BYTE value) {
    if (!objectPtr || offsetDetectDC < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetDetectDC, value);
}

BYTE CSWSDoor::GetTrapType() {
    if (!objectPtr || offsetTrapType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetTrapType);
}

void CSWSDoor::SetTrapType(BYTE value) {
    if (!objectPtr || offsetTrapType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetTrapType, value);
}

Vector CSWSDoor::GetCorner(int index) {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetCorner < 0 || index < 0 || index > 3) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetCorner + index * sizeof(Vector));
}

void CSWSDoor::SetCorner(int index, const Vector& value) {
    if (!objectPtr || offsetCorner < 0 || index < 0 || index > 3) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetCorner + index * sizeof(Vector), value);
}

BYTE CSWSDoor::GetLinkedToFlags() {
    if (!objectPtr || offsetLinkedToFlags < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetLinkedToFlags);
}

void CSWSDoor::SetLinkedToFlags(BYTE value) {
    if (!objectPtr || offsetLinkedToFlags < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetLinkedToFlags, value);
}

CExoString* CSWSDoor::GetLinkedTo() {
    if (!objectPtr || offsetLinkedTo < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetLinkedTo);
}

CExoString* CSWSDoor::GetLinkedToModule() {
    if (!objectPtr || offsetLinkedToModule < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetLinkedToModule);
}

CExoLocString* CSWSDoor::GetLocName() {
    if (!objectPtr || offsetLocName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocName);
}

CExoLocString* CSWSDoor::GetDescription() {
    if (!objectPtr || offsetDescription < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetDescription);
}

BYTE CSWSDoor::GetLoadScreenIdLower() {
    if (!objectPtr || offsetLoadScreenIdLower < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetLoadScreenIdLower);
}

void CSWSDoor::SetLoadScreenIdLower(BYTE value) {
    if (!objectPtr || offsetLoadScreenIdLower < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetLoadScreenIdLower, value);
}

BYTE CSWSDoor::GetLoadScreenIdUpper() {
    if (!objectPtr || offsetLoadScreenIdUpper < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetLoadScreenIdUpper);
}

void CSWSDoor::SetLoadScreenIdUpper(BYTE value) {
    if (!objectPtr || offsetLoadScreenIdUpper < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetLoadScreenIdUpper, value);
}

CExoLocString* CSWSDoor::GetTransitionDestination() {
    if (!objectPtr || offsetTransitionDestination < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetTransitionDestination);
}
