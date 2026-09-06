#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "CSWGuiExtent.h"
#include "../Common.h"
#include "../VTableOverride.h"

class CSWGuiObject : public GameAPIObject {
public:
    explicit CSWGuiObject(void* objectPtr);
    ~CSWGuiObject();

    // Accessors
    CSWGuiExtent GetExtent();
    void SetExtent(const CSWGuiExtent& extent);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

    // Number of entries in this class's game vtable, for the currently-detected
    // game version, or -1 if the layout is unsupported (in which case vtable
    // overriding is disabled). Every class that wants to override a virtual
    // overrides this; the base refuses, since it has no layout of its own.
    //
    // Over-reporting is harmless (we copy a few extra pointers we never dispatch),
    // under-reporting is fatal (the game would dispatch past our buffer), so err high.
    virtual int VTableSlotCount();

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetExtent;

    // Per-instance vtable override (null until the first Override* call). Owned;
    // destroyed before the game object so its original vtable is restored first.
    // See VTableOverride.h for the copy-and-back-pointer scheme.
    VTableOverride* vtableOverride = nullptr;

    // Lazily creates the per-instance vtable override on first use. Returns false
    // if this class's vtable layout is unsupported on the running game version.
    bool EnsureVTableOverride();

    // Puts the game's original vtable back and frees our copy. Idempotent.
    // A derived destructor that runs the game's destructor / frees objectPtr must
    // call this FIRST -- our base destructor runs last, by which time the object
    // would already be gone.
    void RestoreVTable();
};
