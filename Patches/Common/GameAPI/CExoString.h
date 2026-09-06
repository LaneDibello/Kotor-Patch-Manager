#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

#include <string>

class CExoString : public GameAPIObject {
public:
    explicit CExoString(void* stringPtr);

    CExoString();
    CExoString(char* src, int length);
    CExoString(char* src);
    ~CExoString();

    DWORD GetLength();
    char* GetCStr();

    // Copies the text out using the length field; game strings are not reliably
    // NUL terminated. Empty if the string is unset or its length is implausible.
    std::string ToStdString();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    // Above this a length is treated as garbage rather than trusted.
    static const DWORD MAX_SANE_LENGTH = 65536;

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

    // Size of the CExoString struct in game memory, looked up from the
    // classes table at init time (replaces the previously hard-coded 8).
    static int classSize;
};