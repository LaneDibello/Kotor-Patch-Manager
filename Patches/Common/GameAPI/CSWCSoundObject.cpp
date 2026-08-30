#include "CSWCSoundObject.h"
#include "GameVersion.h"

bool CSWCSoundObject::functionsInitialized = false;
bool CSWCSoundObject::offsetsInitialized = false;

void CSWCSoundObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCSoundObject functions wrapped yet
    functionsInitialized = true;
}

void CSWCSoundObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCSoundObject offsets wrapped yet
    offsetsInitialized = true;
}

CSWCSoundObject::CSWCSoundObject(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCSoundObject::~CSWCSoundObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
