#include "C2DA.h"
#include "GameVersion.h"
#include "../Common.h"
#include <cstring>

C2DA::GetCExoStringEntryFn C2DA::getCExoStringEntry = nullptr;
C2DA::GetFLOATEntryFn C2DA::getFLOATEntry = nullptr;
C2DA::GetINTEntryFn C2DA::getINTEntry = nullptr;
C2DA::Load2DArrayFn C2DA::load2DArray = nullptr;
C2DA::Unload2DArrayFn C2DA::unload2DArray = nullptr;
C2DA::ConstructorFn C2DA::constructor = nullptr;

bool C2DA::functionsInitialized = false;

void C2DA::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[C2DA] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        constructor = reinterpret_cast<ConstructorFn>(
            GameVersion::GetFunctionAddress("C2DA", "Constructor")
        );
        load2DArray = reinterpret_cast<Load2DArrayFn>(
            GameVersion::GetFunctionAddress("C2DA", "Load2DArray")
        );
        unload2DArray = reinterpret_cast<Unload2DArrayFn>(
            GameVersion::GetFunctionAddress("C2DA", "Unload2DArray")
        );
        getCExoStringEntry = reinterpret_cast<GetCExoStringEntryFn>(
            GameVersion::GetFunctionAddress("C2DA", "GetCExoStringEntry")
        );
        getFLOATEntry = reinterpret_cast<GetFLOATEntryFn>(
            GameVersion::GetFunctionAddress("C2DA", "GetFLOATEntry")
        );
        getINTEntry = reinterpret_cast<GetINTEntryFn>(
            GameVersion::GetFunctionAddress("C2DA", "GetINTEntry")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[C2DA] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

C2DA::C2DA(void* ptr)
    : ptr(ptr), shouldFree(false) {

    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

C2DA::C2DA(const char* name)
    : ptr(nullptr), shouldFree(true) {

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    // Allocate memory for the C2DA object
    ptr = malloc(objectSize);
    if (!ptr) {
        debugLog("[C2DA] ERROR: Failed to allocate memory for C2DA object\n");
        shouldFree = false;
        return;
    }

    // Prepare the CResRef_struct - must be exactly 16 characters
    CResRef_struct resRef;
    memset(resRef.str, 0, 16);

    // Copy the name, up to 16 characters
    size_t len = strlen(name);
    if (len > 16) {
        len = 16;
    }
    memcpy(resRef.str, name, len);

    // Call the constructor
    // Calling convention: ECX->this, Stack[0x4]->name (16 bytes), Stack[0x14]->usually0
    if (constructor) {
        constructor(ptr, resRef, 0);
    } else {
        debugLog("[C2DA] ERROR: Constructor function not initialized\n");
        free(ptr);
        ptr = nullptr;
        shouldFree = false;
        return;
    }

    // Load the 2DA data
    Load2DArray();
}

C2DA::~C2DA() {
    if (shouldFree && ptr) {
        // Unload the 2DA data before freeing
        Unload2DArray();
        free(ptr);
    }
    ptr = nullptr;
}

bool C2DA::GetCExoStringEntry(int row, CExoString* column, CExoString* output) {
    if (!ptr || !getCExoStringEntry) {
        return false;
    }
    return getCExoStringEntry(ptr, row, column->GetPtr(), output->GetPtr());
}

bool C2DA::GetFLOATEntry(int row, CExoString* column, float* output) {
    if (!ptr || !getFLOATEntry) {
        return false;
    }
    return getFLOATEntry(ptr, row, column->GetPtr(), output);
}

bool C2DA::GetINTEntry(int row, CExoString* column, int* output) {
    if (!ptr || !getINTEntry) {
        return false;
    }
    return getINTEntry(ptr, row, column->GetPtr(), output);
}

void C2DA::Load2DArray() {
    if (!ptr || !load2DArray) {
        return;
    }
    load2DArray(ptr);
}

void C2DA::Unload2DArray() {
    if (!ptr || !unload2DArray) {
        return;
    }
    unload2DArray(ptr);
}
