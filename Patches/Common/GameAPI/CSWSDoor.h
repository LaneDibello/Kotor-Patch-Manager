#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

class CResRef;
class CExoLocString;

class CSWSDoor : public CSWSObject {
public:
    explicit CSWSDoor(void* objectPtr);
    virtual ~CSWSDoor();


    CResRef* GetDialogResref(CResRef* outResRef);
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
    CExoString* GetScriptOnOpen();
    CExoString* GetScriptOnClosed();
    CExoString* GetScriptOnDamaged();
    CExoString* GetScriptOnDeath();
    CExoString* GetScriptOnDisarm();
    CExoString* GetScriptOnHeartbeat();
    CExoString* GetScriptOnLock();
    CExoString* GetScriptOnMeleeAttacked();
    CExoString* GetScriptOnSpellCastAt();
    CExoString* GetScriptOnTrapTriggered();
    CExoString* GetScriptOnUnlock();
    CExoString* GetScriptOnUserDefined();
    CExoString* GetScriptOnClick();
    CExoString* GetScriptOnDialog();
    CExoString* GetScriptOnFailToOpen();
    BYTE GetAppearance();
    void SetAppearance(BYTE value);
    BYTE GetGenericType();
    void SetGenericType(BYTE value);
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
    CExoString* GetLinkedTo();
    CExoString* GetLinkedToModule();
    CExoLocString* GetLocName();
    CExoLocString* GetDescription();
    BYTE GetLoadScreenIdLower();
    void SetLoadScreenIdLower(BYTE value);
    BYTE GetLoadScreenIdUpper();
    void SetLoadScreenIdUpper(BYTE value);
    CExoLocString* GetTransitionDestination();

    int GetLastHeartbeatDay();
    void SetLastHeartbeatDay(int value);
    int GetLastHeartbeatMs();
    void SetLastHeartbeatMs(int value);

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
    static int offsetLastHeartbeatDay;
    static int offsetLastHeartbeatMs;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
