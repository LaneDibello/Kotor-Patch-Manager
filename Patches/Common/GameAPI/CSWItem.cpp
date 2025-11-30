#include "CSWItem.h"
#include "GameVersion.h"
#include "../Common.h"

CSWItem::GetBaseItemFn CSWItem::getBaseItem = nullptr;
bool CSWItem::functionsInitialized = false;
bool CSWItem::offsetsInitialized = false;

void CSWItem::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWItem] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getBaseItem = reinterpret_cast<GetBaseItemFn>(
            GameVersion::GetFunctionAddress("CSWItem", "GetBaseItem")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWItem] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWItem::InitializeOffsets() {
    // CSWItem has no offsets
    offsetsInitialized = true;
}

CSWItem::CSWItem(void* itemPtr)
    : GameAPIObject(itemPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWItem::~CSWItem() {
    // Base class destructor handles objectPtr cleanup
}

WORD CSWItem::GetBaseItem() {
    if (!objectPtr || !getBaseItem) {
        return 0;
    }

    return getBaseItem(objectPtr);
}
