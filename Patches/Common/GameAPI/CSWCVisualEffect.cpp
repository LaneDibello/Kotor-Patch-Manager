#include "CSWCVisualEffect.h"
#include "GameVersion.h"

bool CSWCVisualEffect::functionsInitialized = false;
bool CSWCVisualEffect::offsetsInitialized = false;

void CSWCVisualEffect::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCVisualEffect functions wrapped yet
    functionsInitialized = true;
}

void CSWCVisualEffect::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCVisualEffect offsets wrapped yet
    offsetsInitialized = true;
}

CSWCVisualEffect::CSWCVisualEffect(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCVisualEffect::~CSWCVisualEffect() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
