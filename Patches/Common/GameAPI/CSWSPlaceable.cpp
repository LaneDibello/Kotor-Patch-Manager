#include "CSWSPlaceable.h"
#include "CExoString.h"
#include "CExoLocString.h"
#include "GameVersion.h"
#include "CSWCPlaceable.h"
#include "CResRef.h"

CSWSPlaceable::GetBodyBagAppearanceFn CSWSPlaceable::getBodyBagAppearance = nullptr;
CSWSPlaceable::GetDeadFn CSWSPlaceable::getDead = nullptr;
CSWSPlaceable::GetEffectSpellIdFn CSWSPlaceable::getEffectSpellId = nullptr;
CSWSPlaceable::SetEffectSpellIdFn CSWSPlaceable::setEffectSpellId = nullptr;
CSWSPlaceable::GetLightIsOnFn CSWSPlaceable::getLightIsOn = nullptr;
CSWSPlaceable::SetLightIsOnFn CSWSPlaceable::setLightIsOn = nullptr;
CSWSPlaceable::GetDialogResrefFn CSWSPlaceable::getDialogResref = nullptr;
CSWSPlaceable::GetClientPlaceableFn CSWSPlaceable::getClientPlaceable = nullptr;
CSWSPlaceable::RemoveFromAreaFn CSWSPlaceable::removeFromArea = nullptr;
CSWSPlaceable::SetOrientationFn CSWSPlaceable::setOrientation = nullptr;

int CSWSPlaceable::offsetLocName = -1;
int CSWSPlaceable::offsetAppearance = -1;
int CSWSPlaceable::offsetDescription = -1;
int CSWSPlaceable::offsetFaction = -1;
int CSWSPlaceable::offsetConversation = -1;
int CSWSPlaceable::offsetHardness = -1;
int CSWSPlaceable::offsetLocked = -1;
int CSWSPlaceable::offsetKeyName = -1;
int CSWSPlaceable::offsetKeyRequired = -1;
int CSWSPlaceable::offsetAutoRemoveKey = -1;
int CSWSPlaceable::offsetOpenLockDC = -1;
int CSWSPlaceable::offsetTrapDetectDC = -1;
int CSWSPlaceable::offsetTrapFlag = -1;
int CSWSPlaceable::offsetDisarmDC = -1;
int CSWSPlaceable::offsetTrapDisarmable = -1;
int CSWSPlaceable::offsetTrapDetectable = -1;
int CSWSPlaceable::offsetTrapOneShot = -1;
int CSWSPlaceable::offsetTrapType = -1;
int CSWSPlaceable::offsetScriptOnClosed = -1;
int CSWSPlaceable::offsetScriptOnDamaged = -1;
int CSWSPlaceable::offsetScriptOnDeath = -1;
int CSWSPlaceable::offsetScriptOnDisarm = -1;
int CSWSPlaceable::offsetScriptOnHeartbeat = -1;
int CSWSPlaceable::offsetScriptOnInventoryDisturbed = -1;
int CSWSPlaceable::offsetScriptOnLock = -1;
int CSWSPlaceable::offsetScriptOnMeleeAttacked = -1;
int CSWSPlaceable::offsetScriptOnOpen = -1;
int CSWSPlaceable::offsetScriptOnSpellCastAt = -1;
int CSWSPlaceable::offsetScriptOnTrapTriggered = -1;
int CSWSPlaceable::offsetScriptOnUnlock = -1;
int CSWSPlaceable::offsetScriptOnUsed = -1;
int CSWSPlaceable::offsetScriptOnUserDefined = -1;
int CSWSPlaceable::offsetScriptOnDialog = -1;
int CSWSPlaceable::offsetScriptOnEndDialog = -1;
int CSWSPlaceable::offsetFortitude = -1;
int CSWSPlaceable::offsetWill = -1;
int CSWSPlaceable::offsetReflex = -1;
int CSWSPlaceable::offsetUsable = -1;
int CSWSPlaceable::offsetLockable = -1;
int CSWSPlaceable::offsetHasInventory = -1;
int CSWSPlaceable::offsetOpen = -1;
int CSWSPlaceable::offsetTemplateResRef = -1;
int CSWSPlaceable::offsetBodyBag = -1;
int CSWSPlaceable::offsetOrientationQuat = -1;
int CSWSPlaceable::offsetIsBodyBag = -1;
int CSWSPlaceable::offsetIsCorpse = -1;
int CSWSPlaceable::offsetLastHeartbeatDay = -1;
int CSWSPlaceable::offsetLastHeartbeatMs = -1;

