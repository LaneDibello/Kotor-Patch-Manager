#include "CSWGuiPanel.h"
#include "GameVersion.h"

bool CSWGuiPanel::functionsInitialized = false;
bool CSWGuiPanel::offsetsInitialized = false;

void CSWGuiPanel::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiPanel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiPanel] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiPanel::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiPanel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiPanel] ERROR: %s\n", e.what());
    }
}

CSWGuiPanel::CSWGuiPanel(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiPanel::~CSWGuiPanel()
{
    // Base class destructor handles objectPtr cleanup
}
