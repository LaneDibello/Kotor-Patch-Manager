#include "CSWSStore.h"
#include "GameVersion.h"

bool CSWSStore::functionsInitialized = false;
bool CSWSStore::offsetsInitialized = false;

void CSWSStore::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSStore functions wrapped yet
    functionsInitialized = true;
}

void CSWSStore::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSStore offsets wrapped yet
    offsetsInitialized = true;
}

CSWSStore::CSWSStore(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSStore::~CSWSStore() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
