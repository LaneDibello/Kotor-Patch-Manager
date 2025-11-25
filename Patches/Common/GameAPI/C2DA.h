#pragma once

#include <windows.h>
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPIObject.h"

class C2DA : public GameAPIObject {
public:
    explicit C2DA(void* ptr);
    C2DA(const char* name);
    ~C2DA();

    bool GetCExoStringEntry(int row, CExoString* column, CExoString* output);
    bool GetFLOATEntry(int row, CExoString* column, float* output);
    bool GetINTEntry(int row, CExoString* column, int* output);
    void Load2DArray();
    void Unload2DArray();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    const int objectSize = 0x54;

    typedef bool(__thiscall* GetCExoStringEntryFn)(void* thisPtr, int row, void* column, void* output);
    typedef bool(__thiscall* GetFLOATEntryFn)(void* thisPtr, int row, void* column, float* output);
    typedef bool(__thiscall* GetINTEntryFn)(void* thisPtr, int row, void* column, int* output);
    typedef void(__thiscall* Load2DArrayFn)(void* thisPtr);
    typedef void(__thiscall* Unload2DArrayFn)(void* thisPtr);
    typedef void(__thiscall* ConstructorFn)(void* thisPtr, CResRef_struct name, int usually0);

    static GetCExoStringEntryFn getCExoStringEntry;
    static GetFLOATEntryFn getFLOATEntry;
    static GetINTEntryFn getINTEntry;
    static Load2DArrayFn load2DArray;
    static Unload2DArrayFn unload2DArray;
    static ConstructorFn constructor;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
