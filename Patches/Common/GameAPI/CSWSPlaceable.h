#pragma once

#include "../Common.h"
#include "CSWSObject.h"

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

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
