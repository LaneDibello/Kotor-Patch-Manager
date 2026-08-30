#pragma once

#include "../Common.h"
#include "CSWSObject.h"

/// <summary>
/// Wraps a server-side waypoint.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSWaypoint : public CSWSObject {
public:
    explicit CSWSWaypoint(void* objectPtr);
    virtual ~CSWSWaypoint();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
