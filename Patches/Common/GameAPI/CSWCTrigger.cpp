#include "CSWCTrigger.h"
#include "GameVersion.h"

bool CSWCTrigger::functionsInitialized = false;
bool CSWCTrigger::offsetsInitialized = false;

void CSWCTrigger::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCTrigger functions wrapped yet
    functionsInitialized = true;
}

void CSWCTrigger::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCTrigger offsets wrapped yet
    offsetsInitialized = true;
}

CSWCTrigger::CSWCTrigger(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCTrigger::~CSWCTrigger() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
