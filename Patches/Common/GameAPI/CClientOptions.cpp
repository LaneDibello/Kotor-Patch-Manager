#include "CClientOptions.h"
#include "CClientExoApp.h"
#include "CExoString.h"
#include "CResRef.h"

CClientOptions::SetCameraModeFn CClientOptions::setCameraMode = nullptr;
CClientOptions::GetCameraKeyboardAccelerationFn CClientOptions::getCameraKeyboardAcceleration = nullptr;
CClientOptions::GetCameraKeyboardDecelerationFn CClientOptions::getCameraKeyboardDeceleration = nullptr;
CClientOptions::GetCameraKeyboardDPSFn CClientOptions::getCameraKeyboardDPS = nullptr;

CClientOptions::GetGammaSettingFn CClientOptions::getGammaSetting = nullptr;
CClientOptions::SetAnisotropyFn CClientOptions::setAnisotropy = nullptr;
CClientOptions::SetAntiAliasFn CClientOptions::setAntiAlias = nullptr;
CClientOptions::SetFrameBufferFn CClientOptions::setFrameBuffer = nullptr;
CClientOptions::SetFullScreenEnabledFn CClientOptions::setFullScreenEnabled = nullptr;
CClientOptions::SetGrassFn CClientOptions::setGrass = nullptr;
CClientOptions::SetShadowsFn CClientOptions::setShadows = nullptr;
CClientOptions::SetSoftShadowsFn CClientOptions::setSoftShadows = nullptr;
CClientOptions::SetTexQualFn CClientOptions::setTexQual = nullptr;
CClientOptions::SetUseSmallFontsFn CClientOptions::setUseSmallFonts = nullptr;
CClientOptions::SetVSyncFn CClientOptions::setVSync = nullptr;

CClientOptions::GetMouseSenSettingFn CClientOptions::getMouseSenSetting = nullptr;
CClientOptions::SetEnableHardwareMouseCursorFn CClientOptions::setEnableHardwareMouseCursor = nullptr;

CClientOptions::GetMovieShownFn CClientOptions::getMovieShown = nullptr;
CClientOptions::SetMovieShownFn CClientOptions::setMovieShown = nullptr;

CClientOptions::SetAutoLevelUpNPCsFn CClientOptions::setAutoLevelUpNPCs = nullptr;

CClientOptions::LoadOptionsFn CClientOptions::loadOptions = nullptr;
CClientOptions::SaveOptionsFn CClientOptions::saveOptions = nullptr;

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

    GameVersion::ResolveFunction(setCameraMode, "CClientOptions", "SetCameraMode");
    GameVersion::ResolveFunction(getCameraKeyboardAcceleration, "CClientOptions", "GetCameraKeyboardAcceleration");
    GameVersion::ResolveFunction(getCameraKeyboardDeceleration, "CClientOptions", "GetCameraKeyboardDeceleration");
    GameVersion::ResolveFunction(getCameraKeyboardDPS, "CClientOptions", "GetCameraKeyboardDPS");

    GameVersion::ResolveFunction(getGammaSetting, "CClientOptions", "GetGammaSetting");
    GameVersion::ResolveFunction(setAnisotropy, "CClientOptions", "SetAnisotropy");
    GameVersion::ResolveFunction(setAntiAlias, "CClientOptions", "SetAntiAlias");
    GameVersion::ResolveFunction(setFrameBuffer, "CClientOptions", "SetFrameBuffer");
    GameVersion::ResolveFunction(setFullScreenEnabled, "CClientOptions", "SetFullScreenEnabled");
    GameVersion::ResolveFunction(setGrass, "CClientOptions", "SetGrass");
    GameVersion::ResolveFunction(setShadows, "CClientOptions", "SetShadows");
    GameVersion::ResolveFunction(setSoftShadows, "CClientOptions", "SetSoftShadows");
    GameVersion::ResolveFunction(setTexQual, "CClientOptions", "SetTexQual");
    GameVersion::ResolveFunction(setUseSmallFonts, "CClientOptions", "SetUseSmallFonts");
    GameVersion::ResolveFunction(setVSync, "CClientOptions", "SetVSync");

    GameVersion::ResolveFunction(getMouseSenSetting, "CClientOptions", "GetMouseSenSetting");
    GameVersion::ResolveFunction(setEnableHardwareMouseCursor, "CClientOptions", "SetEnableHardwareMouseCursor");

    GameVersion::ResolveFunction(getMovieShown, "CClientOptions", "GetMovieShown");
    GameVersion::ResolveFunction(setMovieShown, "CClientOptions", "SetMovieShown");

    GameVersion::ResolveFunction(setAutoLevelUpNPCs, "CClientOptions", "SetAutoLevelUpNPCs");

    GameVersion::ResolveFunction(loadOptions, "CClientOptions", "LoadOptions");
    GameVersion::ResolveFunction(saveOptions, "CClientOptions", "SaveOptions");

    functionsInitialized = true;
}

void CClientOptions::InitializeOffsets() {
    // CClientOptions has no offsets
    offsetsInitialized = true;
}

