#include "CGameObject.h"

// Initialize static members
bool CGameObject::functionsInitialized = false;
bool CGameObject::offsetsInitialized = false;
int CGameObject::offsetId = -1;
int CGameObject::offsetObjectType = -1;

CGameObject::CGameObject(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing game memory)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CGameObject::~CGameObject() {
    // Base class destructor handles setting objectPtr to nullptr
}

void CGameObject::InitializeFunctions() {
    // CGameObject has no function pointers currently
    // This method exists for consistency and future extensibility
    functionsInitialized = true;
}

void CGameObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CGameObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetId = GameVersion::GetOffset("CGameObject", "Id");
        offsetObjectType = GameVersion::GetOffset("CGameObject", "ObjectType");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CGameObject] ERROR: %s\n", e.what());
    }
}

DWORD CGameObject::GetId() {
    if (!objectPtr || offsetId < 0) {
        return OBJECT_DEFAULT;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetId);
}

WORD CGameObject::GetObjectType() {
    if (!objectPtr || offsetObjectType < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetObjectType);
}

// GetPtr() is now inherited from GameAPIObject
