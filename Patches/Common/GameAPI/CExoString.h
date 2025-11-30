#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class CExoString : public GameAPIObject {
public:
    explicit CExoString(void* stringPtr);

    CExoString();
    CExoString(char* src, int length);
    CExoString(char* src);
    ~CExoString();

    DWORD GetLength();
    char* GetCStr();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef CExoString* (__thiscall* DefaultConstructor)(CExoString* thisPtr);
    typedef CExoString* (__thiscall* CStrLenConstructor)(CExoString* thisPtr, char* source, int length);
    typedef CExoString* (__thiscall* CStrConstructor)(CExoString* thisPtr, char* source);
    typedef CExoString* (__thiscall* Destructor)(CExoString* thisPtr);

    static DefaultConstructor defaultConstructor;
    static CStrLenConstructor cStrLenConstructor;
    static CStrConstructor cStrConstructor;
    static Destructor destructor;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetCStr;
    static int offsetLength;
};