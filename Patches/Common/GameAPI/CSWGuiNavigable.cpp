#include "CSWGuiNavigable.h"
#include "GameVersion.h"

bool CSWGuiNavigable::functionsInitialized = false;
bool CSWGuiNavigable::offsetsInitialized = false;

void CSWGuiNavigable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiNavigable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiNavigable] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiNavigable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiNavigable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiNavigable] ERROR: %s\n", e.what());
    }
}

CSWGuiNavigable::CSWGuiNavigable(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiNavigable::~CSWGuiNavigable()
{
    // Base class destructor handles objectPtr cleanup
}
