#pragma once
#include "../Common.h"

class CExoString {
public:
    explicit CExoString(void* stringPtr);

    CExoString();
    CExoString(char* src, int length);
    CExoString(char* src);
    ~CExoString();

    DWORD GetLength();
    char* GetCStr();

    void* GetPtr() const { return stringPtr; }

private:
    void* stringPtr;
    bool shouldFree;

    typedef CExoString* (__thiscall* DefaultConstructor)(CExoString* thisPtr);
    typedef CExoString* (__thiscall* CStrLenConstructor)(CExoString* thisPtr, char* source, int length);
    typedef CExoString* (__thiscall* CStrConstructor)(CExoString* thisPtr, char* source);
    typedef CExoString* (__thiscall* Destructor)(CExoString* thisPtr);

    static DefaultConstructor defaultConstructor;
    static CStrLenConstructor cStrLenConstructor;
    static CStrConstructor cStrConstructor;
    static Destructor destructor;

    static void InitializeOffsets();
    static void InitializeFunctions();
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetCStr;
    static int offsetLength;
};