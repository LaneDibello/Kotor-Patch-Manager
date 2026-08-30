#include "CSWCAreaOfEffectObject.h"
#include "GameVersion.h"

bool CSWCAreaOfEffectObject::functionsInitialized = false;
bool CSWCAreaOfEffectObject::offsetsInitialized = false;

void CSWCAreaOfEffectObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCAreaOfEffectObject functions wrapped yet
    functionsInitialized = true;
}

void CSWCAreaOfEffectObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCAreaOfEffectObject offsets wrapped yet
    offsetsInitialized = true;
}

CSWCAreaOfEffectObject::CSWCAreaOfEffectObject(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCAreaOfEffectObject::~CSWCAreaOfEffectObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
