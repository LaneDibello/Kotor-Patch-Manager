#pragma once

#include <windows.h>

class CSWItem {
public:
    explicit CSWItem(void* itemPtr);
    ~CSWItem();

    WORD GetBaseItem();
    void* GetPtr() const { return itemPtr; }

private:
    void* itemPtr;

    typedef WORD (__thiscall* GetBaseItemFn)(void* thisPtr);

    static GetBaseItemFn getBaseItem;
    static void InitializeFunctions();
    static bool functionsInitialized;
};
