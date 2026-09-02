#include "CSWCMapPin.h"
#include "GameVersion.h"

bool CSWCMapPin::functionsInitialized = false;
bool CSWCMapPin::offsetsInitialized = false;

void CSWCMapPin::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCMapPin functions wrapped yet
    functionsInitialized = true;
}

void CSWCMapPin::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCMapPin offsets wrapped yet
    offsetsInitialized = true;
}

CSWCMapPin::CSWCMapPin(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCMapPin::~CSWCMapPin() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
