#include "CSWSAreaOfEffectObject.h"
#include "GameVersion.h"

bool CSWSAreaOfEffectObject::functionsInitialized = false;
bool CSWSAreaOfEffectObject::offsetsInitialized = false;

void CSWSAreaOfEffectObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSAreaOfEffectObject functions wrapped yet
    functionsInitialized = true;
}

void CSWSAreaOfEffectObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSAreaOfEffectObject offsets wrapped yet
    offsetsInitialized = true;
}

CSWSAreaOfEffectObject::CSWSAreaOfEffectObject(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSAreaOfEffectObject::~CSWSAreaOfEffectObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
