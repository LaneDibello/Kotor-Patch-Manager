#pragma once

#include "../Common.h"
#include "CGameObject.h"

/// <summary>
/// Wraps the client-side root of the game object hierarchy. Peer of CSWSObject;
/// holds the shared client state (position, orientation, portrait, model, visual
/// effects) and a back-pointer to its server counterpart.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCObject : public CGameObject {
public:
    explicit CSWCObject(void* objectPtr);
    virtual ~CSWCObject();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
