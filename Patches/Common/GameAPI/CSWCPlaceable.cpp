#include "CSWCPlaceable.h"
#include "GameVersion.h"

bool CSWCPlaceable::functionsInitialized = false;
bool CSWCPlaceable::offsetsInitialized = false;

void CSWCPlaceable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCPlaceable functions wrapped yet
    functionsInitialized = true;
}

void CSWCPlaceable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCPlaceable offsets wrapped yet
    offsetsInitialized = true;
}

CSWCPlaceable::CSWCPlaceable(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCPlaceable::~CSWCPlaceable() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
