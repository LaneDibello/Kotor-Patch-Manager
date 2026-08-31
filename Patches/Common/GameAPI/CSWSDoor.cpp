#include "CSWSDoor.h"
#include "GameVersion.h"
#include "CResRef.h"
#include "CExoLocString.h"

CSWSDoor::GetDialogResrefFn CSWSDoor::getDialogResref = nullptr;
CSWSDoor::GetFirstNameFn CSWSDoor::getFirstName = nullptr;
CSWSDoor::GetIsLinkedFn CSWSDoor::getIsLinked = nullptr;
CSWSDoor::GetLinkedObjectFn CSWSDoor::getLinkedObject = nullptr;
CSWSDoor::InDoorFn CSWSDoor::inDoor = nullptr;
CSWSDoor::MoveToNextOpenStateFn CSWSDoor::moveToNextOpenState = nullptr;
CSWSDoor::RemoveFromAreaFn CSWSDoor::removeFromArea = nullptr;

bool CSWSDoor::functionsInitialized = false;
bool CSWSDoor::offsetsInitialized = false;

void CSWSDoor::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSDoor] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getDialogResref = reinterpret_cast<GetDialogResrefFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetDialogResref"));
        getFirstName = reinterpret_cast<GetFirstNameFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetFirstName"));
        getIsLinked = reinterpret_cast<GetIsLinkedFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetIsLinked"));
        getLinkedObject = reinterpret_cast<GetLinkedObjectFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "GetLinkedObject"));
        inDoor = reinterpret_cast<InDoorFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "InDoor"));
        moveToNextOpenState = reinterpret_cast<MoveToNextOpenStateFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "MoveToNextOpenState"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSDoor", "RemoveFromArea"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSDoor] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSDoor::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSDoor offsets wrapped yet
    offsetsInitialized = true;
}

CSWSDoor::CSWSDoor(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSDoor::~CSWSDoor() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

CResRef* CSWSDoor::GetDialogResref(CResRef* outResRef) {
    if (!objectPtr || !getDialogResref) {
        return nullptr;
    }

    void* resultPtr = getDialogResref(objectPtr, outResRef ? outResRef->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

CExoLocString* CSWSDoor::GetFirstName() {
    if (!objectPtr || !getFirstName) {
        return nullptr;
    }

    void* resultPtr = getFirstName(objectPtr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoLocString(resultPtr);
}

int CSWSDoor::GetIsLinked() {
    if (!objectPtr || !getIsLinked) {
        return 0;
    }
    return getIsLinked(objectPtr);
}

CSWSObject* CSWSDoor::GetLinkedObject() {
    if (!objectPtr || !getLinkedObject) {
        return nullptr;
    }

    void* linkedPtr = getLinkedObject(objectPtr);
    if (!linkedPtr) {
        return nullptr;
    }

    return new CSWSObject(linkedPtr);
}

int CSWSDoor::InDoor(Vector point) {
    if (!objectPtr || !inDoor) {
        return 0;
    }
    return inDoor(objectPtr, point);
}

void CSWSDoor::MoveToNextOpenState() {
    if (!objectPtr || !moveToNextOpenState) {
        return;
    }
    moveToNextOpenState(objectPtr);
}

void CSWSDoor::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}
