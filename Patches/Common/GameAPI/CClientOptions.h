#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "../Common.h"

class CClientOptions : public GameAPIObject {
public:
    explicit CClientOptions(void* optionsPtr);
    ~CClientOptions();

    void SetCameraMode(BYTE mode);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void (__thiscall* SetCameraModeFn)(void* thisPtr, BYTE mode);

    static SetCameraModeFn setCameraMode;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
