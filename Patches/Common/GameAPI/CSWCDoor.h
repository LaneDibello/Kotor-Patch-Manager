#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CSWSDoor;
class CExoString;

/// <summary>
/// Wraps a client-side door.
/// </summary>
class CSWCDoor : public CSWCObject {
public:
    explicit CSWCDoor(void* objectPtr);
    virtual ~CSWCDoor();

    int GetIsOpen();

    // Fill outName and return it wrapped; caller owns the returned wrapper.
    CExoString* GetModelName(CExoString* outName);
    // Hides CSWCObject::GetName; this is the door-specific implementation.
    CExoString* GetName(CExoString* outName);

    // Returns the paired server door as a heap-allocated wrapper; caller owns it.
    CSWSDoor* GetServerDoor();

    void SetIsAreaTransition(int isTransition);
    // NOTE: the address DB records param_size 0 for this, which cannot be right
    // for a byte parameter. Typed from the Ghidra signature instead.
    void SetState(BYTE state);
    void UpdateAreaTransitionDisplay();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef int(__thiscall* GetIsOpenFn)(void* thisPtr);
    typedef void* (__thiscall* GetModelNameFn)(void* thisPtr, void* outName);
    typedef void* (__thiscall* GetNameFn)(void* thisPtr, void* outName);
    typedef void* (__thiscall* GetServerDoorFn)(void* thisPtr);
    typedef void(__thiscall* SetIsAreaTransitionFn)(void* thisPtr, int isTransition);
    typedef void(__thiscall* SetStateFn)(void* thisPtr, BYTE state);
    typedef void(__thiscall* UpdateAreaTransitionDisplayFn)(void* thisPtr);

    static GetIsOpenFn getIsOpen;
    static GetModelNameFn getModelName;
    static GetNameFn getName;
    static GetServerDoorFn getServerDoor;
    static SetIsAreaTransitionFn setIsAreaTransition;
    static SetStateFn setState;
    static UpdateAreaTransitionDisplayFn updateAreaTransitionDisplay;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
