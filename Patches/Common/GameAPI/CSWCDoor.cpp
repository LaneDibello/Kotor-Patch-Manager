#include "CSWCDoor.h"
#include "GameVersion.h"
#include "CSWSDoor.h"
#include "CExoString.h"

CSWCDoor::GetIsOpenFn CSWCDoor::getIsOpen = nullptr;
CSWCDoor::GetModelNameFn CSWCDoor::getModelName = nullptr;
CSWCDoor::GetNameFn CSWCDoor::getName = nullptr;
CSWCDoor::GetServerDoorFn CSWCDoor::getServerDoor = nullptr;
CSWCDoor::SetIsAreaTransitionFn CSWCDoor::setIsAreaTransition = nullptr;
CSWCDoor::SetStateFn CSWCDoor::setState = nullptr;
CSWCDoor::UpdateAreaTransitionDisplayFn CSWCDoor::updateAreaTransitionDisplay = nullptr;

int CSWCDoor::offsetIsHostile = -1;
int CSWCDoor::offsetAppearance = -1;
int CSWCDoor::offsetDoorType = -1;
int CSWCDoor::offsetIsTransition = -1;

bool CSWCDoor::functionsInitialized = false;
bool CSWCDoor::offsetsInitialized = false;

void CSWCDoor::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCDoor] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getIsOpen = reinterpret_cast<GetIsOpenFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "GetIsOpen"));
        getModelName = reinterpret_cast<GetModelNameFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "GetModelName"));
        getName = reinterpret_cast<GetNameFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "GetName"));
        getServerDoor = reinterpret_cast<GetServerDoorFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "GetServerDoor"));
        setIsAreaTransition = reinterpret_cast<SetIsAreaTransitionFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "SetIsAreaTransition"));
        setState = reinterpret_cast<SetStateFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "SetState"));
        updateAreaTransitionDisplay = reinterpret_cast<UpdateAreaTransitionDisplayFn>(
            GameVersion::GetFunctionAddress("CSWCDoor", "UpdateAreaTransitionDisplay"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCDoor] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWCDoor::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCDoor] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetIsHostile = GameVersion::GetOffset("CSWCDoor", "is_hostile");
        offsetAppearance = GameVersion::GetOffset("CSWCDoor", "appearance");
        offsetDoorType = GameVersion::GetOffset("CSWCDoor", "door_type");
        offsetIsTransition = GameVersion::GetOffset("CSWCDoor", "is_transition");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCDoor] ERROR: %s\n", e.what());
    }
}

CSWCDoor::CSWCDoor(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCDoor::~CSWCDoor() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

int CSWCDoor::GetIsOpen() {
    if (!objectPtr || !getIsOpen) {
        return 0;
    }
    return getIsOpen(objectPtr);
}

CExoString* CSWCDoor::GetModelName(CExoString* outName) {
    if (!objectPtr || !getModelName) {
        return nullptr;
    }

    void* resultPtr = getModelName(objectPtr, outName ? outName->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoString(resultPtr);
}

CExoString* CSWCDoor::GetName(CExoString* outName) {
    if (!objectPtr || !getName) {
        return nullptr;
    }

    void* resultPtr = getName(objectPtr, outName ? outName->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoString(resultPtr);
}

CSWSDoor* CSWCDoor::GetServerDoor() {
    if (!objectPtr || !getServerDoor) {
        return nullptr;
    }

    void* serverPtr = getServerDoor(objectPtr);
    if (!serverPtr) {
        return nullptr;
    }

    return new CSWSDoor(serverPtr);
}

void CSWCDoor::SetIsAreaTransition(int isTransition) {
    if (!objectPtr || !setIsAreaTransition) {
        return;
    }
    setIsAreaTransition(objectPtr, isTransition);
}

void CSWCDoor::SetState(BYTE state) {
    if (!objectPtr || !setState) {
        return;
    }
    setState(objectPtr, state);
}

void CSWCDoor::UpdateAreaTransitionDisplay() {
    if (!objectPtr || !updateAreaTransitionDisplay) {
        return;
    }
    updateAreaTransitionDisplay(objectPtr);
}

// ===== Offsets =====

int CSWCDoor::GetIsHostile() {
    if (!objectPtr || offsetIsHostile < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetIsHostile);
}

void CSWCDoor::SetIsHostile(int value) {
    if (!objectPtr || offsetIsHostile < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetIsHostile, value);
}

BYTE CSWCDoor::GetAppearance() {
    if (!objectPtr || offsetAppearance < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetAppearance);
}

void CSWCDoor::SetAppearance(BYTE value) {
    if (!objectPtr || offsetAppearance < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetAppearance, value);
}

BYTE CSWCDoor::GetDoorType() {
    if (!objectPtr || offsetDoorType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetDoorType);
}

void CSWCDoor::SetDoorType(BYTE value) {
    if (!objectPtr || offsetDoorType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetDoorType, value);
}

int CSWCDoor::GetIsTransition() {
    if (!objectPtr || offsetIsTransition < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetIsTransition);
}
