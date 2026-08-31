#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CResRef;
class CExoLocString;

/// <summary>
/// Wraps a server-side door.
/// </summary>
class CSWSDoor : public CSWSObject {
public:
    explicit CSWSDoor(void* objectPtr);
    virtual ~CSWSDoor();

    // Fills outResRef and returns it wrapped; caller owns the returned wrapper.
    CResRef* GetDialogResref(CResRef* outResRef);

    // Returns a heap-allocated wrapper; caller owns it.
    CExoLocString* GetFirstName();

    int GetIsLinked();
    // Returns the linked object (the door's transition target) as a
    // heap-allocated wrapper; caller owns it.
    CSWSObject* GetLinkedObject();

    // Point is passed by value (12 bytes).
    int InDoor(Vector point);

    void MoveToNextOpenState();
    void RemoveFromArea();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void* (__thiscall* GetDialogResrefFn)(void* thisPtr, void* outResRef);
    typedef void* (__thiscall* GetFirstNameFn)(void* thisPtr);
    typedef int(__thiscall* GetIsLinkedFn)(void* thisPtr);
    typedef void* (__thiscall* GetLinkedObjectFn)(void* thisPtr);
    typedef int(__thiscall* InDoorFn)(void* thisPtr, Vector point);
    typedef void(__thiscall* MoveToNextOpenStateFn)(void* thisPtr);
    typedef void(__thiscall* RemoveFromAreaFn)(void* thisPtr);

    static GetDialogResrefFn getDialogResref;
    static GetFirstNameFn getFirstName;
    static GetIsLinkedFn getIsLinked;
    static GetLinkedObjectFn getLinkedObject;
    static InDoorFn inDoor;
    static MoveToNextOpenStateFn moveToNextOpenState;
    static RemoveFromAreaFn removeFromArea;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
