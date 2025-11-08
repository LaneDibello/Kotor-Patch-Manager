#pragma once

#include <windows.h>

class CAppManager;
class CSWSCreature;

class CServerExoApp {
public:
    static CServerExoApp* GetInstance();
    ~CServerExoApp();

    void* GetObjectArray();
    DWORD GetPlayerCreatureId();
    CSWSCreature* GetCreatureByGameObjectID(DWORD objectId);

    void* GetPtr() const { return serverPtr; }

private:
    friend class CAppManager;
    explicit CServerExoApp(void* serverPtr);

    void* serverPtr;

    typedef void* (__thiscall* GetObjectArrayFn)(void* thisPtr);
    typedef DWORD(__thiscall* GetPlayerCreatureIdFn)(void* thisPtr);
    typedef void* (__thiscall* GetCreatureByGameObjectIDFn)(void* thisPtr, DWORD objectId);

    static GetObjectArrayFn getObjectArray;
    static GetPlayerCreatureIdFn getPlayerCreatureId;
    static GetCreatureByGameObjectIDFn getCreatureByGameObjectID;

    static void InitializeFunctions();
    static bool functionsInitialized;
};
