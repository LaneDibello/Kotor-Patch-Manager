#include "CSWCObject.h"
#include "GameVersion.h"

bool CSWCObject::functionsInitialized = false;
bool CSWCObject::offsetsInitialized = false;

void CSWCObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CGameObject::InitializeFunctions();

    // No CSWCObject functions wrapped yet
    functionsInitialized = true;
}

void CSWCObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CGameObject::InitializeOffsets();

    // No CSWCObject offsets wrapped yet
    offsetsInitialized = true;
}

CSWCObject::CSWCObject(void* objectPtr)
    : CGameObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCObject::~CSWCObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
