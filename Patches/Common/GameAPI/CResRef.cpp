#include "CResRef.h"
#include <cstring>

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

CResRef::CResRef(const char* src)
    : GameAPIObject(nullptr, true) {  // true = we own this buffer
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // A CResRef is a fixed 16-byte field, zero-padded (and not necessarily
    // null-terminated when full). Allocate it locally, zero it, then copy at
    // most 16 characters from the source string.
    objectPtr = malloc(sizeof(CResRef_struct));
    if (objectPtr) {
        memset(objectPtr, 0, sizeof(CResRef_struct));
        if (src) {
            size_t len = strlen(src);
            if (len > sizeof(CResRef_struct)) {
                len = sizeof(CResRef_struct);
            }
            memcpy(objectPtr, src, len);
        }
    }
}

CResRef::~CResRef() {
    if (shouldFree && objectPtr) {
        free(objectPtr);
        objectPtr = nullptr;
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