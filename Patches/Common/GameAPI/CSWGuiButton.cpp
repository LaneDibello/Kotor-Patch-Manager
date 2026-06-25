#include "CSWGuiButton.h"
#include "GameVersion.h"
#include "CSWGuiText.h"

CSWGuiButton::ReSetFontFn  CSWGuiButton::reSetFont  = nullptr;
CSWGuiButton::SetActiveFn  CSWGuiButton::setActive  = nullptr;
CSWGuiButton::SetEnabledFn CSWGuiButton::setEnabled = nullptr;

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
        reSetFont  = reinterpret_cast<ReSetFontFn> (GameVersion::GetFunctionAddress("CSWGuiButton", "ReSetFont"));
        setActive  = reinterpret_cast<SetActiveFn> (GameVersion::GetFunctionAddress("CSWGuiButton", "SetActive"));
        setEnabled = reinterpret_cast<SetEnabledFn>(GameVersion::GetFunctionAddress("CSWGuiButton", "SetEnabled"));

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

void CSWGuiButton::ReSetFont() {
    if (!objectPtr || !reSetFont) return;
    reSetFont(objectPtr);
}

void CSWGuiButton::SetActive(UINT active) {
    if (!objectPtr || !setActive) return;
    setActive(objectPtr, active);
}

void CSWGuiButton::SetEnabled(UINT enabled) {
    if (!objectPtr || !setEnabled) return;
    setEnabled(objectPtr, enabled);
}
