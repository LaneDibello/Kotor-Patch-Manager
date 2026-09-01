#include "CSWSCreature.h"
#include "CExoString.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CSWCCreature.h"
#include "CSWSCreatureStats.h"
#include "CSWInventory.h"

CSWSCreature::GetClientCreatureFn CSWSCreature::getClientCreature = nullptr;
bool CSWSCreature::functionsInitialized = false;

int CSWSCreature::offsetCreatureStats = -1;
int CSWSCreature::offsetInventory = -1;
int CSWSCreature::offsetJoiningXP = -1;
int CSWSCreature::offsetScriptHeartbeat = -1;
int CSWSCreature::offsetScriptOnNotice = -1;
int CSWSCreature::offsetScriptSpellAt = -1;
int CSWSCreature::offsetScriptAttacked = -1;
int CSWSCreature::offsetScriptDamaged = -1;
int CSWSCreature::offsetScriptDisturbed = -1;
int CSWSCreature::offsetScriptEndRound = -1;
int CSWSCreature::offsetScriptDialogue = -1;
int CSWSCreature::offsetScriptSpawn = -1;
int CSWSCreature::offsetScriptRested = -1;
int CSWSCreature::offsetScriptDeath = -1;
int CSWSCreature::offsetScriptUserDefine = -1;
int CSWSCreature::offsetScriptOnBlocked = -1;
int CSWSCreature::offsetScriptEndDialogue = -1;
int CSWSCreature::offsetAreaObjectId = -1;
int CSWSCreature::offsetEntryPoint = -1;
int CSWSCreature::offsetBlockingDoorId = -1;
int CSWSCreature::offsetCreateOnScriptFired = -1;
int CSWSCreature::offsetAmbientAnimationState = -1;
int CSWSCreature::offsetDetectMode = -1;
int CSWSCreature::offsetStealthMode = -1;
int CSWSCreature::offsetDisarmable = -1;
int CSWSCreature::offsetCreatureSize = -1;
int CSWSCreature::offsetEffectSpellId = -1;
int CSWSCreature::offsetPrimaryRange = -1;
int CSWSCreature::offsetSecondaryRange = -1;
int CSWSCreature::offsetInvitedToParty = -1;
int CSWSCreature::offsetBaseClassLevel = -1;
int CSWSCreature::offsetSoundSetFile = -1;
int CSWSCreature::offsetBodyBag = -1;
int CSWSCreature::offsetMovementRateFactor = -1;
int CSWSCreature::offsetIsDisguised = -1;
int CSWSCreature::offsetAppearance = -1;
int CSWSCreature::offsetHeartbeatMsRemaining = -1;
bool CSWSCreature::offsetsInitialized = false;

