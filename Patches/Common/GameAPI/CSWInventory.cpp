#include "CSWInventory.h"
#include "GameVersion.h"
#include "../Common.h"

CSWInventory::GetItemInSlotFn CSWInventory::getItemInSlot = nullptr;
bool CSWInventory::functionsInitialized = false;
bool CSWInventory::offsetsInitialized = false;

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

void CSWInventory::InitializeOffsets() {
    // CSWInventory has no offsets
    offsetsInitialized = true;
}

CSWInventory::CSWInventory(void* inventoryPtr)
    : GameAPIObject(inventoryPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWInventory::~CSWInventory() {
    // Base class destructor handles objectPtr cleanup
}

void* CSWInventory::GetItemInSlot(int slot) {
    if (!objectPtr || !getItemInSlot) {
        return nullptr;
    }

    return getItemInSlot(objectPtr, slot);
}
