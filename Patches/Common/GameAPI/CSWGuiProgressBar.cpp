#include "CSWGuiProgressBar.h"
#include "GameVersion.h"

bool CSWGuiProgressBar::functionsInitialized = false;
bool CSWGuiProgressBar::offsetsInitialized = false;

void CSWGuiProgressBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiProgressBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
    }
}

CSWGuiProgressBar::CSWGuiProgressBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiProgressBar::~CSWGuiProgressBar()
{
    // Base class destructor handles objectPtr cleanup
}
