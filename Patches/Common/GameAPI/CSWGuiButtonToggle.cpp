#include "CSWGuiButtonToggle.h"
#include "GameVersion.h"

CSWGuiButtonToggle::SetSelectedFn CSWGuiButtonToggle::setSelected = nullptr;

bool CSWGuiButtonToggle::functionsInitialized = false;
bool CSWGuiButtonToggle::offsetsInitialized = false;

void CSWGuiButtonToggle::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiButton::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButtonToggle] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        setSelected = reinterpret_cast<SetSelectedFn>(GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "SetSelected"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButtonToggle] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiButtonToggle::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiButton::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButtonToggle] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButtonToggle] ERROR: %s\n", e.what());
    }
}

CSWGuiButtonToggle::CSWGuiButtonToggle(void* objectPtr)
    : CSWGuiButton(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiButtonToggle::~CSWGuiButtonToggle()
{
    // Base class destructor handles objectPtr cleanup
}

void CSWGuiButtonToggle::SetSelected(UINT selected) {
    if (!objectPtr || !setSelected) return;
    setSelected(objectPtr, selected);
}
