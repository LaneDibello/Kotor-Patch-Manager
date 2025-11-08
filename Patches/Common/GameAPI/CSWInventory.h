#pragma once

#include <windows.h>

class CSWInventory {
public:
    explicit CSWInventory(void* inventoryPtr);
    ~CSWInventory();

    void* GetItemInSlot(int slot);
    void* GetPtr() const { return inventoryPtr; }

private:
    void* inventoryPtr;

    typedef void* (__thiscall* GetItemInSlotFn)(void* thisPtr, int slot);

    static GetItemInSlotFn getItemInSlot;
    static void InitializeFunctions();
    static bool functionsInitialized;
};