void CSWSCreature::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    // Call base class initialization first
    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getClientCreature = reinterpret_cast<GetClientCreatureFn>(
            GameVersion::GetFunctionAddress("CSWSCreature", "GetClientCreature")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreature] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSCreature::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    // Call base class offset initialization (which includes CSWSObject offsets)
    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // CSWSCreature-specific offsets only
        offsetCreatureStats = GameVersion::GetOffset("CSWSCreature", "CreatureStats");
        offsetInventory = GameVersion::GetOffset("CSWSCreature", "Inventory");
        offsetJoiningXP = GameVersion::GetOffset("CSWSCreature", "joining_xp");
        offsetScriptHeartbeat = GameVersion::GetOffset("CSWSCreature", "script_heartbeat");
        offsetScriptOnNotice = GameVersion::GetOffset("CSWSCreature", "script_on_notice");
        offsetScriptSpellAt = GameVersion::GetOffset("CSWSCreature", "script_spell_at");
        offsetScriptAttacked = GameVersion::GetOffset("CSWSCreature", "script_attacked");
        offsetScriptDamaged = GameVersion::GetOffset("CSWSCreature", "script_damaged");
        offsetScriptDisturbed = GameVersion::GetOffset("CSWSCreature", "script_disturbed");
        offsetScriptEndRound = GameVersion::GetOffset("CSWSCreature", "script_end_round");
        offsetScriptDialogue = GameVersion::GetOffset("CSWSCreature", "script_dialogue");
        offsetScriptSpawn = GameVersion::GetOffset("CSWSCreature", "script_spawn");
        offsetScriptRested = GameVersion::GetOffset("CSWSCreature", "script_rested");
        offsetScriptDeath = GameVersion::GetOffset("CSWSCreature", "script_death");
        offsetScriptUserDefine = GameVersion::GetOffset("CSWSCreature", "script_user_define");
        offsetScriptOnBlocked = GameVersion::GetOffset("CSWSCreature", "script_on_blocked");
        offsetScriptEndDialogue = GameVersion::GetOffset("CSWSCreature", "script_end_dialogue");
        offsetAreaObjectId = GameVersion::GetOffset("CSWSCreature", "area_object_id");
        offsetEntryPoint = GameVersion::GetOffset("CSWSCreature", "entry_point");
        offsetBlockingDoorId = GameVersion::GetOffset("CSWSCreature", "blocking_door_id");
        offsetCreateOnScriptFired = GameVersion::GetOffset("CSWSCreature", "create_on_script_fired");
        offsetAmbientAnimationState = GameVersion::GetOffset("CSWSCreature", "ambient_animation_state");
        offsetDetectMode = GameVersion::GetOffset("CSWSCreature", "detect_mode");
        offsetStealthMode = GameVersion::GetOffset("CSWSCreature", "stealth_mode");
        offsetDisarmable = GameVersion::GetOffset("CSWSCreature", "disarmable");
        offsetCreatureSize = GameVersion::GetOffset("CSWSCreature", "creature_size");
        offsetEffectSpellId = GameVersion::GetOffset("CSWSCreature", "effect_spell_id");
        offsetPrimaryRange = GameVersion::GetOffset("CSWSCreature", "primary_range");
        offsetSecondaryRange = GameVersion::GetOffset("CSWSCreature", "secondary_range");
        offsetInvitedToParty = GameVersion::GetOffset("CSWSCreature", "invited_to_party");
        offsetBaseClassLevel = GameVersion::GetOffset("CSWSCreature", "base_class_level");
        offsetSoundSetFile = GameVersion::GetOffset("CSWSCreature", "sound_set_file");
        offsetBodyBag = GameVersion::GetOffset("CSWSCreature", "body_bag");
        offsetMovementRateFactor = GameVersion::GetOffset("CSWSCreature", "movement_rate_factor");
        offsetIsDisguised = GameVersion::GetOffset("CSWSCreature", "is_disguised");
        offsetAppearance = GameVersion::GetOffset("CSWSCreature", "appearance");
        offsetHeartbeatMsRemaining = GameVersion::GetOffset("CSWSCreature", "heartbeat_ms_remaining");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreature] ERROR: %s\n", e.what());
    }
}

