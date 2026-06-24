#include "CSWGuiButton.h"
#include "GameVersion.h"
#include "CSWGuiText.h"

bool CSWGuiButton::functionsInitialized = false;
bool CSWGuiButton::offsetsInitialized = false;

int CSWGuiButton::offsetText = -1;

void CSWGuiButton::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButton] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButton] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiButton::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButton] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetText = GameVersion::GetOffset("CSWGuiButton", "text");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButton] ERROR: %s\n", e.what());
    }
}

CSWGuiButton::CSWGuiButton(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiButton::~CSWGuiButton()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiText* CSWGuiButton::GetText() {
    if (!objectPtr || offsetText < 0) {
        return nullptr;
    }
    return new CSWGuiText((char*)objectPtr + offsetText);
}
