#include "CSWGuiBorder.h"
#include "GameVersion.h"

bool CSWGuiBorder::functionsInitialized = false;
bool CSWGuiBorder::offsetsInitialized = false;

void CSWGuiBorder::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorder] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorder] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiBorder::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorder] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorder] ERROR: %s\n", e.what());
    }
}

CSWGuiBorder::CSWGuiBorder(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiBorder::~CSWGuiBorder()
{
    // Base class destructor handles objectPtr cleanup
}
