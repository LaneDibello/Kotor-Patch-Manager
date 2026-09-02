#include "CSWCProjectile.h"
#include "GameVersion.h"

bool CSWCProjectile::functionsInitialized = false;
bool CSWCProjectile::offsetsInitialized = false;

void CSWCProjectile::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCProjectile functions wrapped yet
    functionsInitialized = true;
}

void CSWCProjectile::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCProjectile offsets wrapped yet
    offsetsInitialized = true;
}

CSWCProjectile::CSWCProjectile(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCProjectile::~CSWCProjectile() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
