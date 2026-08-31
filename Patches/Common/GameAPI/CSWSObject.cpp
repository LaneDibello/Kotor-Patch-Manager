#include "CSWSObject.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CSWCObject.h"
#include "CExoString.h"
#include "CResRef.h"
#include "CExoLocString.h"

CSWSObject::AddActionToFrontFn CSWSObject::addActionToFront = nullptr;

CSWSObject::ClearAllActionsFn CSWSObject::clearAllActions = nullptr;
CSWSObject::GetAcceptableActionFn CSWSObject::getAcceptableAction = nullptr;
CSWSObject::GetAIStateReputationFn CSWSObject::getAIStateReputation = nullptr;
CSWSObject::GetDeadFn CSWSObject::getDead = nullptr;
CSWSObject::GetHasFeatEffectAppliedFn CSWSObject::getHasFeatEffectApplied = nullptr;
CSWSObject::HasSpellEffectAppliedFn CSWSObject::hasSpellEffectApplied = nullptr;
CSWSObject::SetAnimationFn CSWSObject::setAnimation = nullptr;
CSWSObject::SetCurrentHitPointsFn CSWSObject::setCurrentHitPoints = nullptr;
CSWSObject::SetKeepCorpseFn CSWSObject::setKeepCorpse = nullptr;
CSWSObject::SpawnBodyBagFn CSWSObject::spawnBodyBag = nullptr;

CSWSObject::GetLastNameFn CSWSObject::getLastName = nullptr;
CSWSObject::SetTagFn CSWSObject::setTag = nullptr;
CSWSObject::GetPortraitFn CSWSObject::getPortrait = nullptr;
CSWSObject::SetPortraitFn CSWSObject::setPortrait = nullptr;
CSWSObject::GetPortraitIdFn CSWSObject::getPortraitId = nullptr;
CSWSObject::SetPortraitIdFn CSWSObject::setPortraitId = nullptr;

CSWSObject::GetScriptLocationFn CSWSObject::getScriptLocation = nullptr;
CSWSObject::GetNearestObjectByNameFn CSWSObject::getNearestObjectByName = nullptr;
CSWSObject::GetClientObjectFn CSWSObject::getClientObject = nullptr;

CSWSObject::GetDialogResrefFn CSWSObject::getDialogResref = nullptr;
CSWSObject::SetDialogDelayFn CSWSObject::setDialogDelay = nullptr;
CSWSObject::SetDialogOwnerFn CSWSObject::setDialogOwner = nullptr;
CSWSObject::StopDialogFn CSWSObject::stopDialog = nullptr;
CSWSObject::StopSoundPlayingInDialogFn CSWSObject::stopSoundPlayingInDialog = nullptr;

bool CSWSObject::functionsInitialized = false;

int CSWSObject::offsetPosition = -1;
int CSWSObject::offsetOrientation = -1;
int CSWSObject::offsetAreaId = -1;
bool CSWSObject::offsetsInitialized = false;

void CSWSObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    // Call base class initialization first
    CGameObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addActionToFront = reinterpret_cast<AddActionToFrontFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "AddActionToFront")
        );

        clearAllActions = reinterpret_cast<ClearAllActionsFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "ClearAllActions"));
        getAcceptableAction = reinterpret_cast<GetAcceptableActionFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetAcceptableAction"));
        getAIStateReputation = reinterpret_cast<GetAIStateReputationFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetAIStateReputation"));
        getDead = reinterpret_cast<GetDeadFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetDead"));
        getHasFeatEffectApplied = reinterpret_cast<GetHasFeatEffectAppliedFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetHasFeatEffectApplied"));
        hasSpellEffectApplied = reinterpret_cast<HasSpellEffectAppliedFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "HasSpellEffectApplied"));
        setAnimation = reinterpret_cast<SetAnimationFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetAnimation"));
        setCurrentHitPoints = reinterpret_cast<SetCurrentHitPointsFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetCurrentHitPoints"));
        setKeepCorpse = reinterpret_cast<SetKeepCorpseFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetKeepCorpse"));
        spawnBodyBag = reinterpret_cast<SpawnBodyBagFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SpawnBodyBag"));

        getLastName = reinterpret_cast<GetLastNameFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetLastName"));
        setTag = reinterpret_cast<SetTagFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetTag"));
        getPortrait = reinterpret_cast<GetPortraitFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetPortrait"));
        setPortrait = reinterpret_cast<SetPortraitFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetPortrait"));
        getPortraitId = reinterpret_cast<GetPortraitIdFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetPortraitId"));
        setPortraitId = reinterpret_cast<SetPortraitIdFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetPortraitId"));

        getScriptLocation = reinterpret_cast<GetScriptLocationFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetScriptLocation"));
        getNearestObjectByName = reinterpret_cast<GetNearestObjectByNameFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetNearestObjectByName"));
        getClientObject = reinterpret_cast<GetClientObjectFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetClientObject"));

        getDialogResref = reinterpret_cast<GetDialogResrefFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "GetDialogResref"));
        setDialogDelay = reinterpret_cast<SetDialogDelayFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetDialogDelay"));
        setDialogOwner = reinterpret_cast<SetDialogOwnerFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "SetDialogOwner"));
        stopDialog = reinterpret_cast<StopDialogFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "StopDialog"));
        stopSoundPlayingInDialog = reinterpret_cast<StopSoundPlayingInDialogFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "StopSoundPlayingInDialog"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSObject] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    // Call base class offset initialization
    CGameObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetPosition = GameVersion::GetOffset("CSWSObject", "Position");
        offsetOrientation = GameVersion::GetOffset("CSWSObject", "Orientation");
        offsetAreaId = GameVersion::GetOffset("CSWSObject", "AreaId");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSObject] ERROR: %s\n", e.what());
    }
}

CSWSObject::CSWSObject(void* objectPtr)
    : CGameObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSObject::~CSWSObject() {
    // Base class destructor will handle objectPtr cleanup
}

void CSWSObject::AddActionToFront(
    DWORD param_1, USHORT param_2, DWORD param_3, void* param_4, DWORD param_5,
    void* param_6, DWORD param_7, void* param_8, DWORD param_9, void* param_10, DWORD param_11,
    void* param_12, DWORD param_13, void* param_14, DWORD param_15, void* param_16, DWORD param_17,
    void* param_18, DWORD param_19, void* param_20, DWORD param_21, void* param_22, DWORD param_23,
    void* param_24, DWORD param_25, void* param_26, DWORD param_27, void* param_28)
{
    if (!objectPtr || !addActionToFront) {
        return;
    }
    debugLog("[CSWSObject::AddActionToFront] objectPtr is %p");

    addActionToFront(objectPtr, param_1, param_2, param_3, param_4, param_5,
        param_6, param_7, param_8, param_9, param_10, param_11,
        param_12, param_13, param_14, param_15, param_16, param_17,
        param_18, param_19, param_20, param_21, param_22, param_23,
        param_24, param_25, param_26, param_27, param_28);
}

Vector CSWSObject::GetPosition() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!objectPtr || offsetPosition < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetPosition);
}

Vector CSWSObject::GetOrientation() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!objectPtr || offsetOrientation < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetOrientation);
}

DWORD CSWSObject::GetAreaId() {
    if (!objectPtr || offsetAreaId < 0) {
        return 0x7F000000;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetAreaId);
}

void CSWSObject::SetPosition(const Vector& position) {
    if (!objectPtr || offsetPosition < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetPosition, position);
}

void CSWSObject::SetOrientation(const Vector& orientation) {
    if (!objectPtr || offsetOrientation < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetOrientation, orientation);
}

void CSWSObject::SetAreaId(DWORD areaId) {
    if (!objectPtr || offsetAreaId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetAreaId, areaId);
}

// ===== Actions =====

void CSWSObject::ClearAllActions(int includeAttacks) {
    if (!objectPtr || !clearAllActions) {
        return;
    }
    clearAllActions(objectPtr, includeAttacks);
}

int CSWSObject::GetAcceptableAction(DWORD action) {
    if (!objectPtr || !getAcceptableAction) {
        return 0;
    }
    return getAcceptableAction(objectPtr, action);
}

// ===== State =====

BYTE CSWSObject::GetAIStateReputation(DWORD objectId) {
    if (!objectPtr || !getAIStateReputation) {
        return 0;
    }
    return getAIStateReputation(objectPtr, objectId);
}

int CSWSObject::GetDead() {
    if (!objectPtr || !getDead) {
        return 0;
    }
    return getDead(objectPtr);
}

int CSWSObject::GetHasFeatEffectApplied(WORD feat) {
    if (!objectPtr || !getHasFeatEffectApplied) {
        return 0;
    }
    return getHasFeatEffectApplied(objectPtr, feat);
}

