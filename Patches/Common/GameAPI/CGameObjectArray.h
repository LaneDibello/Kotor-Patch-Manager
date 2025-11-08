#pragma once

#include <windows.h>

class CGameObjectArray {
public:
    explicit CGameObjectArray(void* arrayPtr);
    ~CGameObjectArray();

    void* GetGameObject(DWORD objectId);
    void* GetPtr() const { return arrayPtr; }

private:
    void* arrayPtr;

    typedef int (__thiscall* GetGameObjectFn)(void* thisPtr, DWORD objectId, void** outObject);

    static GetGameObjectFn getGameObject;
    static void InitializeFunctions();
    static bool functionsInitialized;
};
