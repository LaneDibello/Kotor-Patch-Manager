#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoLocString;
class CExoString;

class CSWCPlaceable;
class CResRef;

/// <summary>
/// Wraps a server-side placeable object (containers, computer panels, footlockers).
/// </summary>
class CSWSPlaceable : public CSWSObject {
public:
    explicit CSWSPlaceable(void* objectPtr);
    virtual ~CSWSPlaceable();

    // State
    BYTE GetBodyBagAppearance();
    int GetDead();
    DWORD GetEffectSpellId();
    void SetEffectSpellId(DWORD spellId);
    int GetLightIsOn();
    void SetLightIsOn(int lightState);

    // Fills outResref and returns it wrapped; caller owns the returned wrapper.
    CResRef* GetDialogResref(CResRef* outResref);

    // Returns the paired client placeable as a heap-allocated wrapper; caller owns it.
    CSWCPlaceable* GetClientPlaceable();

    void RemoveFromArea();

    // Placeables orient by quaternion, unlike the Vector-based CSWSObject setter.
    // The base overload stays reachable via the using-declaration below.
    using CSWSObject::SetOrientation;
    void SetOrientation(Quaternion* orientation);


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocName();
    DWORD GetAppearance();
    void SetAppearance(DWORD value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetDescription();
    DWORD GetFaction();
    void SetFaction(DWORD value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CResRef* GetConversation();
    int GetHardness();
    void SetHardness(int value);
    int GetLocked();
    void SetLocked(int value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetKeyName();
    int GetKeyRequired();
    void SetKeyRequired(int value);
    int GetAutoRemoveKey();
    void SetAutoRemoveKey(int value);
    short GetOpenLockDC();
    void SetOpenLockDC(short value);
    short GetTrapDetectDC();
    void SetTrapDetectDC(short value);
    int GetTrapFlag();
    void SetTrapFlag(int value);
    int GetDisarmDC();
    void SetDisarmDC(int value);
    int GetTrapDisarmable();
    void SetTrapDisarmable(int value);
    int GetTrapDetectable();
    void SetTrapDetectable(int value);
    int GetTrapOneShot();
    void SetTrapOneShot(int value);
    int GetTrapType();
    void SetTrapType(int value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnClosed();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDamaged();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDeath();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDisarm();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnHeartbeat();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnInventoryDisturbed();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnLock();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnMeleeAttacked();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnOpen();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnSpellCastAt();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnTrapTriggered();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUnlock();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUsed();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUserDefined();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDialog();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnEndDialog();
    BYTE GetFortitude();
    void SetFortitude(BYTE value);
    BYTE GetWill();
    void SetWill(BYTE value);
    BYTE GetReflex();
    void SetReflex(BYTE value);
    int GetUsable();
    void SetUsable(int value);
    int GetLockable();
    void SetLockable(int value);
    int GetHasInventory();
    void SetHasInventory(int value);
    int GetOpen();
    void SetOpen(int value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CResRef* GetTemplateResRef();
    int GetBodyBag();
    void SetBodyBag(int value);
    Quaternion GetOrientationQuat();
    int GetIsBodyBag();
    void SetIsBodyBag(int value);
    int GetIsCorpse();
    void SetIsCorpse(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef BYTE(__thiscall* GetBodyBagAppearanceFn)(void* thisPtr);
    typedef int(__thiscall* GetDeadFn)(void* thisPtr);
    typedef DWORD(__thiscall* GetEffectSpellIdFn)(void* thisPtr);
    typedef void(__thiscall* SetEffectSpellIdFn)(void* thisPtr, DWORD spellId);
    typedef int(__thiscall* GetLightIsOnFn)(void* thisPtr);
    typedef void(__thiscall* SetLightIsOnFn)(void* thisPtr, int lightState);
    typedef void* (__thiscall* GetDialogResrefFn)(void* thisPtr, void* outResref);
    typedef void* (__thiscall* GetClientPlaceableFn)(void* thisPtr);
    typedef void(__thiscall* RemoveFromAreaFn)(void* thisPtr);
    typedef void(__thiscall* SetOrientationFn)(void* thisPtr, void* orientation);

    static GetBodyBagAppearanceFn getBodyBagAppearance;
    static GetDeadFn getDead;
    static GetEffectSpellIdFn getEffectSpellId;
    static SetEffectSpellIdFn setEffectSpellId;
    static GetLightIsOnFn getLightIsOn;
    static SetLightIsOnFn setLightIsOn;
    static GetDialogResrefFn getDialogResref;
    static GetClientPlaceableFn getClientPlaceable;
    static RemoveFromAreaFn removeFromArea;
    static SetOrientationFn setOrientation;

    static int offsetLocName;
    static int offsetAppearance;
    static int offsetDescription;
    static int offsetFaction;
    static int offsetConversation;
    static int offsetHardness;
    static int offsetLocked;
    static int offsetKeyName;
    static int offsetKeyRequired;
    static int offsetAutoRemoveKey;
    static int offsetOpenLockDC;
    static int offsetTrapDetectDC;
    static int offsetTrapFlag;
    static int offsetDisarmDC;
    static int offsetTrapDisarmable;
    static int offsetTrapDetectable;
    static int offsetTrapOneShot;
    static int offsetTrapType;
    static int offsetScriptOnClosed;
    static int offsetScriptOnDamaged;
    static int offsetScriptOnDeath;
    static int offsetScriptOnDisarm;
    static int offsetScriptOnHeartbeat;
    static int offsetScriptOnInventoryDisturbed;
    static int offsetScriptOnLock;
    static int offsetScriptOnMeleeAttacked;
    static int offsetScriptOnOpen;
    static int offsetScriptOnSpellCastAt;
    static int offsetScriptOnTrapTriggered;
    static int offsetScriptOnUnlock;
    static int offsetScriptOnUsed;
    static int offsetScriptOnUserDefined;
    static int offsetScriptOnDialog;
    static int offsetScriptOnEndDialog;
    static int offsetFortitude;
    static int offsetWill;
    static int offsetReflex;
    static int offsetUsable;
    static int offsetLockable;
    static int offsetHasInventory;
    static int offsetOpen;
    static int offsetTemplateResRef;
    static int offsetBodyBag;
    static int offsetOrientationQuat;
    static int offsetIsBodyBag;
    static int offsetIsCorpse;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
