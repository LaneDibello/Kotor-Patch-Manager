#include "CSWGuiListBox.h"
#include "GameVersion.h"

bool CSWGuiListBox::functionsInitialized = false;
bool CSWGuiListBox::offsetsInitialized = false;

void CSWGuiListBox::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiListBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiListBox] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiListBox::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiListBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiListBox] ERROR: %s\n", e.what());
    }
}

CSWGuiListBox::CSWGuiListBox(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiListBox::~CSWGuiListBox()
{
    // Base class destructor handles objectPtr cleanup
}
