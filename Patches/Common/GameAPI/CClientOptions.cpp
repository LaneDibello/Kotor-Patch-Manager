#include "CClientOptions.h"

CClientOptions::SetCameraModeFn CClientOptions::setCameraMode = nullptr;
bool CClientOptions::functionsInitialized = false;

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

CClientOptions::CClientOptions(void* optionsPtr)
    : optionsPtr(optionsPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

CClientOptions::~CClientOptions() {
    optionsPtr = nullptr;
}

void CClientOptions::SetCameraMode(BYTE mode) {
    if (!optionsPtr || !setCameraMode) {
        return;
    }

    setCameraMode(optionsPtr, mode);
}
