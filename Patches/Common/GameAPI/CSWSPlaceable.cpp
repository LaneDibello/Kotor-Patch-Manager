#include "CSWSPlaceable.h"
#include "GameVersion.h"

bool CSWSPlaceable::functionsInitialized = false;
bool CSWSPlaceable::offsetsInitialized = false;

void CSWSPlaceable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSPlaceable functions wrapped yet
    functionsInitialized = true;
}

void CSWSPlaceable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSPlaceable offsets wrapped yet
    offsetsInitialized = true;
}

CSWSPlaceable::CSWSPlaceable(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSPlaceable::~CSWSPlaceable() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
