#include "CSWGuiLabel.h"
#include "GameVersion.h"
#include "CSWGuiText.h"

CSWGuiLabel::ReSetFontFn  CSWGuiLabel::reSetFont  = nullptr;
CSWGuiLabel::SetEnabledFn CSWGuiLabel::setEnabled = nullptr;
CSWGuiLabel::ConstructorFn CSWGuiLabel::constructor = nullptr;
CSWGuiLabel::DestructorFn  CSWGuiLabel::destructor  = nullptr;
int CSWGuiLabel::classSize = -1;

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
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiLabel", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiLabel", "Destructor"));

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
        classSize = GameVersion::GetClassSize("CSWGuiLabel");

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

CSWGuiLabel::CSWGuiLabel()
    : CSWGuiControl(nullptr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    if (classSize > 0 && constructor) {
        objectPtr = malloc(classSize);
        if (objectPtr) {
            constructor(objectPtr);
            shouldFree = true;
        }
    }
}

CSWGuiLabel::~CSWGuiLabel()
{
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
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
