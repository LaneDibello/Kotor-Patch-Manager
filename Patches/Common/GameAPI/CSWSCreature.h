#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

class CSWCCreature;
class CSWSCreatureStats;
class CSWInventory;

class CSWSCreature : public CSWSObject {
public:
    explicit CSWSCreature(void* creaturePtr);
    virtual ~CSWSCreature();

    CSWCCreature* GetClientCreature();

    CSWSCreatureStats* GetCreatureStats();
    CSWInventory* GetInventory();


    // ===== Offsets =====
    int GetJoiningXP();
    void SetJoiningXP(int value);
    CExoString* GetScriptHeartbeat();
    CExoString* GetScriptOnNotice();
    CExoString* GetScriptSpellAt();
    CExoString* GetScriptAttacked();
    CExoString* GetScriptDamaged();
    CExoString* GetScriptDisturbed();
    CExoString* GetScriptEndRound();
    CExoString* GetScriptDialogue();
    CExoString* GetScriptSpawn();
    CExoString* GetScriptRested();
    CExoString* GetScriptDeath();
    CExoString* GetScriptUserDefine();
    CExoString* GetScriptOnBlocked();
    CExoString* GetScriptEndDialogue();
    DWORD GetAreaObjectId();
    void SetAreaObjectId(DWORD value);
    Vector GetEntryPoint();
    void SetEntryPoint(const Vector& value);
    DWORD GetBlockingDoorId();
    void SetBlockingDoorId(DWORD value);
    int GetCreateOnScriptFired();
    void SetCreateOnScriptFired(int value);
    BYTE GetAmbientAnimationState();
    void SetAmbientAnimationState(BYTE value);
    BYTE GetDetectMode();
    void SetDetectMode(BYTE value);
    BYTE GetStealthMode();
    void SetStealthMode(BYTE value);
    int GetDisarmable();
    void SetDisarmable(int value);
    int GetCreatureSize();
    void SetCreatureSize(int value);
    int GetEffectSpellId();
    void SetEffectSpellId(int value);
    float GetPrimaryRange();
    void SetPrimaryRange(float value);
    float GetSecondaryRange();
    void SetSecondaryRange(float value);
    int GetInvitedToParty();
    void SetInvitedToParty(int value);
    int GetBaseClassLevel();
    void SetBaseClassLevel(int value);
    short GetSoundSetFile();
    void SetSoundSetFile(short value);
    BYTE GetBodyBag();
    void SetBodyBag(BYTE value);
    float GetMovementRateFactor();
    void SetMovementRateFactor(float value);
    int GetIsDisguised();
    void SetIsDisguised(int value);
    WORD GetAppearance();
    void SetAppearance(WORD value);
    int GetHeartbeatMsRemaining();
    void SetHeartbeatMsRemaining(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:

    typedef void* (__thiscall* GetClientCreatureFn)(void* thisPtr);

    static GetClientCreatureFn getClientCreature;
    static int offsetJoiningXP;
    static int offsetScriptHeartbeat;
    static int offsetScriptOnNotice;
    static int offsetScriptSpellAt;
    static int offsetScriptAttacked;
    static int offsetScriptDamaged;
    static int offsetScriptDisturbed;
    static int offsetScriptEndRound;
    static int offsetScriptDialogue;
    static int offsetScriptSpawn;
    static int offsetScriptRested;
    static int offsetScriptDeath;
    static int offsetScriptUserDefine;
    static int offsetScriptOnBlocked;
    static int offsetScriptEndDialogue;
    static int offsetAreaObjectId;
    static int offsetEntryPoint;
    static int offsetBlockingDoorId;
    static int offsetCreateOnScriptFired;
    static int offsetAmbientAnimationState;
    static int offsetDetectMode;
    static int offsetStealthMode;
    static int offsetDisarmable;
    static int offsetCreatureSize;
    static int offsetEffectSpellId;
    static int offsetPrimaryRange;
    static int offsetSecondaryRange;
    static int offsetInvitedToParty;
    static int offsetBaseClassLevel;
    static int offsetSoundSetFile;
    static int offsetBodyBag;
    static int offsetMovementRateFactor;
    static int offsetIsDisguised;
    static int offsetAppearance;
    static int offsetHeartbeatMsRemaining;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetCreatureStats;
    static int offsetInventory;
};
