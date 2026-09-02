#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoLocString;

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


    // ===== Offsets =====
    int GetHasMapNote();
    void SetHasMapNote(int value);
    int GetMapNoteEnabled();
    void SetMapNoteEnabled(int value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetMapNote();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocalizedName();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetHasMapNote;
    static int offsetMapNoteEnabled;
    static int offsetMapNote;
    static int offsetLocalizedName;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
