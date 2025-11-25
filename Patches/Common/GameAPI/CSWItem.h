#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CSWItem : public GameAPIObject {
public:
    explicit CSWItem(void* itemPtr);
    ~CSWItem();

    WORD GetBaseItem();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef WORD (__thiscall* GetBaseItemFn)(void* thisPtr);

    static GetBaseItemFn getBaseItem;
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
