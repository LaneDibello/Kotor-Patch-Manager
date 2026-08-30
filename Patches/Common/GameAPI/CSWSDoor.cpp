#include "CSWSDoor.h"
#include "GameVersion.h"

bool CSWSDoor::functionsInitialized = false;
bool CSWSDoor::offsetsInitialized = false;

void CSWSDoor::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSDoor functions wrapped yet
    functionsInitialized = true;
}

void CSWSDoor::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSDoor offsets wrapped yet
    offsetsInitialized = true;
}

CSWSDoor::CSWSDoor(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSDoor::~CSWSDoor() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
