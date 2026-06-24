#include "CSWGuiScrollBar.h"
#include "GameVersion.h"

bool CSWGuiScrollBar::functionsInitialized = false;
bool CSWGuiScrollBar::offsetsInitialized = false;

void CSWGuiScrollBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiScrollBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
    }
}

CSWGuiScrollBar::CSWGuiScrollBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiScrollBar::~CSWGuiScrollBar()
{
    // Base class destructor handles objectPtr cleanup
}
