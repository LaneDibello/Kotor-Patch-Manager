#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CAppManager;
class CSWSCreature;
class CWorldTimer;
class CGameObject;

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
    // Returns the world timer (heap-allocated wrapper; caller owns it). K1 only.
    CWorldTimer* GetWorldTimer();
    // Returns the active timer for an object (heap-allocated wrapper; caller owns it). K1 only.
    CWorldTimer* GetActiveTimer(int objectId);
    // Returns a game object by id (heap-allocated wrapper; caller owns it). K1 only.
    CGameObject* GetGameObject(DWORD objectId);

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
    typedef void* (__thiscall* GetWorldTimerFn)(void* thisPtr);
    typedef void* (__thiscall* GetActiveTimerFn)(void* thisPtr, int objectId);
    typedef void* (__thiscall* GetGameObjectFn)(void* thisPtr, DWORD objectId);

    static GetObjectArrayFn getObjectArray;
    static GetPlayerCreatureIdFn getPlayerCreatureId;
    static GetCreatureByGameObjectIDFn getCreatureByGameObjectID;
    static GetPlayerCreatureFn getPlayerCreature;
    static GetGlobalVariableTableFn getGlobalVariableTable;
    static ClientToServerObjectIdFn clientToServerObjectId;
    static GetWorldTimerFn getWorldTimer;
    static GetActiveTimerFn getActiveTimer;
    static GetGameObjectFn getGameObject;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
