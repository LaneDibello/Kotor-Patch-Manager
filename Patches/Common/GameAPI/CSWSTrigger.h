#pragma once

#include "../Common.h"
#include "CSWSObject.h"

/// <summary>
/// Wraps a server-side trigger volume (area transitions, traps, generic triggers).
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSTrigger : public CSWSObject {
public:
    explicit CSWSTrigger(void* objectPtr);
    virtual ~CSWSTrigger();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
