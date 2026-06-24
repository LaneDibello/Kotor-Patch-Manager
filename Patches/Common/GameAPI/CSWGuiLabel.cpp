#include "CSWGuiLabel.h"
#include "GameVersion.h"

bool CSWGuiLabel::functionsInitialized = false;
bool CSWGuiLabel::offsetsInitialized = false;

void CSWGuiLabel::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiLabel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiLabel] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiLabel::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiLabel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiLabel] ERROR: %s\n", e.what());
    }
}

CSWGuiLabel::CSWGuiLabel(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiLabel::~CSWGuiLabel()
{
    // Base class destructor handles objectPtr cleanup
}
