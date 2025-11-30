#include "CSWGuiObject.h"

bool CSWGuiObject::functionsInitialized = false;
bool CSWGuiObject::offsetsInitialized = false;


void CSWGuiObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiObject] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        //Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCCreature] ERROR: %s\n", e.what());
    }
}

CSWGuiObject::CSWGuiObject(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiObject::~CSWGuiObject()
{
    // Base class destructor handles objectPtr cleanup
}