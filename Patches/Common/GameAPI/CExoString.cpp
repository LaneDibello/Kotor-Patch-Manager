#include "CExoString.h"
#include "GameVersion.h"
#include "../Common.h"

CExoString::DefaultConstructor CExoString::defaultConstructor = nullptr;
CExoString::CStrLenConstructor CExoString::cStrLenConstructor = nullptr;
CExoString::CStrConstructor CExoString::cStrConstructor = nullptr;
CExoString::Destructor CExoString::destructor = nullptr;

bool CExoString::functionsInitialized = false;
bool CExoString::offsetsInitialized = false;

int CExoString::offsetCStr = -1;
int CExoString::offsetLength = -1;

void CExoString::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoString] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        defaultConstructor = reinterpret_cast<DefaultConstructor>(
            GameVersion::GetFunctionAddress("CExoString", "DefaultConstructor")
            );
        cStrLenConstructor = reinterpret_cast<CStrLenConstructor>(
            GameVersion::GetFunctionAddress("CExoString", "CStrLenConstructor")
            );
        cStrConstructor = reinterpret_cast<CStrConstructor>(
            GameVersion::GetFunctionAddress("CExoString", "CStrConstructor")
            );
        destructor = reinterpret_cast<Destructor>(
            GameVersion::GetFunctionAddress("CExoString", "Destructor")
            );
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoString] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CExoString::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoString] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetCStr = GameVersion::GetOffset("CExoString", "CStr");
        offsetLength = GameVersion::GetOffset("CExoString", "Length");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoString] ERROR: %s\n", e.what());
    }
}

CExoString::CExoString(void* stringPtr)
    : stringPtr(stringPtr) {
    
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    shouldFree = false;
}

CExoString::CExoString() {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    shouldFree = true;

    stringPtr = malloc(8);

    defaultConstructor(static_cast<CExoString*>(stringPtr));
}

CExoString::CExoString(char* src, int length) {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    shouldFree = true;

    stringPtr = malloc(8);

    cStrLenConstructor(static_cast<CExoString*>(stringPtr), src, length);
}

CExoString::CExoString(char* src) {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    shouldFree = true;

    stringPtr = malloc(8);

    cStrConstructor(static_cast<CExoString*>(stringPtr), src);

}

CExoString::~CExoString() {
    if (shouldFree) {
        destructor(static_cast<CExoString*>(stringPtr));
        free(stringPtr);
    }
    stringPtr = nullptr;
}

DWORD CExoString::GetLength() {
    if (!stringPtr || offsetLength < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(stringPtr, offsetLength);
}

char* CExoString::GetCStr() {
    if (!stringPtr || offsetCStr < 0) {
        return nullptr;
    }
    return getObjectProperty<char*>(stringPtr, offsetCStr);
}