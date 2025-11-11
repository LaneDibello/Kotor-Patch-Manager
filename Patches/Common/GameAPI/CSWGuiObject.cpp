#include "CSWGuiObject.h"

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

CSWGuiObject::CSWGuiObject(void* optionsPtr)
    : objectPtr(objectPtr)
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
    objectPtr = nullptr;
}