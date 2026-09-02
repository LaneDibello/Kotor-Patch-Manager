#pragma once

#include <windows.h>
#include "CGameObject.h"
#include "CExoArrayList.h"

class CSWSObject;
#include "../Common.h"

class CSWCObject;
class CExoString;
class CResRef;
class CExoLocString;

class CSWSObject : public CGameObject {
public:
    explicit CSWSObject(void* objectPtr);
    virtual ~CSWSObject();

    // Action queue management
    void AddActionToFront(
        DWORD param_1, USHORT param_2, DWORD param_3, void* param_4, DWORD param_5,
        void* param_6, DWORD param_7, void* param_8, DWORD param_9, void* param_10, DWORD param_11,
        void* param_12, DWORD param_13, void* param_14, DWORD param_15, void* param_16, DWORD param_17,
        void* param_18, DWORD param_19, void* param_20, DWORD param_21, void* param_22, DWORD param_23,
        void* param_24, DWORD param_25, void* param_26, DWORD param_27, void* param_28);

    // CSWSObject-specific accessors
    Vector GetPosition();
    Vector GetOrientation();
    DWORD GetAreaId();

    void SetPosition(const Vector& position);
    void SetOrientation(const Vector& orientation);
    void SetAreaId(DWORD areaId);

    // Actions
    void ClearAllActions(int includeAttacks);
    int GetAcceptableAction(DWORD action);

    // State
    BYTE GetAIStateReputation(DWORD objectId);
    int GetDead();
    int GetHasFeatEffectApplied(WORD feat);
    int HasSpellEffectApplied(int spellId);
    void SetAnimation(int animation);
    void SetCurrentHitPoints(int currentHP);
    void SetKeepCorpse(int keepCorpse);
    int SpawnBodyBag();

    // Identity / naming
    // Returns a heap-allocated wrapper; caller owns it.
    CExoLocString* GetLastName();
    void SetTag(CExoString* tag);
    // Fills outResRef and returns it wrapped; caller owns the returned wrapper.
    CResRef* GetPortrait(CResRef* outResRef);
    void SetPortrait(CResRef* portrait);
    WORD GetPortraitId();
    void SetPortraitId(WORD id);

    // Location / linkage
    // Fills outLocation (a plain CScriptLocation from Common.h) and returns it.
    CScriptLocation* GetScriptLocation(CScriptLocation* outLocation);
    DWORD GetNearestObjectByName(CExoString* name, float radius);
    // Returns the paired client object as a heap-allocated wrapper; caller owns it.
    CSWCObject* GetClientObject();

    // Dialog
    // NOTE: the address DB records this as __stdcall, but it is really __thiscall.
    // The function never touches ECX, so Ghidra could not see the 'this' pointer.
    // Either convention pushes one arg and returns with ret 4, so the two are
    // interchangeable here; __thiscall is used because it is what the game does.
    CResRef* GetDialogResref(CResRef* outResRef);
    void SetDialogDelay(float delay);
    void SetDialogOwner(CSWSObject* owner);
    int StopDialog();
    void StopSoundPlayingInDialog();


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetTag();
    DWORD GetDlgDelayDay();
    void SetDlgDelayDay(DWORD value);
    DWORD GetDlgDelayMS();
    void SetDlgDelayMS(DWORD value);
    DWORD GetDlgLastTimeDay();
    void SetDlgLastTimeDay(DWORD value);
    DWORD GetDlgLastTimeMS();
    void SetDlgLastTimeMS(DWORD value);
    // Returns a heap-allocated wrapper; caller owns it.
    CSWSObject* GetDialogOwner();
    int GetAILevel();
    void SetAILevel(int value);
    int GetAnimation();
    int GetHitPoints();
    int GetIsDestroyable();
    void SetIsDestroyable(int value);
    int GetIsRaiseable();
    void SetIsRaiseable(int value);
    int GetDeadSelectable();
    void SetDeadSelectable(int value);
    DWORD GetPlot();
    void SetPlot(DWORD value);
    // Wrapper over the embedded list; caller owns the wrapper, not the memory.
    CExoArrayList<DWORD>* GetEffectTargets();
    int GetListening();
    void SetListening(int value);
    // Wrapper over the embedded list; caller owns the wrapper, not the memory.
    CExoArrayList<CExoString*>* GetMatchedExpressionStrings();
    Vector GetSpellTargetPosition();
    void SetSpellTargetPosition(const Vector& value);
    int GetMin1HP();
    void SetMin1HP(int value);
    int GetPartyInteract();
    void SetPartyInteract(int value);
    int GetReorienting();
    void SetReorienting(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* AddActionToFrontFn)(
        void* thisPtr, DWORD param_1, USHORT param_2, DWORD param_3, void* param_4, DWORD param_5,
        void* param_6, DWORD param_7, void* param_8, DWORD param_9, void* param_10, DWORD param_11,
        void* param_12, DWORD param_13, void* param_14, DWORD param_15, void* param_16, DWORD param_17,
        void* param_18, DWORD param_19, void* param_20, DWORD param_21, void* param_22, DWORD param_23,
        void* param_24, DWORD param_25, void* param_26, DWORD param_27, void* param_28);

