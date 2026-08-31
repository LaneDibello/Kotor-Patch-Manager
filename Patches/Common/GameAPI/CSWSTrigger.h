#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

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


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocalizedName();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetLinkedTo();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetLinkedToModule();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptHeartbeat();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnEnter();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnExit();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptUserDefined();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnTrapTriggered();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDisarm();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnClick();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetKeyName();
    int GetGeometryCount();
    void SetGeometryCount(int value);
    Vector* GetGeometry();
    void SetGeometry(Vector* value);
    DWORD* GetGeometryIndices();
    void SetGeometryIndices(DWORD* value);
    DWORD GetFaction();
    void SetFaction(DWORD value);
    DWORD GetTrapDetectable();
    void SetTrapDetectable(DWORD value);
    DWORD GetTrapDisarmable();
    void SetTrapDisarmable(DWORD value);
    BYTE GetTrapType();
    void SetTrapType(BYTE value);
    float GetBBoxMinX();
    void SetBBoxMinX(float value);
    float GetBBoxMinY();
    void SetBBoxMinY(float value);
    float GetBBoxMaxX();
    void SetBBoxMaxX(float value);
    float GetBBoxMaxY();
    void SetBBoxMaxY(float value);
    BYTE GetCursor();
    void SetCursor(BYTE value);
    WORD GetLoadScreenId();
    void SetLoadScreenId(WORD value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetTransitionDestination();

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

    static int offsetLocalizedName;
    static int offsetLinkedTo;
    static int offsetLinkedToModule;
    static int offsetScriptHeartbeat;
    static int offsetScriptOnEnter;
    static int offsetScriptOnExit;
    static int offsetScriptUserDefined;
    static int offsetScriptOnTrapTriggered;
    static int offsetScriptOnDisarm;
    static int offsetScriptOnClick;
    static int offsetKeyName;
    static int offsetGeometryCount;
    static int offsetGeometry;
    static int offsetGeometryIndices;
    static int offsetFaction;
    static int offsetTrapDetectable;
    static int offsetTrapDisarmable;
    static int offsetTrapType;
    static int offsetBBoxMinX;
    static int offsetBBoxMinY;
    static int offsetBBoxMaxX;
    static int offsetBBoxMaxY;
    static int offsetCursor;
    static int offsetLoadScreenId;
    static int offsetTransitionDestination;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
