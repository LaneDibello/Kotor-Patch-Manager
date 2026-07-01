#include "CSWGuiButton.h"
#include "GameVersion.h"
#include "CSWGuiText.h"
#include "CSWGuiBorder.h"
#include "CSWGuiExtent.h"
#include "CSWGuiTextParams.h"
#include "CSWGuiBorderParams.h"

CSWGuiButton::ReSetFontFn  CSWGuiButton::reSetFont  = nullptr;
CSWGuiButton::SetActiveFn  CSWGuiButton::setActive  = nullptr;
CSWGuiButton::SetEnabledFn CSWGuiButton::setEnabled = nullptr;
CSWGuiButton::InitializeButtonFn CSWGuiButton::initializeButton = nullptr;
CSWGuiButton::InitializeParamsFn CSWGuiButton::initializeParams = nullptr;
CSWGuiButton::ConstructorFn CSWGuiButton::constructor = nullptr;
CSWGuiButton::DestructorFn  CSWGuiButton::destructor  = nullptr;
int CSWGuiButton::classSize = -1;

bool CSWGuiButton::functionsInitialized = false;
bool CSWGuiButton::offsetsInitialized = false;

int CSWGuiButton::offsetText = -1;
int CSWGuiButton::offsetBorder1 = -1;
int CSWGuiButton::offsetBorder2 = -1;

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
        initializeButton = reinterpret_cast<InitializeButtonFn>(GameVersion::GetFunctionAddress("CSWGuiButton", "Initialize_2"));
        initializeParams = reinterpret_cast<InitializeParamsFn>(GameVersion::GetFunctionAddress("CSWGuiButton", "Initialize"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiButton", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiButton", "Destructor"));

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
        offsetBorder1 = GameVersion::GetOffset("CSWGuiButton", "border_1");
        offsetBorder2 = GameVersion::GetOffset("CSWGuiButton", "border_2");
        classSize = GameVersion::GetClassSize("CSWGuiButton");

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

CSWGuiButton::CSWGuiButton()
    : CSWGuiNavigable(nullptr)
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

CSWGuiButton::~CSWGuiButton()
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

CSWGuiText* CSWGuiButton::GetText() {
    if (!objectPtr || offsetText < 0) {
        return nullptr;
    }
    return new CSWGuiText((char*)objectPtr + offsetText);
}

CSWGuiBorder* CSWGuiButton::GetBorder1() {
    if (!objectPtr || offsetBorder1 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder1);
}

CSWGuiBorder* CSWGuiButton::GetBorder2() {
    if (!objectPtr || offsetBorder2 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder2);
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

void CSWGuiButton::Initialize(CSWGuiExtent* extent, CSWGuiButton* button) {
    if (!objectPtr || !initializeButton) return;
    initializeButton(objectPtr, extent, button ? button->GetPtr() : nullptr);
}

void CSWGuiButton::Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                              CSWGuiBorderParams* border1Params, CSWGuiBorderParams* border2Params) {
    if (!objectPtr || !initializeParams) return;
    initializeParams(objectPtr, extent,
                     textParams ? textParams->GetPtr() : nullptr,
                     border1Params ? border1Params->GetPtr() : nullptr,
                     border2Params ? border2Params->GetPtr() : nullptr);
}
