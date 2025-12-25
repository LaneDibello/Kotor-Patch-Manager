#include "CRes.h"
#include "GameVersion.h"

// Initialize static members
bool CRes::functionsInitialized = false;
bool CRes::offsetsInitialized = false;

CRes::ConstructorFn CRes::constructor = nullptr;
CRes::DestructorFn CRes::destructor = nullptr;
CRes::GetResRefFn CRes::getResRef = nullptr;
CRes::RequestFn CRes::request = nullptr;
CRes::CancelRequestFn CRes::cancelRequest = nullptr;
CRes::DemandFn CRes::demand = nullptr;
CRes::ReleaseFn CRes::release = nullptr;

int CRes::offsetVTable = -1;
int CRes::offsetData = -1;
int CRes::offsetSize = -1;

void CRes::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CRes] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        constructor = reinterpret_cast<ConstructorFn>(
            GameVersion::GetFunctionAddress("CRes", "Constructor")
        );
        destructor = reinterpret_cast<DestructorFn>(
            GameVersion::GetFunctionAddress("CRes", "Destructor")
        );
        getResRef = reinterpret_cast<GetResRefFn>(
            GameVersion::GetFunctionAddress("CRes", "GetResRef")
        );
        request = reinterpret_cast<RequestFn>(
            GameVersion::GetFunctionAddress("CRes", "Request")
        );
        cancelRequest = reinterpret_cast<CancelRequestFn>(
            GameVersion::GetFunctionAddress("CRes", "CancelRequest")
        );
        demand = reinterpret_cast<DemandFn>(
            GameVersion::GetFunctionAddress("CRes", "Demand")
        );
        release = reinterpret_cast<ReleaseFn>(
            GameVersion::GetFunctionAddress("CRes", "Release")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CRes] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CRes::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CRes] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetVTable = GameVersion::GetOffset("CRes", "vtable");
        offsetData = GameVersion::GetOffset("CRes", "data");
        offsetSize = GameVersion::GetOffset("CRes", "size");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CRes] ERROR: %s\n", e.what());
    }
}

CRes::CRes(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CRes::CRes()
    : GameAPIObject(nullptr, true)  // true = will free (allocating new)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(OBJECT_SIZE);
    constructor(objectPtr);
}

CRes::~CRes() {
    if (shouldFree && objectPtr) {
        destructor(objectPtr);
        free(objectPtr);
    }
    // Base class destructor handles setting objectPtr to nullptr
}

void* CRes::GetVTable() {
    if (!objectPtr || offsetVTable < 0) {
        return nullptr;
    }
    return getObjectProperty<void*>(objectPtr, offsetVTable);
}

void* CRes::GetData() {
    if (!objectPtr || offsetData < 0) {
        return nullptr;
    }
    return getObjectProperty<void*>(objectPtr, offsetData);
}

DWORD CRes::GetSize() {
    if (!objectPtr || offsetSize < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetSize);
}

void CRes::GetResRef(CResRef* outRef, WORD* outType) {
    if (objectPtr && getResRef) {
        getResRef(objectPtr, outRef->GetPtr(), outType);
    }
}

int CRes::Request() {
    if (objectPtr && request) {
        return request(objectPtr);
    }
    return 0;
}

void CRes::CancelRequest() {
    if (objectPtr && cancelRequest) {
        cancelRequest(objectPtr);
    }
}

void CRes::Demand() {
    if (objectPtr && demand) {
        demand(objectPtr);
    }
}

void CRes::Release() {
    if (objectPtr && release) {
        release(objectPtr);
    }
}
