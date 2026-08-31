#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CSWSPlaceable;
class CSWCCreature;

/// <summary>
/// Wraps a client-side placeable object.
/// </summary>
class CSWCPlaceable : public CSWCObject {
public:
    explicit CSWCPlaceable(void* objectPtr);
    virtual ~CSWCPlaceable();

    // Opens the security (slice/unlock) action menu for the given creature.
    void ActionMenuSecurity(CSWCCreature* creature);

    int IsHostile();
    void SetAppearance(BYTE appearance);

    // Returns the paired server placeable as a heap-allocated wrapper; caller owns it.
    CSWSPlaceable* GetServerPlaceable();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void(__thiscall* ActionMenuSecurityFn)(void* thisPtr, void* creature);
    typedef int(__thiscall* IsHostileFn)(void* thisPtr);
    typedef void(__thiscall* SetAppearanceFn)(void* thisPtr, BYTE appearance);
    typedef void* (__thiscall* GetServerPlaceableFn)(void* thisPtr);

    static ActionMenuSecurityFn actionMenuSecurity;
    static IsHostileFn isHostile;
    static SetAppearanceFn setAppearance;
    static GetServerPlaceableFn getServerPlaceable;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
