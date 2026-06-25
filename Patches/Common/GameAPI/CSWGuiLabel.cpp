#include "CSWGuiLabel.h"
#include "GameVersion.h"
#include "CSWGuiText.h"

CSWGuiLabel::ReSetFontFn  CSWGuiLabel::reSetFont  = nullptr;
CSWGuiLabel::SetEnabledFn CSWGuiLabel::setEnabled = nullptr;

bool CSWGuiLabel::functionsInitialized = false;
bool CSWGuiLabel::offsetsInitialized = false;

int CSWGuiLabel::offsetText = -1;

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
        reSetFont  = reinterpret_cast<ReSetFontFn> (GameVersion::GetFunctionAddress("CSWGuiLabel", "ReSetFont"));
        setEnabled = reinterpret_cast<SetEnabledFn>(GameVersion::GetFunctionAddress("CSWGuiLabel", "SetEnabled"));

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
        offsetText = GameVersion::GetOffset("CSWGuiLabel", "text");

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

CSWGuiText* CSWGuiLabel::GetText() {
    if (!objectPtr || offsetText < 0) {
        return nullptr;
    }
    return new CSWGuiText((char*)objectPtr + offsetText);
}

void CSWGuiLabel::ReSetFont() {
    if (!objectPtr || !reSetFont) return;
    reSetFont(objectPtr);
}

void CSWGuiLabel::SetEnabled(UINT enabled) {
    if (!objectPtr || !setEnabled) return;
    setEnabled(objectPtr, enabled);
}
