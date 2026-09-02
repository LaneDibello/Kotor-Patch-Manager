#pragma once

#include "../Common.h"
#include "CSWCObject.h"

/// <summary>
/// Wraps a client-side map pin.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCMapPin : public CSWCObject {
public:
    explicit CSWCMapPin(void* objectPtr);
    virtual ~CSWCMapPin();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
