#include "CSWGuiEditBox.h"
#include "GameVersion.h"

bool CSWGuiEditBox::functionsInitialized = false;
bool CSWGuiEditBox::offsetsInitialized = false;

void CSWGuiEditBox::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiEditBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiEditBox] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiEditBox::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiEditBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiEditBox] ERROR: %s\n", e.what());
    }
}

CSWGuiEditBox::CSWGuiEditBox(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiEditBox::~CSWGuiEditBox()
{
    // Base class destructor handles objectPtr cleanup
}
