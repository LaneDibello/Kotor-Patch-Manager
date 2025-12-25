#include "CResRef.h"

bool CResRef::functionsInitialized = false;
bool CResRef::offsetsInitialized = false;

CResRef::CResRef(void* ptr)
    : GameAPIObject(ptr, false) {  // false = don't free (wrapping existing)
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

void CResRef::InitializeFunctions() {
    // CResRef has no functions yet
    functionsInitialized = true;
}

void CResRef::InitializeOffsets() {
    // CResRef has no offsets yet
    offsetsInitialized = true;
}

char* CResRef::GetCStr() {
    if (!objectPtr) {
        return nullptr;
    }

    // Allocate and copy the string (caller must free!)
    char* result = (char*)malloc(17);  // 16 chars + null terminator
    if (result) {
        memcpy(result, objectPtr, 16);
        result[16] = '\0';  // Ensure null termination
    }
    return result;
}