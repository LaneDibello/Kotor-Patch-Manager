#pragma once

#include "../Common.h"
#include "CSWSObject.h"

/// <summary>
/// Wraps a server-side door.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSDoor : public CSWSObject {
public:
    explicit CSWSDoor(void* objectPtr);
    virtual ~CSWSDoor();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
