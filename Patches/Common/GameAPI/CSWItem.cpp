#include "CSWItem.h"
#include "GameVersion.h"
#include "../Common.h"

CSWItem::GetBaseItemFn CSWItem::getBaseItem = nullptr;
bool CSWItem::functionsInitialized = false;

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

CSWItem::CSWItem(void* itemPtr)
    : itemPtr(itemPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

CSWItem::~CSWItem() {
    itemPtr = nullptr;
}

WORD CSWItem::GetBaseItem() {
    if (!itemPtr || !getBaseItem) {
        return 0;
    }

    return getBaseItem(itemPtr);
}
