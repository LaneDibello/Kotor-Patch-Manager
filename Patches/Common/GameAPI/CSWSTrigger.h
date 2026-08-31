#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoLocString;

/// <summary>
/// Wraps a server-side trigger volume (area transitions, traps, generic triggers).
/// </summary>
class CSWSTrigger : public CSWSObject {
public:
    explicit CSWSTrigger(void* objectPtr);
    virtual ~CSWSTrigger();

    // Mines
    int GetCanFireMineOnObject(DWORD objectId, int skipFactionCheck);
    void OnEnterMine(int skipFactionCheck);

    // Returns a heap-allocated wrapper; caller owns it.
    CExoLocString* GetFirstName();

    DWORD GetTargetArea();

    // Point is passed by pointer here, unlike CSWSDoor::InDoor which takes a
    // Vector by value.
    int InTrigger(Vector* point);

    void SetCreator(DWORD creatorId);
    void RemoveFromArea();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef int(__thiscall* GetCanFireMineOnObjectFn)(void* thisPtr, DWORD objectId, int skipFactionCheck);
    typedef void(__thiscall* OnEnterMineFn)(void* thisPtr, int skipFactionCheck);
    typedef void* (__thiscall* GetFirstNameFn)(void* thisPtr);
    typedef DWORD(__thiscall* GetTargetAreaFn)(void* thisPtr);
    typedef int(__thiscall* InTriggerFn)(void* thisPtr, Vector* point);
    typedef void(__thiscall* SetCreatorFn)(void* thisPtr, DWORD creatorId);
    typedef void(__thiscall* RemoveFromAreaFn)(void* thisPtr);

    static GetCanFireMineOnObjectFn getCanFireMineOnObject;
    static OnEnterMineFn onEnterMine;
    static GetFirstNameFn getFirstName;
    static GetTargetAreaFn getTargetArea;
    static InTriggerFn inTrigger;
    static SetCreatorFn setCreator;
    static RemoveFromAreaFn removeFromArea;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
