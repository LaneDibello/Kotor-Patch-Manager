#include "CSWGuiSlider.h"
#include "GameVersion.h"

bool CSWGuiSlider::functionsInitialized = false;
bool CSWGuiSlider::offsetsInitialized = false;

void CSWGuiSlider::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiSlider] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiSlider] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiSlider::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiSlider] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiSlider] ERROR: %s\n", e.what());
    }
}

CSWGuiSlider::CSWGuiSlider(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiSlider::~CSWGuiSlider()
{
    // Base class destructor handles objectPtr cleanup
}
