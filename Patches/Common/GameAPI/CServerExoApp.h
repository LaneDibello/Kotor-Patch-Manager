#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CAppManager;
class CSWSCreature;

class CServerExoApp : public GameAPIObject {
public:
    static CServerExoApp* GetInstance();
    ~CServerExoApp();

    void* GetObjectArray();
    DWORD GetPlayerCreatureId();
    CSWSCreature* GetCreatureByGameObjectID(DWORD objectId);
    CSWSCreature* GetPlayerCreature();
    void* GetGlobalVariableTable();
    DWORD ClientToServerObjectId(DWORD clientId);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    friend class CAppManager;
    explicit CServerExoApp(void* serverPtr);

    typedef void* (__thiscall* GetObjectArrayFn)(void* thisPtr);
    typedef DWORD(__thiscall* GetPlayerCreatureIdFn)(void* thisPtr);
    typedef void* (__thiscall* GetCreatureByGameObjectIDFn)(void* thisPtr, DWORD objectId);
    typedef void* (__thiscall* GetPlayerCreatureFn)(void* thisPtr);
    typedef void* (__thiscall* GetGlobalVariableTableFn)(void* thisPtr);
    typedef DWORD(__thiscall* ClientToServerObjectIdFn)(void* thisPtr, DWORD clientId);

    static GetObjectArrayFn getObjectArray;
    static GetPlayerCreatureIdFn getPlayerCreatureId;
    static GetCreatureByGameObjectIDFn getCreatureByGameObjectID;
    static GetPlayerCreatureFn getPlayerCreature;
    static GetGlobalVariableTableFn getGlobalVariableTable;
    static ClientToServerObjectIdFn clientToServerObjectId;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