int CSWSObject::HasSpellEffectApplied(int spellId) {
    if (!objectPtr || !hasSpellEffectApplied) {
        return 0;
    }
    return hasSpellEffectApplied(objectPtr, spellId);
}

void CSWSObject::SetAnimation(int animation) {
    if (!objectPtr || !setAnimation) {
        return;
    }
    setAnimation(objectPtr, animation);
}

void CSWSObject::SetCurrentHitPoints(int currentHP) {
    if (!objectPtr || !setCurrentHitPoints) {
        return;
    }
    setCurrentHitPoints(objectPtr, currentHP);
}

void CSWSObject::SetKeepCorpse(int keepCorpse) {
    if (!objectPtr || !setKeepCorpse) {
        return;
    }
    setKeepCorpse(objectPtr, keepCorpse);
}

int CSWSObject::SpawnBodyBag() {
    if (!objectPtr || !spawnBodyBag) {
        return 0;
    }
    return spawnBodyBag(objectPtr);
}

// ===== Identity / naming =====

CExoLocString* CSWSObject::GetLastName() {
    if (!objectPtr || !getLastName) {
        return nullptr;
    }

    void* resultPtr = getLastName(objectPtr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoLocString(resultPtr);
}

void CSWSObject::SetTag(CExoString* tag) {
    if (!objectPtr || !setTag) {
        return;
    }
    setTag(objectPtr, tag ? tag->GetPtr() : nullptr);
}

CResRef* CSWSObject::GetPortrait(CResRef* outResRef) {
    if (!objectPtr || !getPortrait) {
        return nullptr;
    }

    void* resultPtr = getPortrait(objectPtr, outResRef ? outResRef->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

void CSWSObject::SetPortrait(CResRef* portrait) {
    if (!objectPtr || !setPortrait) {
        return;
    }
    setPortrait(objectPtr, portrait ? portrait->GetPtr() : nullptr);
}

WORD CSWSObject::GetPortraitId() {
    if (!objectPtr || !getPortraitId) {
        return 0;
    }
    return getPortraitId(objectPtr);
}

void CSWSObject::SetPortraitId(WORD id) {
    if (!objectPtr || !setPortraitId) {
        return;
    }
    setPortraitId(objectPtr, id);
}

// ===== Location / linkage =====

CScriptLocation* CSWSObject::GetScriptLocation(CScriptLocation* outLocation) {
    // CScriptLocation is a plain struct (Common.h), so it passes straight through.
    if (!objectPtr || !getScriptLocation) {
        return nullptr;
    }

    return static_cast<CScriptLocation*>(getScriptLocation(objectPtr, outLocation));
}

DWORD CSWSObject::GetNearestObjectByName(CExoString* name, float radius) {
    if (!objectPtr || !getNearestObjectByName) {
        return OBJECT_DEFAULT;
    }
    return getNearestObjectByName(objectPtr, name ? name->GetPtr() : nullptr, radius);
}

CSWCObject* CSWSObject::GetClientObject() {
    if (!objectPtr || !getClientObject) {
        return nullptr;
    }

    void* clientPtr = getClientObject(objectPtr);
    if (!clientPtr) {
        return nullptr;
    }

    return new CSWCObject(clientPtr);
}

// ===== Dialog =====

CResRef* CSWSObject::GetDialogResref(CResRef* outResRef) {
    if (!objectPtr || !getDialogResref) {
        return nullptr;
    }

    void* resultPtr = getDialogResref(objectPtr, outResRef ? outResRef->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

void CSWSObject::SetDialogDelay(float delay) {
    if (!objectPtr || !setDialogDelay) {
        return;
    }
    setDialogDelay(objectPtr, delay);
}

void CSWSObject::SetDialogOwner(CSWSObject* owner) {
    if (!objectPtr || !setDialogOwner) {
        return;
    }
    setDialogOwner(objectPtr, owner ? owner->GetPtr() : nullptr);
}

int CSWSObject::StopDialog() {
    if (!objectPtr || !stopDialog) {
        return 0;
    }
    return stopDialog(objectPtr);
}

void CSWSObject::StopSoundPlayingInDialog() {
    if (!objectPtr || !stopSoundPlayingInDialog) {
        return;
    }
    stopSoundPlayingInDialog(objectPtr);
}
