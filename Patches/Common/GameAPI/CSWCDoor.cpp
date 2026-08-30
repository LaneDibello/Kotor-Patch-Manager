#include "CSWCDoor.h"
#include "GameVersion.h"

bool CSWCDoor::functionsInitialized = false;
bool CSWCDoor::offsetsInitialized = false;

void CSWCDoor::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCDoor functions wrapped yet
    functionsInitialized = true;
}

void CSWCDoor::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCDoor offsets wrapped yet
    offsetsInitialized = true;
}

CSWCDoor::CSWCDoor(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCDoor::~CSWCDoor() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
