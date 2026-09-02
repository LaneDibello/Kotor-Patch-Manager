#pragma once

#include "../Common.h"
#include "CSWCObject.h"

/// <summary>
/// Wraps a client-side projectile.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCProjectile : public CSWCObject {
public:
    explicit CSWCProjectile(void* objectPtr);
    virtual ~CSWCProjectile();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
