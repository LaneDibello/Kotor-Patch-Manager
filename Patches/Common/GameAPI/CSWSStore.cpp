#include "CSWSStore.h"
#include "CExoString.h"
#include "CExoLocString.h"
#include "GameVersion.h"

int CSWSStore::offsetOnOpenStore = -1;
int CSWSStore::offsetLocName = -1;
int CSWSStore::offsetMarkDown = -1;
int CSWSStore::offsetMarkUp = -1;
int CSWSStore::offsetBuySellFlag = -1;

bool CSWSStore::functionsInitialized = false;
bool CSWSStore::offsetsInitialized = false;

void CSWSStore::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSStore functions wrapped yet
    functionsInitialized = true;
}

void CSWSStore::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSStore] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetOnOpenStore = GameVersion::GetOffset("CSWSStore", "on_open_store");
        offsetLocName = GameVersion::GetOffset("CSWSStore", "loc_name");
        offsetMarkDown = GameVersion::GetOffset("CSWSStore", "mark_down");
        offsetMarkUp = GameVersion::GetOffset("CSWSStore", "mark_up");
        offsetBuySellFlag = GameVersion::GetOffset("CSWSStore", "buy_sell_flag");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSStore] ERROR: %s\n", e.what());
    }
}

CSWSStore::CSWSStore(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSStore::~CSWSStore() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

CExoString* CSWSStore::GetOnOpenStore() {
    if (!objectPtr || offsetOnOpenStore < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetOnOpenStore);
}

CExoLocString* CSWSStore::GetLocName() {
    if (!objectPtr || offsetLocName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocName);
}

int CSWSStore::GetMarkDown() {
    if (!objectPtr || offsetMarkDown < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMarkDown);
}

void CSWSStore::SetMarkDown(int value) {
    if (!objectPtr || offsetMarkDown < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMarkDown, value);
}

int CSWSStore::GetMarkUp() {
    if (!objectPtr || offsetMarkUp < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMarkUp);
}

void CSWSStore::SetMarkUp(int value) {
    if (!objectPtr || offsetMarkUp < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMarkUp, value);
}

short CSWSStore::GetBuySellFlag() {
    if (!objectPtr || offsetBuySellFlag < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetBuySellFlag);
}

void CSWSStore::SetBuySellFlag(short value) {
    if (!objectPtr || offsetBuySellFlag < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetBuySellFlag, value);
}
