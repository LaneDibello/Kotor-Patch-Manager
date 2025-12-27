#include "CExoString.h"
#include "GameVersion.h"

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
    : GameAPIObject(stringPtr, false) {  // false = don't free (wrapping existing)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CExoString::CExoString()
    : GameAPIObject(nullptr, true) {  // true = will free (allocating new)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(8);

    defaultConstructor(static_cast<CExoString*>(objectPtr));
}

CExoString::CExoString(char* src, int length)
    : GameAPIObject(nullptr, true) {  // true = will free (allocating new)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(8);

    cStrLenConstructor(static_cast<CExoString*>(objectPtr), src, length);
}

CExoString::CExoString(char* src)
    : GameAPIObject(nullptr, true) {  // true = will free (allocating new)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    objectPtr = malloc(8);

    cStrConstructor(static_cast<CExoString*>(objectPtr), src);
}

CExoString::~CExoString() {
    if (shouldFree && objectPtr) {
        destructor(static_cast<CExoString*>(objectPtr));
        free(objectPtr);
    }
    // Base class destructor handles setting objectPtr to nullptr
}

DWORD CExoString::GetLength() {
    if (!objectPtr || offsetLength < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetLength);
}

char* CExoString::GetCStr() {
    if (!objectPtr || offsetCStr < 0) {
        return nullptr;
    }
    return getObjectProperty<char*>(objectPtr, offsetCStr);
}