bool CSWSPlaceable::functionsInitialized = false;
bool CSWSPlaceable::offsetsInitialized = false;

void CSWSPlaceable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSPlaceable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getBodyBagAppearance = reinterpret_cast<GetBodyBagAppearanceFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetBodyBagAppearance"));
        getDead = reinterpret_cast<GetDeadFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetDead"));
        getEffectSpellId = reinterpret_cast<GetEffectSpellIdFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetEffectSpellId"));
        setEffectSpellId = reinterpret_cast<SetEffectSpellIdFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetEffectSpellId"));
        getLightIsOn = reinterpret_cast<GetLightIsOnFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetLightIsOn"));
        setLightIsOn = reinterpret_cast<SetLightIsOnFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetLightIsOn"));
        getDialogResref = reinterpret_cast<GetDialogResrefFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetDialogResref"));
        getClientPlaceable = reinterpret_cast<GetClientPlaceableFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetClientPlaceable"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "RemoveFromArea"));
        setOrientation = reinterpret_cast<SetOrientationFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetOrientation"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSPlaceable] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSPlaceable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSPlaceable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetLocName = GameVersion::GetOffset("CSWSPlaceable", "loc_name");
        offsetAppearance = GameVersion::GetOffset("CSWSPlaceable", "appearance");
        offsetDescription = GameVersion::GetOffset("CSWSPlaceable", "description");
        offsetFaction = GameVersion::GetOffset("CSWSPlaceable", "faction");
        offsetConversation = GameVersion::GetOffset("CSWSPlaceable", "conversation");
        offsetHardness = GameVersion::GetOffset("CSWSPlaceable", "hardness");
        offsetLocked = GameVersion::GetOffset("CSWSPlaceable", "locked");
        offsetKeyName = GameVersion::GetOffset("CSWSPlaceable", "key_name");
        offsetKeyRequired = GameVersion::GetOffset("CSWSPlaceable", "key_required");
        offsetAutoRemoveKey = GameVersion::GetOffset("CSWSPlaceable", "auto_remove_key");
        offsetOpenLockDC = GameVersion::GetOffset("CSWSPlaceable", "open_lock_dc");
        offsetTrapDetectDC = GameVersion::GetOffset("CSWSPlaceable", "trap_detect_dc");
        offsetTrapFlag = GameVersion::GetOffset("CSWSPlaceable", "trap_flag");
        offsetDisarmDC = GameVersion::GetOffset("CSWSPlaceable", "disarm_dc");
        offsetTrapDisarmable = GameVersion::GetOffset("CSWSPlaceable", "trap_disarmable");
        offsetTrapDetectable = GameVersion::GetOffset("CSWSPlaceable", "trap_detectable");
        offsetTrapOneShot = GameVersion::GetOffset("CSWSPlaceable", "trap_one_shot");
        offsetTrapType = GameVersion::GetOffset("CSWSPlaceable", "trap_type");
        offsetScriptOnClosed = GameVersion::GetOffset("CSWSPlaceable", "script_on_closed");
        offsetScriptOnDamaged = GameVersion::GetOffset("CSWSPlaceable", "script_on_damaged");
        offsetScriptOnDeath = GameVersion::GetOffset("CSWSPlaceable", "script_on_death");
        offsetScriptOnDisarm = GameVersion::GetOffset("CSWSPlaceable", "script_on_disarm");
        offsetScriptOnHeartbeat = GameVersion::GetOffset("CSWSPlaceable", "script_on_heartbeat");
        offsetScriptOnInventoryDisturbed = GameVersion::GetOffset("CSWSPlaceable", "script_on_inventory_disturbed");
        offsetScriptOnLock = GameVersion::GetOffset("CSWSPlaceable", "script_on_lock");
        offsetScriptOnMeleeAttacked = GameVersion::GetOffset("CSWSPlaceable", "script_on_melee_attacked");
        offsetScriptOnOpen = GameVersion::GetOffset("CSWSPlaceable", "script_on_open");
        offsetScriptOnSpellCastAt = GameVersion::GetOffset("CSWSPlaceable", "script_on_spell_cast_at");
        offsetScriptOnTrapTriggered = GameVersion::GetOffset("CSWSPlaceable", "script_on_trap_triggered");
        offsetScriptOnUnlock = GameVersion::GetOffset("CSWSPlaceable", "script_on_unlock");
        offsetScriptOnUsed = GameVersion::GetOffset("CSWSPlaceable", "script_on_used");
        offsetScriptOnUserDefined = GameVersion::GetOffset("CSWSPlaceable", "script_on_user_defined");
        offsetScriptOnDialog = GameVersion::GetOffset("CSWSPlaceable", "script_on_dialog");
        offsetScriptOnEndDialog = GameVersion::GetOffset("CSWSPlaceable", "script_on_end_dialog");
        offsetFortitude = GameVersion::GetOffset("CSWSPlaceable", "fortitude");
        offsetWill = GameVersion::GetOffset("CSWSPlaceable", "will");
        offsetReflex = GameVersion::GetOffset("CSWSPlaceable", "reflex");
        offsetUsable = GameVersion::GetOffset("CSWSPlaceable", "usable");
        offsetLockable = GameVersion::GetOffset("CSWSPlaceable", "lockable");
        offsetHasInventory = GameVersion::GetOffset("CSWSPlaceable", "has_inventory");
        offsetOpen = GameVersion::GetOffset("CSWSPlaceable", "open");
        offsetTemplateResRef = GameVersion::GetOffset("CSWSPlaceable", "template_res_ref");
        offsetBodyBag = GameVersion::GetOffset("CSWSPlaceable", "body_bag");
        offsetOrientationQuat = GameVersion::GetOffset("CSWSPlaceable", "orientation");
        offsetIsBodyBag = GameVersion::GetOffset("CSWSPlaceable", "is_body_bag");
        offsetIsCorpse = GameVersion::GetOffset("CSWSPlaceable", "is_corpse");
        offsetLastHeartbeatDay = GameVersion::GetOffset("CSWSPlaceable", "last_heartbeat_day");
        offsetLastHeartbeatMs = GameVersion::GetOffset("CSWSPlaceable", "last_heartbeat_ms");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSPlaceable] ERROR: %s\n", e.what());
    }
}

