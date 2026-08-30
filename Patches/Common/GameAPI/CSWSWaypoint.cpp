#include "CSWSWaypoint.h"
#include "GameVersion.h"

bool CSWSWaypoint::functionsInitialized = false;
bool CSWSWaypoint::offsetsInitialized = false;

void CSWSWaypoint::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSWaypoint functions wrapped yet
    functionsInitialized = true;
}

void CSWSWaypoint::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSWaypoint offsets wrapped yet
    offsetsInitialized = true;
}

CSWSWaypoint::CSWSWaypoint(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSWaypoint::~CSWSWaypoint() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}
