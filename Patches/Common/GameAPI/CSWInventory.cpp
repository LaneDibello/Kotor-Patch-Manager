#include "CSWInventory.h"
#include "GameVersion.h"
#include "../Common.h"

CSWInventory::GetItemInSlotFn CSWInventory::getItemInSlot = nullptr;
bool CSWInventory::functionsInitialized = false;

void CSWInventory::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWInventory] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getItemInSlot = reinterpret_cast<GetItemInSlotFn>(
            GameVersion::GetFunctionAddress("CSWInventory", "GetItemInSlot")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWInventory] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

CSWInventory::CSWInventory(void* inventoryPtr)
    : inventoryPtr(inventoryPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

CSWInventory::~CSWInventory() {
    inventoryPtr = nullptr;
}

void* CSWInventory::GetItemInSlot(int slot) {
    if (!inventoryPtr || !getItemInSlot) {
        return nullptr;
    }

    return getItemInSlot(inventoryPtr, slot);
}
