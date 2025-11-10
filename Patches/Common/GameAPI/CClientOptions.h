#pragma once
#include "GameVersion.h"
#include "../Common.h"

class CClientOptions {
public:
    explicit CClientOptions(void* optionsPtr);
    ~CClientOptions();

    void SetCameraMode(BYTE mode);
    void* GetPtr() const { return optionsPtr; }

private:
    void* optionsPtr;

    typedef void (__thiscall* SetCameraModeFn)(void* thisPtr, BYTE mode);

    static SetCameraModeFn setCameraMode;

    static void InitializeFunctions();
    static bool functionsInitialized;
};
