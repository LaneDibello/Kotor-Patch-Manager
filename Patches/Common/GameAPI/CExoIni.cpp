#include "CExoIni.h"
#include "CExoString.h"

CExoIni::ConstructorFn CExoIni::constructor = nullptr;
CExoIni::DestructorFn  CExoIni::destructor = nullptr;
CExoIni::ReadIniEntryFn CExoIni::readIniEntry = nullptr;
CExoIni::WriteIniEntryFn CExoIni::writeIniEntry = nullptr;

bool CExoIni::functionsInitialized = false;
bool CExoIni::offsetsInitialized = false;

int CExoIni::classSize = -1;

void CExoIni::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        debugLog("[CExoIni] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CExoIni", "Constructor"));
        destructor = reinterpret_cast<DestructorFn>(GameVersion::GetFunctionAddress("CExoIni", "Destructor"));
        readIniEntry = reinterpret_cast<ReadIniEntryFn>(GameVersion::GetFunctionAddress("CExoIni", "ReadIniEntry"));
        writeIniEntry = reinterpret_cast<WriteIniEntryFn>(GameVersion::GetFunctionAddress("CExoIni", "WriteIniEntry"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoIni] ERROR: %s\n", e.what());
        return;
    }
}

void CExoIni::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoIni] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        classSize = GameVersion::GetClassSize("CExoIni");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoIni] ERROR: %s\n", e.what());
    }
}


CExoIni::CExoIni(void* objectPtr) : GameAPIObject(objectPtr, false) {
    InitializeFunctions();
    InitializeOffsets();
}

CExoIni::CExoIni() : GameAPIObject(nullptr, true) {
    InitializeFunctions();
    InitializeOffsets();

    objectPtr = (classSize > 0) ? malloc(classSize) : nullptr;

    if (objectPtr && constructor) {
        constructor(objectPtr);
    }
}

CExoIni::~CExoIni() {
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

int CExoIni::ReadIniEntry(CExoString* outValue, CExoString* filename, CExoString* category, CExoString* key) {
    if (!objectPtr || !readIniEntry) return;
    readIniEntry(objectPtr,
        outValue ? outValue->GetPtr() : nullptr,
        filename ? filename->GetPtr() : nullptr,
        category ? category->GetPtr() : nullptr,
        key ? key->GetPtr() : nullptr)
}

int CExoIni::WriteIniEntry(CExoString* value, CExoString* filename, CExoString* category, CExoString* key) {
    if (!objectPtr || !writeIniEntry) return;
    writeIniEntry(objectPtr,
        value ? value->GetPtr() : nullptr,
        filename ? filename->GetPtr() : nullptr,
        category ? category->GetPtr() : nullptr,
        key ? key->GetPtr() : nullptr)
}