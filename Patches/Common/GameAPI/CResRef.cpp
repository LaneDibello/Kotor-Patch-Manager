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

//TODO: implement class methods