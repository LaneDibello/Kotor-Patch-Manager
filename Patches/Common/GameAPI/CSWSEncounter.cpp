#include "CSWSEncounter.h"
#include "GameVersion.h"

bool CSWSEncounter::functionsInitialized = false;
bool CSWSEncounter::offsetsInitialized = false;

void CSWSEncounter::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSEncounter functions wrapped yet
    functionsInitialized = true;
}

void CSWSEncounter::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSEncounter offsets wrapped yet
    offsetsInitialized = true;
}

CSWSEncounter::CSWSEncounter(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSEncounter::~CSWSEncounter() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