CSWSCreature::CSWSCreature(void* creaturePtr)
    : CSWSObject(creaturePtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSCreature::~CSWSCreature() {
    // Base class destructor will handle objectPtr cleanup
}

CSWCCreature* CSWSCreature::GetClientCreature() {
    if (!objectPtr || !getClientCreature) {
        return nullptr;
    }
    void* clientCreaturePtr = getClientCreature(objectPtr);
    if (!clientCreaturePtr) {
        return nullptr;
    }
    return new CSWCCreature(clientCreaturePtr);
}

CSWSCreatureStats* CSWSCreature::GetCreatureStats() {
    if (!objectPtr || offsetCreatureStats < 0) {
        return nullptr;
    }
    void* statsPtr = getObjectProperty<void*>(objectPtr, offsetCreatureStats);
    if (!statsPtr) {
        return nullptr;
    }
    return new CSWSCreatureStats(statsPtr);
}

CSWInventory* CSWSCreature::GetInventory() {
    if (!objectPtr || offsetInventory < 0) {
        return nullptr;
    }
    void* inventoryPtr = getObjectProperty<void*>(objectPtr, offsetInventory);
    if (!inventoryPtr) {
        return nullptr;
    }
    return new CSWInventory(inventoryPtr);
}

// ===== Offsets =====

int CSWSCreature::GetJoiningXP() {
    if (!objectPtr || offsetJoiningXP < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetJoiningXP);
}

void CSWSCreature::SetJoiningXP(int value) {
    if (!objectPtr || offsetJoiningXP < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetJoiningXP, value);
}

CExoString* CSWSCreature::GetScriptHeartbeat() {
    if (!objectPtr || offsetScriptHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptHeartbeat);
}

CExoString* CSWSCreature::GetScriptOnNotice() {
    if (!objectPtr || offsetScriptOnNotice < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnNotice);
}

CExoString* CSWSCreature::GetScriptSpellAt() {
    if (!objectPtr || offsetScriptSpellAt < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptSpellAt);
}

CExoString* CSWSCreature::GetScriptAttacked() {
    if (!objectPtr || offsetScriptAttacked < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptAttacked);
}

CExoString* CSWSCreature::GetScriptDamaged() {
    if (!objectPtr || offsetScriptDamaged < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptDamaged);
}

CExoString* CSWSCreature::GetScriptDisturbed() {
    if (!objectPtr || offsetScriptDisturbed < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptDisturbed);
}

CExoString* CSWSCreature::GetScriptEndRound() {
    if (!objectPtr || offsetScriptEndRound < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptEndRound);
}

CExoString* CSWSCreature::GetScriptDialogue() {
    if (!objectPtr || offsetScriptDialogue < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptDialogue);
}

CExoString* CSWSCreature::GetScriptSpawn() {
    if (!objectPtr || offsetScriptSpawn < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptSpawn);
}

CExoString* CSWSCreature::GetScriptRested() {
    if (!objectPtr || offsetScriptRested < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptRested);
}

CExoString* CSWSCreature::GetScriptDeath() {
    if (!objectPtr || offsetScriptDeath < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptDeath);
}

CExoString* CSWSCreature::GetScriptUserDefine() {
    if (!objectPtr || offsetScriptUserDefine < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptUserDefine);
}

CExoString* CSWSCreature::GetScriptOnBlocked() {
    if (!objectPtr || offsetScriptOnBlocked < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnBlocked);
}

CExoString* CSWSCreature::GetScriptEndDialogue() {
    if (!objectPtr || offsetScriptEndDialogue < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptEndDialogue);
}

DWORD CSWSCreature::GetAreaObjectId() {
    if (!objectPtr || offsetAreaObjectId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetAreaObjectId);
}

void CSWSCreature::SetAreaObjectId(DWORD value) {
    if (!objectPtr || offsetAreaObjectId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetAreaObjectId, value);
}

Vector CSWSCreature::GetEntryPoint() {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetEntryPoint < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetEntryPoint);
}

void CSWSCreature::SetEntryPoint(const Vector& value) {
    if (!objectPtr || offsetEntryPoint < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetEntryPoint, value);
}

DWORD CSWSCreature::GetBlockingDoorId() {
    if (!objectPtr || offsetBlockingDoorId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetBlockingDoorId);
}

void CSWSCreature::SetBlockingDoorId(DWORD value) {
    if (!objectPtr || offsetBlockingDoorId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetBlockingDoorId, value);
}

int CSWSCreature::GetCreateOnScriptFired() {
    if (!objectPtr || offsetCreateOnScriptFired < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCreateOnScriptFired);
}

void CSWSCreature::SetCreateOnScriptFired(int value) {
    if (!objectPtr || offsetCreateOnScriptFired < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCreateOnScriptFired, value);
}

BYTE CSWSCreature::GetAmbientAnimationState() {
    if (!objectPtr || offsetAmbientAnimationState < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetAmbientAnimationState);
}

void CSWSCreature::SetAmbientAnimationState(BYTE value) {
    if (!objectPtr || offsetAmbientAnimationState < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetAmbientAnimationState, value);
}

BYTE CSWSCreature::GetDetectMode() {
    if (!objectPtr || offsetDetectMode < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetDetectMode);
}

void CSWSCreature::SetDetectMode(BYTE value) {
    if (!objectPtr || offsetDetectMode < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetDetectMode, value);
}

