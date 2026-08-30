#include "CSWSTrigger.h"
#include "GameVersion.h"

bool CSWSTrigger::functionsInitialized = false;
bool CSWSTrigger::offsetsInitialized = false;

void CSWSTrigger::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSTrigger functions wrapped yet
    functionsInitialized = true;
}

void CSWSTrigger::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSTrigger offsets wrapped yet
    offsetsInitialized = true;
}

CSWSTrigger::CSWSTrigger(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSTrigger::~CSWSTrigger() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
