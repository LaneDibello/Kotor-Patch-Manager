#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "../Common.h"

class CExoString;
class CResRef;

class CClientOptions : public GameAPIObject {
public:
    static CClientOptions* GetInstance();
    // Automatically retrieves and wraps the global options object
    // (APP_MANAGER_PTR -> CAppManager::Client -> CClientExoApp::GetClientOptions()).
    CClientOptions();
    explicit CClientOptions(void* optionsPtr);
    ~CClientOptions();

    // Camera / keyboard
    void SetCameraMode(BYTE mode);
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetCameraKeyboardAcceleration();
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetCameraKeyboardDeceleration();
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetCameraKeyboardDPS();

    // Graphics
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetGammaSetting();
    void SetAnisotropy(DWORD anisotropy);
    void SetAntiAlias(BYTE antiAlias);
    void SetFrameBuffer(DWORD fbEnabled);
    void SetFullScreenEnabled(int enabled);
    void SetGrass(DWORD enabled);
    void SetShadows(int enabled);
    void SetSoftShadows(DWORD enabled);
    void SetTexQual(BYTE quality);
    void SetUseSmallFonts(int enabled);
    void SetVSync(DWORD enabled);

    // Mouse
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetMouseSenSetting();
    void SetEnableHardwareMouseCursor(int enabled);

    // Movies
    int GetMovieShown(CResRef* movie);
    void SetMovieShown(CExoString* movie, int shown);

    // Gameplay
    void SetAutoLevelUpNPCs(int autoLevel);

    // Persistence
    int LoadOptions();
    int SaveOptions();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void (__thiscall* SetCameraModeFn)(void* thisPtr, BYTE mode);
    typedef double (__thiscall* GetCameraKeyboardAccelerationFn)(void* thisPtr);
    typedef double (__thiscall* GetCameraKeyboardDecelerationFn)(void* thisPtr);
    typedef double (__thiscall* GetCameraKeyboardDPSFn)(void* thisPtr);

    typedef double (__thiscall* GetGammaSettingFn)(void* thisPtr);
    typedef void (__thiscall* SetAnisotropyFn)(void* thisPtr, DWORD anisotropy);
    typedef void (__thiscall* SetAntiAliasFn)(void* thisPtr, BYTE antiAlias);
    typedef void (__thiscall* SetFrameBufferFn)(void* thisPtr, DWORD fbEnabled);
    typedef void (__thiscall* SetFullScreenEnabledFn)(void* thisPtr, int enabled);
    typedef void (__thiscall* SetGrassFn)(void* thisPtr, DWORD enabled);
    typedef void (__thiscall* SetShadowsFn)(void* thisPtr, int enabled);
    typedef void (__thiscall* SetSoftShadowsFn)(void* thisPtr, DWORD enabled);
    typedef void (__thiscall* SetTexQualFn)(void* thisPtr, BYTE quality);
    typedef void (__thiscall* SetUseSmallFontsFn)(void* thisPtr, int enabled);
    typedef void (__thiscall* SetVSyncFn)(void* thisPtr, DWORD enabled);

    typedef double (__thiscall* GetMouseSenSettingFn)(void* thisPtr);
    typedef void (__thiscall* SetEnableHardwareMouseCursorFn)(void* thisPtr, int enabled);

    typedef int (__thiscall* GetMovieShownFn)(void* thisPtr, void* movie);
    typedef void (__thiscall* SetMovieShownFn)(void* thisPtr, void* movie, int shown);

    typedef void (__thiscall* SetAutoLevelUpNPCsFn)(void* thisPtr, int autoLevel);

    typedef int (__thiscall* LoadOptionsFn)(void* thisPtr);
    typedef int (__thiscall* SaveOptionsFn)(void* thisPtr);

    static SetCameraModeFn setCameraMode;
    static GetCameraKeyboardAccelerationFn getCameraKeyboardAcceleration;
    static GetCameraKeyboardDecelerationFn getCameraKeyboardDeceleration;
    static GetCameraKeyboardDPSFn getCameraKeyboardDPS;

    static GetGammaSettingFn getGammaSetting;
    static SetAnisotropyFn setAnisotropy;
    static SetAntiAliasFn setAntiAlias;
    static SetFrameBufferFn setFrameBuffer;
    static SetFullScreenEnabledFn setFullScreenEnabled;
    static SetGrassFn setGrass;
    static SetShadowsFn setShadows;
    static SetSoftShadowsFn setSoftShadows;
    static SetTexQualFn setTexQual;
    static SetUseSmallFontsFn setUseSmallFonts;
    static SetVSyncFn setVSync;

    static GetMouseSenSettingFn getMouseSenSetting;
    static SetEnableHardwareMouseCursorFn setEnableHardwareMouseCursor;

    static GetMovieShownFn getMovieShown;
    static SetMovieShownFn setMovieShown;

    static SetAutoLevelUpNPCsFn setAutoLevelUpNPCs;

    static LoadOptionsFn loadOptions;
    static SaveOptionsFn saveOptions;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
