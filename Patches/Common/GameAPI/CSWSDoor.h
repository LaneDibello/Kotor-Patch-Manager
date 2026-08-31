#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

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


    // ===== Offsets =====
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnOpen();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnClosed();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDamaged();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDeath();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDisarm();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnHeartbeat();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnLock();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnMeleeAttacked();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnSpellCastAt();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnTrapTriggered();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUnlock();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUserDefined();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnClick();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnDialog();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnFailToOpen();
    BYTE GetAppearance();
    void SetAppearance(BYTE value);
    BYTE GetGenericType();
    void SetGenericType(BYTE value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CResRef* GetConversation();
    DWORD GetFaction();
    void SetFaction(DWORD value);
    BYTE GetFortitude();
    void SetFortitude(BYTE value);
    BYTE GetReflex();
    void SetReflex(BYTE value);
    BYTE GetWill();
    void SetWill(BYTE value);
    BYTE GetOpenLockDC();
    void SetOpenLockDC(BYTE value);
    BYTE GetCloseLockDC();
    void SetCloseLockDC(BYTE value);
    BYTE GetSecretDoorDC();
    void SetSecretDoorDC(BYTE value);
    BYTE GetHardness();
    void SetHardness(BYTE value);
    BYTE GetOpenState();
    void SetOpenState(BYTE value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetKeyName();
    int GetKeyRequired();
    void SetKeyRequired(int value);
    BYTE GetDisarmDC();
    void SetDisarmDC(BYTE value);
    BYTE GetDetectDC();
    void SetDetectDC(BYTE value);
    BYTE GetTrapType();
    void SetTrapType(BYTE value);
    // Indexed access to the 4-element Vector array. index 0-3.
    Vector GetCorner(int index);
    void SetCorner(int index, const Vector& value);
    BYTE GetLinkedToFlags();
    void SetLinkedToFlags(BYTE value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetLinkedTo();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetLinkedToModule();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocName();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetDescription();
    BYTE GetLoadScreenIdLower();
    void SetLoadScreenIdLower(BYTE value);
    BYTE GetLoadScreenIdUpper();
    void SetLoadScreenIdUpper(BYTE value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetTransitionDestination();

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

    static int offsetScriptOnOpen;
    static int offsetScriptOnClosed;
    static int offsetScriptOnDamaged;
    static int offsetScriptOnDeath;
    static int offsetScriptOnDisarm;
    static int offsetScriptOnHeartbeat;
    static int offsetScriptOnLock;
    static int offsetScriptOnMeleeAttacked;
    static int offsetScriptOnSpellCastAt;
    static int offsetScriptOnTrapTriggered;
    static int offsetScriptOnUnlock;
    static int offsetScriptOnUserDefined;
    static int offsetScriptOnClick;
    static int offsetScriptOnDialog;
    static int offsetScriptOnFailToOpen;
    static int offsetAppearance;
    static int offsetGenericType;
    static int offsetConversation;
    static int offsetFaction;
    static int offsetFortitude;
    static int offsetReflex;
    static int offsetWill;
    static int offsetOpenLockDC;
    static int offsetCloseLockDC;
    static int offsetSecretDoorDC;
    static int offsetHardness;
    static int offsetOpenState;
    static int offsetKeyName;
    static int offsetKeyRequired;
    static int offsetDisarmDC;
    static int offsetDetectDC;
    static int offsetTrapType;
    static int offsetCorner;
    static int offsetLinkedToFlags;
    static int offsetLinkedTo;
    static int offsetLinkedToModule;
    static int offsetLocName;
    static int offsetDescription;
    static int offsetLoadScreenIdLower;
    static int offsetLoadScreenIdUpper;
    static int offsetTransitionDestination;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
