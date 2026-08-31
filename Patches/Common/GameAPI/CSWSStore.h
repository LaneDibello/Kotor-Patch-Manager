#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoLocString;
class CExoString;

/// <summary>
/// Wraps a server-side merchant store.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSStore : public CSWSObject {
public:
    explicit CSWSStore(void* objectPtr);
    virtual ~CSWSStore();


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetOnOpenStore();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocName();
    int GetMarkDown();
    void SetMarkDown(int value);
    int GetMarkUp();
    void SetMarkUp(int value);
    short GetBuySellFlag();
    void SetBuySellFlag(short value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetOnOpenStore;
    static int offsetLocName;
    static int offsetMarkDown;
    static int offsetMarkUp;
    static int offsetBuySellFlag;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
