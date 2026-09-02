#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CExoString;

/// <summary>
/// Wraps a client-side trigger volume.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCTrigger : public CSWCObject {
public:
    explicit CSWCTrigger(void* objectPtr);
    virtual ~CSWCTrigger();


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetName();
    int GetCursorId();
    void SetCursorId(int value);
    Vector* GetGeometry();
    void SetGeometry(Vector* value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetName;
    static int offsetCursorId;
    static int offsetGeometry;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
