#include "CGameObjectArray.h"
#include "GameVersion.h"
#include "../Common.h"

CGameObjectArray::GetGameObjectFn CGameObjectArray::getGameObject = nullptr;
bool CGameObjectArray::functionsInitialized = false;
bool CGameObjectArray::offsetsInitialized = false;

void CGameObjectArray::InitializeFunctions() {
    OutputDebugStringA("[CGameObjectArray] Doing Function initialization\n");
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CGameObjectArray] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getGameObject = reinterpret_cast<GetGameObjectFn>(
            GameVersion::GetFunctionAddress("CGameObjectArray", "GetGameObject")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CGameObjectArray] ERROR: %s\n", e.what());
        return;
    }

    OutputDebugStringA("[CGameObjectArray] Functions initialized\n");
    functionsInitialized = true;
}

void CGameObjectArray::InitializeOffsets() {
    // CGameObjectArray has no offsets
    offsetsInitialized = true;
}

CGameObjectArray::CGameObjectArray(void* arrayPtr)
    : GameAPIObject(arrayPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CGameObjectArray::~CGameObjectArray() {
    // Base class destructor handles objectPtr cleanup
}

void* CGameObjectArray::GetGameObject(DWORD objectId) {
    OutputDebugStringA("[CGameObjectArray] Getting Game Object\n");

    if (!objectPtr || !getGameObject) {
        char debug[128];
        sprintf_s(debug, sizeof(debug), "[CGameObjectArray] Failed with array: %p and function: %p", objectPtr, getGameObject);
        OutputDebugStringA(debug);

        return nullptr;
    }

    char debug[128];
    sprintf_s(debug, sizeof(debug), "[CGameObjectArray] Running getGameObject: %p", getGameObject);
    OutputDebugStringA(debug);
    void* objPtr;
    getGameObject(objectPtr, objectId, &objPtr);
    return objPtr;
}
