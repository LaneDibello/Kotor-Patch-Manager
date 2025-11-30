#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CGameObjectArray : public GameAPIObject {
public:
    explicit CGameObjectArray(void* arrayPtr);
    ~CGameObjectArray();

    void* GetGameObject(DWORD objectId);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef int (__thiscall* GetGameObjectFn)(void* thisPtr, DWORD objectId, void** outObject);

    static GetGameObjectFn getGameObject;
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