BYTE CSWSCreature::GetStealthMode() {
    if (!objectPtr || offsetStealthMode < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetStealthMode);
}

void CSWSCreature::SetStealthMode(BYTE value) {
    if (!objectPtr || offsetStealthMode < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetStealthMode, value);
}

int CSWSCreature::GetDisarmable() {
    if (!objectPtr || offsetDisarmable < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetDisarmable);
}

void CSWSCreature::SetDisarmable(int value) {
    if (!objectPtr || offsetDisarmable < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetDisarmable, value);
}

int CSWSCreature::GetCreatureSize() {
    if (!objectPtr || offsetCreatureSize < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCreatureSize);
}

void CSWSCreature::SetCreatureSize(int value) {
    if (!objectPtr || offsetCreatureSize < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCreatureSize, value);
}

int CSWSCreature::GetEffectSpellId() {
    if (!objectPtr || offsetEffectSpellId < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetEffectSpellId);
}

void CSWSCreature::SetEffectSpellId(int value) {
    if (!objectPtr || offsetEffectSpellId < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetEffectSpellId, value);
}

float CSWSCreature::GetPrimaryRange() {
    if (!objectPtr || offsetPrimaryRange < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetPrimaryRange);
}

void CSWSCreature::SetPrimaryRange(float value) {
    if (!objectPtr || offsetPrimaryRange < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetPrimaryRange, value);
}

float CSWSCreature::GetSecondaryRange() {
    if (!objectPtr || offsetSecondaryRange < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetSecondaryRange);
}

void CSWSCreature::SetSecondaryRange(float value) {
    if (!objectPtr || offsetSecondaryRange < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetSecondaryRange, value);
}

int CSWSCreature::GetInvitedToParty() {
    if (!objectPtr || offsetInvitedToParty < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetInvitedToParty);
}

void CSWSCreature::SetInvitedToParty(int value) {
    if (!objectPtr || offsetInvitedToParty < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetInvitedToParty, value);
}

int CSWSCreature::GetBaseClassLevel() {
    if (!objectPtr || offsetBaseClassLevel < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBaseClassLevel);
}

void CSWSCreature::SetBaseClassLevel(int value) {
    if (!objectPtr || offsetBaseClassLevel < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetBaseClassLevel, value);
}

short CSWSCreature::GetSoundSetFile() {
    if (!objectPtr || offsetSoundSetFile < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetSoundSetFile);
}

void CSWSCreature::SetSoundSetFile(short value) {
    if (!objectPtr || offsetSoundSetFile < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetSoundSetFile, value);
}

BYTE CSWSCreature::GetBodyBag() {
    if (!objectPtr || offsetBodyBag < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetBodyBag);
}

void CSWSCreature::SetBodyBag(BYTE value) {
    if (!objectPtr || offsetBodyBag < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetBodyBag, value);
}

float CSWSCreature::GetMovementRateFactor() {
    if (!objectPtr || offsetMovementRateFactor < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetMovementRateFactor);
}

void CSWSCreature::SetMovementRateFactor(float value) {
    if (!objectPtr || offsetMovementRateFactor < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetMovementRateFactor, value);
}

int CSWSCreature::GetIsDisguised() {
    if (!objectPtr || offsetIsDisguised < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetIsDisguised);
}

void CSWSCreature::SetIsDisguised(int value) {
    if (!objectPtr || offsetIsDisguised < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetIsDisguised, value);
}

WORD CSWSCreature::GetAppearance() {
    if (!objectPtr || offsetAppearance < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetAppearance);
}

void CSWSCreature::SetAppearance(WORD value) {
    if (!objectPtr || offsetAppearance < 0) {
        return;
    }
    setObjectProperty<WORD>(objectPtr, offsetAppearance, value);
}

int CSWSCreature::GetHeartbeatMsRemaining() {
    if (!objectPtr || offsetHeartbeatMsRemaining < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHeartbeatMsRemaining);
}

void CSWSCreature::SetHeartbeatMsRemaining(int value) {
    if (!objectPtr || offsetHeartbeatMsRemaining < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHeartbeatMsRemaining, value);
}
