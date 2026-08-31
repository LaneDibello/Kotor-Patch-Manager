#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

/// <summary>
/// Wraps a server-side area-of-effect object (persistent spell effects).
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSAreaOfEffectObject : public CSWSObject {
public:
    explicit CSWSAreaOfEffectObject(void* objectPtr);
    virtual ~CSWSAreaOfEffectObject();


    // ===== Offsets =====
    BYTE GetShape();
    void SetShape(BYTE value);
    DWORD GetSpellId();
    void SetSpellId(DWORD value);
    Vector* GetCorners();
    void SetCorners(Vector* value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnHeartbeat();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUserDefined();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnEnter();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnExit();
    BYTE GetDurationType();
    void SetDurationType(BYTE value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetShape;
    static int offsetSpellId;
    static int offsetCorners;
    static int offsetScriptOnHeartbeat;
    static int offsetScriptOnUserDefined;
    static int offsetScriptOnEnter;
    static int offsetScriptOnExit;
    static int offsetDurationType;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
