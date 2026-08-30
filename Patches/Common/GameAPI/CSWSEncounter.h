#pragma once

#include "../Common.h"
#include "CSWSObject.h"

/// <summary>
/// Wraps a server-side encounter spawner.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSEncounter : public CSWSObject {
public:
    explicit CSWSEncounter(void* objectPtr);
    virtual ~CSWSEncounter();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