CClientOptions* CClientOptions::GetInstance() {
    CClientExoApp* client = CClientExoApp::GetInstance();
    if (!client) {
        OutputDebugStringA("[CClientOptions] ERROR: Failed to get CClientExoApp instance\n");
        return nullptr;
    }

    CClientOptions* options = client->GetClientOptions();
    delete client;  // Clean up the temporary CClientExoApp instance

    return options;
}

CClientOptions::CClientOptions()
    : GameAPIObject(nullptr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // The options object has no global pointer of its own; it hangs off the client
    // (APP_MANAGER_PTR -> CAppManager::Client -> CClientExoApp::GetClientOptions()).
    CClientOptions* options = GetInstance();
    if (options) {
        objectPtr = options->GetPtr();
        delete options;  // Clean up the temporary wrapper; we don't own the singleton
    } else {
        OutputDebugStringA("[CClientOptions] ERROR: Failed to resolve client options\n");
    }
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

// x87 FPU call: return comes back in ST0 (typed as double).
double CClientOptions::GetCameraKeyboardAcceleration() {
    if (!objectPtr || !getCameraKeyboardAcceleration) return 0;
    return getCameraKeyboardAcceleration(objectPtr);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double CClientOptions::GetCameraKeyboardDeceleration() {
    if (!objectPtr || !getCameraKeyboardDeceleration) return 0;
    return getCameraKeyboardDeceleration(objectPtr);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double CClientOptions::GetCameraKeyboardDPS() {
    if (!objectPtr || !getCameraKeyboardDPS) return 0;
    return getCameraKeyboardDPS(objectPtr);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double CClientOptions::GetGammaSetting() {
    if (!objectPtr || !getGammaSetting) return 0;
    return getGammaSetting(objectPtr);
}

void CClientOptions::SetAnisotropy(DWORD anisotropy) {
    if (!objectPtr || !setAnisotropy) {
        return;
    }
    setAnisotropy(objectPtr, anisotropy);
}

void CClientOptions::SetAntiAlias(BYTE antiAlias) {
    if (!objectPtr || !setAntiAlias) {
        return;
    }
    setAntiAlias(objectPtr, antiAlias);
}

void CClientOptions::SetFrameBuffer(DWORD fbEnabled) {
    if (!objectPtr || !setFrameBuffer) {
        return;
    }
    setFrameBuffer(objectPtr, fbEnabled);
}

void CClientOptions::SetFullScreenEnabled(int enabled) {
    if (!objectPtr || !setFullScreenEnabled) {
        return;
    }
    setFullScreenEnabled(objectPtr, enabled);
}

void CClientOptions::SetGrass(DWORD enabled) {
    if (!objectPtr || !setGrass) {
        return;
    }
    setGrass(objectPtr, enabled);
}

void CClientOptions::SetShadows(int enabled) {
    if (!objectPtr || !setShadows) {
        return;
    }
    setShadows(objectPtr, enabled);
}

void CClientOptions::SetSoftShadows(DWORD enabled) {
    if (!objectPtr || !setSoftShadows) {
        return;
    }
    setSoftShadows(objectPtr, enabled);
}

void CClientOptions::SetTexQual(BYTE quality) {
    if (!objectPtr || !setTexQual) {
        return;
    }
    setTexQual(objectPtr, quality);
}

void CClientOptions::SetUseSmallFonts(int enabled) {
    if (!objectPtr || !setUseSmallFonts) {
        return;
    }
    setUseSmallFonts(objectPtr, enabled);
}

void CClientOptions::SetVSync(DWORD enabled) {
    if (!objectPtr || !setVSync) {
        return;
    }
    setVSync(objectPtr, enabled);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double CClientOptions::GetMouseSenSetting() {
    if (!objectPtr || !getMouseSenSetting) return 0;
    return getMouseSenSetting(objectPtr);
}

void CClientOptions::SetEnableHardwareMouseCursor(int enabled) {
    if (!objectPtr || !setEnableHardwareMouseCursor) {
        return;
    }
    setEnableHardwareMouseCursor(objectPtr, enabled);
}

int CClientOptions::GetMovieShown(CResRef* movie) {
    if (!objectPtr || !getMovieShown) {
        return 0;
    }
    return getMovieShown(objectPtr, movie ? movie->GetPtr() : nullptr);
}

void CClientOptions::SetMovieShown(CExoString* movie, int shown) {
    if (!objectPtr || !setMovieShown) {
        return;
    }
    setMovieShown(objectPtr, movie ? movie->GetPtr() : nullptr, shown);
}

void CClientOptions::SetAutoLevelUpNPCs(int autoLevel) {
    if (!objectPtr || !setAutoLevelUpNPCs) {
        return;
    }
    setAutoLevelUpNPCs(objectPtr, autoLevel);
}

int CClientOptions::LoadOptions() {
    if (!objectPtr || !loadOptions) {
        return 0;
    }
    return loadOptions(objectPtr);
}

int CClientOptions::SaveOptions() {
    if (!objectPtr || !saveOptions) {
        return 0;
    }
    return saveOptions(objectPtr);
}
