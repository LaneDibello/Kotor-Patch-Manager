#include "CExoLocString.h"
#include "GameVersion.h"

bool CExoLocString::functionsInitialized = false;
bool CExoLocString::offsetsInitialized = false;

int CExoLocString::offsetInternal = -1;

void CExoLocString::InitializeFunctions() {
    // CExoLocString has no functions wrapped yet
    functionsInitialized = true;
}

void CExoLocString::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoLocString] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetInternal = GameVersion::GetOffset("CExoLocString", "internal");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoLocString] ERROR: %s\n", e.what());
    }
}

CExoLocString::CExoLocString(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CExoLocString::~CExoLocString() {
    // Base class destructor handles objectPtr cleanup
}

void* CExoLocString::GetInternal() {
    if (!objectPtr || offsetInternal < 0) {
        return nullptr;
    }
    return getObjectProperty<void*>(objectPtr, offsetInternal);
}