CSWSPlaceable::CSWSPlaceable(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSPlaceable::~CSWSPlaceable() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

BYTE CSWSPlaceable::GetBodyBagAppearance() {
    if (!objectPtr || !getBodyBagAppearance) {
        return 0;
    }
    return getBodyBagAppearance(objectPtr);
}

int CSWSPlaceable::GetDead() {
    if (!objectPtr || !getDead) {
        return 0;
    }
    return getDead(objectPtr);
}

DWORD CSWSPlaceable::GetEffectSpellId() {
    if (!objectPtr || !getEffectSpellId) {
        return 0;
    }
    return getEffectSpellId(objectPtr);
}

void CSWSPlaceable::SetEffectSpellId(DWORD spellId) {
    if (!objectPtr || !setEffectSpellId) {
        return;
    }
    setEffectSpellId(objectPtr, spellId);
}

int CSWSPlaceable::GetLightIsOn() {
    if (!objectPtr || !getLightIsOn) {
        return 0;
    }
    return getLightIsOn(objectPtr);
}

void CSWSPlaceable::SetLightIsOn(int lightState) {
    if (!objectPtr || !setLightIsOn) {
        return;
    }
    setLightIsOn(objectPtr, lightState);
}

CResRef* CSWSPlaceable::GetDialogResref(CResRef* outResref) {
    if (!objectPtr || !getDialogResref) {
        return nullptr;
    }

    void* resultPtr = getDialogResref(objectPtr, outResref ? outResref->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

CSWCPlaceable* CSWSPlaceable::GetClientPlaceable() {
    if (!objectPtr || !getClientPlaceable) {
        return nullptr;
    }

    void* clientPtr = getClientPlaceable(objectPtr);
    if (!clientPtr) {
        return nullptr;
    }

    return new CSWCPlaceable(clientPtr);
}

void CSWSPlaceable::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}

void CSWSPlaceable::SetOrientation(Quaternion* orientation) {
    if (!objectPtr || !setOrientation) {
        return;
    }
    setOrientation(objectPtr, orientation);
}

// ===== Offsets =====

CExoLocString* CSWSPlaceable::GetLocName() {
    if (!objectPtr || offsetLocName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocName);
}

DWORD CSWSPlaceable::GetAppearance() {
    if (!objectPtr || offsetAppearance < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetAppearance);
}

void CSWSPlaceable::SetAppearance(DWORD value) {
    if (!objectPtr || offsetAppearance < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetAppearance, value);
}

CExoLocString* CSWSPlaceable::GetDescription() {
    if (!objectPtr || offsetDescription < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetDescription);
}

DWORD CSWSPlaceable::GetFaction() {
    if (!objectPtr || offsetFaction < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetFaction);
}

void CSWSPlaceable::SetFaction(DWORD value) {
    if (!objectPtr || offsetFaction < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetFaction, value);
}

CResRef* CSWSPlaceable::GetConversation() {
    if (!objectPtr || offsetConversation < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<BYTE*>(objectPtr) + offsetConversation);
}

int CSWSPlaceable::GetHardness() {
    if (!objectPtr || offsetHardness < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHardness);
}

void CSWSPlaceable::SetHardness(int value) {
    if (!objectPtr || offsetHardness < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHardness, value);
}

int CSWSPlaceable::GetLocked() {
    if (!objectPtr || offsetLocked < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLocked);
}

void CSWSPlaceable::SetLocked(int value) {
    if (!objectPtr || offsetLocked < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLocked, value);
}

CExoString* CSWSPlaceable::GetKeyName() {
    if (!objectPtr || offsetKeyName < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetKeyName);
}

int CSWSPlaceable::GetKeyRequired() {
    if (!objectPtr || offsetKeyRequired < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetKeyRequired);
}

void CSWSPlaceable::SetKeyRequired(int value) {
    if (!objectPtr || offsetKeyRequired < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetKeyRequired, value);
}

int CSWSPlaceable::GetAutoRemoveKey() {
    if (!objectPtr || offsetAutoRemoveKey < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetAutoRemoveKey);
}

void CSWSPlaceable::SetAutoRemoveKey(int value) {
    if (!objectPtr || offsetAutoRemoveKey < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetAutoRemoveKey, value);
}

short CSWSPlaceable::GetOpenLockDC() {
    if (!objectPtr || offsetOpenLockDC < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetOpenLockDC);
}

void CSWSPlaceable::SetOpenLockDC(short value) {
    if (!objectPtr || offsetOpenLockDC < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetOpenLockDC, value);
}

short CSWSPlaceable::GetTrapDetectDC() {
    if (!objectPtr || offsetTrapDetectDC < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetTrapDetectDC);
}

void CSWSPlaceable::SetTrapDetectDC(short value) {
    if (!objectPtr || offsetTrapDetectDC < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetTrapDetectDC, value);
}

int CSWSPlaceable::GetTrapFlag() {
    if (!objectPtr || offsetTrapFlag < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTrapFlag);
}

void CSWSPlaceable::SetTrapFlag(int value) {
    if (!objectPtr || offsetTrapFlag < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTrapFlag, value);
}

int CSWSPlaceable::GetDisarmDC() {
    if (!objectPtr || offsetDisarmDC < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetDisarmDC);
}

void CSWSPlaceable::SetDisarmDC(int value) {
    if (!objectPtr || offsetDisarmDC < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetDisarmDC, value);
}

int CSWSPlaceable::GetTrapDisarmable() {
    if (!objectPtr || offsetTrapDisarmable < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTrapDisarmable);
}

void CSWSPlaceable::SetTrapDisarmable(int value) {
    if (!objectPtr || offsetTrapDisarmable < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTrapDisarmable, value);
}

int CSWSPlaceable::GetTrapDetectable() {
    if (!objectPtr || offsetTrapDetectable < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTrapDetectable);
}

void CSWSPlaceable::SetTrapDetectable(int value) {
    if (!objectPtr || offsetTrapDetectable < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTrapDetectable, value);
}

int CSWSPlaceable::GetTrapOneShot() {
    if (!objectPtr || offsetTrapOneShot < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTrapOneShot);
}

void CSWSPlaceable::SetTrapOneShot(int value) {
    if (!objectPtr || offsetTrapOneShot < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTrapOneShot, value);
}

int CSWSPlaceable::GetTrapType() {
    if (!objectPtr || offsetTrapType < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTrapType);
}

void CSWSPlaceable::SetTrapType(int value) {
    if (!objectPtr || offsetTrapType < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTrapType, value);
}

CExoString* CSWSPlaceable::GetScriptOnClosed() {
    if (!objectPtr || offsetScriptOnClosed < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnClosed);
}

CExoString* CSWSPlaceable::GetScriptOnDamaged() {
    if (!objectPtr || offsetScriptOnDamaged < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDamaged);
}

CExoString* CSWSPlaceable::GetScriptOnDeath() {
    if (!objectPtr || offsetScriptOnDeath < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDeath);
}

CExoString* CSWSPlaceable::GetScriptOnDisarm() {
    if (!objectPtr || offsetScriptOnDisarm < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDisarm);
}

CExoString* CSWSPlaceable::GetScriptOnHeartbeat() {
    if (!objectPtr || offsetScriptOnHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnHeartbeat);
}

CExoString* CSWSPlaceable::GetScriptOnInventoryDisturbed() {
    if (!objectPtr || offsetScriptOnInventoryDisturbed < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnInventoryDisturbed);
}

CExoString* CSWSPlaceable::GetScriptOnLock() {
    if (!objectPtr || offsetScriptOnLock < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnLock);
}

CExoString* CSWSPlaceable::GetScriptOnMeleeAttacked() {
    if (!objectPtr || offsetScriptOnMeleeAttacked < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnMeleeAttacked);
}

CExoString* CSWSPlaceable::GetScriptOnOpen() {
    if (!objectPtr || offsetScriptOnOpen < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnOpen);
}

CExoString* CSWSPlaceable::GetScriptOnSpellCastAt() {
    if (!objectPtr || offsetScriptOnSpellCastAt < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnSpellCastAt);
}

CExoString* CSWSPlaceable::GetScriptOnTrapTriggered() {
    if (!objectPtr || offsetScriptOnTrapTriggered < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnTrapTriggered);
}

CExoString* CSWSPlaceable::GetScriptOnUnlock() {
    if (!objectPtr || offsetScriptOnUnlock < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUnlock);
}

CExoString* CSWSPlaceable::GetScriptOnUsed() {
    if (!objectPtr || offsetScriptOnUsed < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUsed);
}

CExoString* CSWSPlaceable::GetScriptOnUserDefined() {
    if (!objectPtr || offsetScriptOnUserDefined < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUserDefined);
}

CExoString* CSWSPlaceable::GetScriptOnDialog() {
    if (!objectPtr || offsetScriptOnDialog < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnDialog);
}

CExoString* CSWSPlaceable::GetScriptOnEndDialog() {
    if (!objectPtr || offsetScriptOnEndDialog < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnEndDialog);
}

BYTE CSWSPlaceable::GetFortitude() {
    if (!objectPtr || offsetFortitude < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetFortitude);
}

void CSWSPlaceable::SetFortitude(BYTE value) {
    if (!objectPtr || offsetFortitude < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetFortitude, value);
}

BYTE CSWSPlaceable::GetWill() {
    if (!objectPtr || offsetWill < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetWill);
}

void CSWSPlaceable::SetWill(BYTE value) {
    if (!objectPtr || offsetWill < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetWill, value);
}

BYTE CSWSPlaceable::GetReflex() {
    if (!objectPtr || offsetReflex < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetReflex);
}

void CSWSPlaceable::SetReflex(BYTE value) {
    if (!objectPtr || offsetReflex < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetReflex, value);
}

int CSWSPlaceable::GetUsable() {
    if (!objectPtr || offsetUsable < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetUsable);
}

void CSWSPlaceable::SetUsable(int value) {
    if (!objectPtr || offsetUsable < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetUsable, value);
}

int CSWSPlaceable::GetLockable() {
    if (!objectPtr || offsetLockable < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLockable);
}

void CSWSPlaceable::SetLockable(int value) {
    if (!objectPtr || offsetLockable < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLockable, value);
}

int CSWSPlaceable::GetHasInventory() {
    if (!objectPtr || offsetHasInventory < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHasInventory);
}

void CSWSPlaceable::SetHasInventory(int value) {
    if (!objectPtr || offsetHasInventory < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHasInventory, value);
}

int CSWSPlaceable::GetOpen() {
    if (!objectPtr || offsetOpen < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetOpen);
}

void CSWSPlaceable::SetOpen(int value) {
    if (!objectPtr || offsetOpen < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetOpen, value);
}

CResRef* CSWSPlaceable::GetTemplateResRef() {
    if (!objectPtr || offsetTemplateResRef < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<BYTE*>(objectPtr) + offsetTemplateResRef);
}

int CSWSPlaceable::GetBodyBag() {
    if (!objectPtr || offsetBodyBag < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBodyBag);
}

void CSWSPlaceable::SetBodyBag(int value) {
    if (!objectPtr || offsetBodyBag < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetBodyBag, value);
}

Quaternion CSWSPlaceable::GetOrientationQuat() {
    Quaternion result = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetOrientationQuat < 0) {
        return result;
    }

    return getObjectProperty<Quaternion>(objectPtr, offsetOrientationQuat);
}

int CSWSPlaceable::GetIsBodyBag() {
    if (!objectPtr || offsetIsBodyBag < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetIsBodyBag);
}

void CSWSPlaceable::SetIsBodyBag(int value) {
    if (!objectPtr || offsetIsBodyBag < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetIsBodyBag, value);
}

int CSWSPlaceable::GetIsCorpse() {
    if (!objectPtr || offsetIsCorpse < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetIsCorpse);
}

void CSWSPlaceable::SetIsCorpse(int value) {
    if (!objectPtr || offsetIsCorpse < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetIsCorpse, value);
}

int CSWSPlaceable::GetLastHeartbeatDay() {
    if (!objectPtr || offsetLastHeartbeatDay < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLastHeartbeatDay);
}

void CSWSPlaceable::SetLastHeartbeatDay(int value) {
    if (!objectPtr || offsetLastHeartbeatDay < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLastHeartbeatDay, value);
}

int CSWSPlaceable::GetLastHeartbeatMs() {
    if (!objectPtr || offsetLastHeartbeatMs < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLastHeartbeatMs);
}

void CSWSPlaceable::SetLastHeartbeatMs(int value) {
    if (!objectPtr || offsetLastHeartbeatMs < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLastHeartbeatMs, value);
}
