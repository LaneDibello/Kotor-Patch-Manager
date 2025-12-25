#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"
#include "GameAPIObject.h"

class CRes : public GameAPIObject {
private:
    static constexpr size_t OBJECT_SIZE = 0x28;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    typedef void(__thiscall* GetResRefFn)(void* thisPtr, void* outRef, WORD* outType);
    typedef int(__thiscall* RequestFn)(void* thisPtr);
    typedef void(__thiscall* CancelRequestFn)(void* thisPtr);
    typedef void(__thiscall* DemandFn)(void* thisPtr);
    typedef void(__thiscall* ReleaseFn)(void* thisPtr);

    typedef void(__thiscall* ConstructorFn)(void* thisPtr);
    typedef void(__thiscall* DestructorFn)(void* thisPtr);

    // Static function pointers
    static ConstructorFn constructor;
    static DestructorFn destructor;
    static GetResRefFn getResRef;
    static RequestFn request;
    static CancelRequestFn cancelRequest;
    static DemandFn demand;
    static ReleaseFn release;

    // Static offsets
    static int offsetVTable;
    static int offsetData;
    static int offsetSize;

public:
    // Wrapping constructor - for existing game objects
    explicit CRes(void* objectPtr);

    // Allocating constructor - creates new CRes object
    CRes();

    virtual ~CRes();

    // Accessors for offsets
    void* GetVTable();
    void* GetData();
    DWORD GetSize();

    // Game function wrappers
    void GetResRef(CResRef* outRef, WORD* outType);
    int Request();
    void CancelRequest();
    void Demand();
    void Release();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;
};