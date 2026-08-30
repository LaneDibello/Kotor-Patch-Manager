#pragma once

#include "../Common.h"
#include "CSWCObject.h"

/// <summary>
/// Wraps a client-side area-of-effect object.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCAreaOfEffectObject : public CSWCObject {
public:
    explicit CSWCAreaOfEffectObject(void* objectPtr);
    virtual ~CSWCAreaOfEffectObject();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
