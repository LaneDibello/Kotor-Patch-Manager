#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CSWInventory : public GameAPIObject {
public:
    explicit CSWInventory(void* inventoryPtr);
    ~CSWInventory();

    void* GetItemInSlot(int slot);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void* (__thiscall* GetItemInSlotFn)(void* thisPtr, int slot);

    static GetItemInSlotFn getItemInSlot;
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
