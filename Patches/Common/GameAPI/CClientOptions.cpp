#include "CClientOptions.h"

CClientOptions::SetCameraModeFn CClientOptions::setCameraMode = nullptr;
bool CClientOptions::functionsInitialized = false;
bool CClientOptions::offsetsInitialized = false;

void CClientOptions::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CClientOptions] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        setCameraMode = reinterpret_cast<SetCameraModeFn>(
            GameVersion::GetFunctionAddress("CClientOptions", "SetCameraMode")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CClientOptions] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CClientOptions::InitializeOffsets() {
    // CClientOptions has no offsets
    offsetsInitialized = true;
}

CClientOptions::CClientOptions(void* optionsPtr)
    : GameAPIObject(optionsPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CClientOptions::~CClientOptions() {
    // Base class destructor handles objectPtr cleanup
}

void CClientOptions::SetCameraMode(BYTE mode) {
    if (!objectPtr || !setCameraMode) {
        return;
    }

    setCameraMode(objectPtr, mode);
}