    typedef void(__thiscall* ClearAllActionsFn)(void* thisPtr, int includeAttacks);
    typedef int(__thiscall* GetAcceptableActionFn)(void* thisPtr, DWORD action);
    typedef BYTE(__thiscall* GetAIStateReputationFn)(void* thisPtr, DWORD objectId);
    typedef int(__thiscall* GetDeadFn)(void* thisPtr);
    typedef int(__thiscall* GetHasFeatEffectAppliedFn)(void* thisPtr, WORD feat);
    typedef int(__thiscall* HasSpellEffectAppliedFn)(void* thisPtr, int spellId);
    typedef void(__thiscall* SetAnimationFn)(void* thisPtr, int animation);
    typedef void(__thiscall* SetCurrentHitPointsFn)(void* thisPtr, int currentHP);
    typedef void(__thiscall* SetKeepCorpseFn)(void* thisPtr, int keepCorpse);
    typedef int(__thiscall* SpawnBodyBagFn)(void* thisPtr);

    typedef void* (__thiscall* GetLastNameFn)(void* thisPtr);
    typedef void(__thiscall* SetTagFn)(void* thisPtr, void* tag);
    typedef void* (__thiscall* GetPortraitFn)(void* thisPtr, void* outResRef);
    typedef void(__thiscall* SetPortraitFn)(void* thisPtr, void* portrait);
    typedef WORD(__thiscall* GetPortraitIdFn)(void* thisPtr);
    typedef void(__thiscall* SetPortraitIdFn)(void* thisPtr, WORD id);

    typedef void* (__thiscall* GetScriptLocationFn)(void* thisPtr, void* outLocation);
    typedef DWORD(__thiscall* GetNearestObjectByNameFn)(void* thisPtr, void* name, float radius);
    typedef void* (__thiscall* GetClientObjectFn)(void* thisPtr);

    typedef void* (__thiscall* GetDialogResrefFn)(void* thisPtr, void* outResRef);
    typedef void(__thiscall* SetDialogDelayFn)(void* thisPtr, float delay);
    typedef void(__thiscall* SetDialogOwnerFn)(void* thisPtr, void* owner);
    typedef int(__thiscall* StopDialogFn)(void* thisPtr);
    typedef void(__thiscall* StopSoundPlayingInDialogFn)(void* thisPtr);

    static AddActionToFrontFn addActionToFront;

    static ClearAllActionsFn clearAllActions;
    static GetAcceptableActionFn getAcceptableAction;
    static GetAIStateReputationFn getAIStateReputation;
    static GetDeadFn getDead;
    static GetHasFeatEffectAppliedFn getHasFeatEffectApplied;
    static HasSpellEffectAppliedFn hasSpellEffectApplied;
    static SetAnimationFn setAnimation;
    static SetCurrentHitPointsFn setCurrentHitPoints;
    static SetKeepCorpseFn setKeepCorpse;
    static SpawnBodyBagFn spawnBodyBag;

    static GetLastNameFn getLastName;
    static SetTagFn setTag;
    static GetPortraitFn getPortrait;
    static SetPortraitFn setPortrait;
    static GetPortraitIdFn getPortraitId;
    static SetPortraitIdFn setPortraitId;

    static GetScriptLocationFn getScriptLocation;
    static GetNearestObjectByNameFn getNearestObjectByName;
    static GetClientObjectFn getClientObject;

    static GetDialogResrefFn getDialogResref;
    static SetDialogDelayFn setDialogDelay;
    static SetDialogOwnerFn setDialogOwner;
    static StopDialogFn stopDialog;
    static StopSoundPlayingInDialogFn stopSoundPlayingInDialog;

    static int offsetTag;
    static int offsetDlgDelayDay;
    static int offsetDlgDelayMS;
    static int offsetDlgLastTimeDay;
    static int offsetDlgLastTimeMS;
    static int offsetDialogOwner;
    static int offsetAILevel;
    static int offsetAnimation;
    static int offsetHitPoints;
    static int offsetIsDestroyable;
    static int offsetIsRaiseable;
    static int offsetDeadSelectable;
    static int offsetPlot;
    static int offsetEffectTargets;
    static int offsetListening;
    static int offsetMatchedExpressionStrings;
    static int offsetSpellTargetPosition;
    static int offsetMin1HP;
    static int offsetPartyInteract;
    static int offsetReorienting;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetPosition;
    static int offsetOrientation;
    static int offsetAreaId;
};
