#pragma once

#include "../Common.h"
#include "CSWCObject.h"

/// <summary>
/// Wraps a client-side placeable object.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCPlaceable : public CSWCObject {
public:
    explicit CSWCPlaceable(void* objectPtr);
    virtual ~CSWCPlaceable();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
