#include "CSWGuiButtonToggle.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiBorderParams.h"
#include "CSWGuiTextParams.h"
#include "CSWGuiExtent.h"

CSWGuiButtonToggle::SetSelectedFn CSWGuiButtonToggle::setSelected = nullptr;
CSWGuiButtonToggle::InitializeToggleParamsFn CSWGuiButtonToggle::initializeToggleParams = nullptr;
CSWGuiButtonToggle::ConstructorFn CSWGuiButtonToggle::constructor = nullptr;
CSWGuiButtonToggle::DestructorFn  CSWGuiButtonToggle::destructor  = nullptr;
int CSWGuiButtonToggle::classSize = -1;

bool CSWGuiButtonToggle::functionsInitialized = false;
bool CSWGuiButtonToggle::offsetsInitialized = false;

int CSWGuiButtonToggle::offsetToggleEvent = -1;
int CSWGuiButtonToggle::offsetBitFlags = -1;
int CSWGuiButtonToggle::offsetSelected = -1;
int CSWGuiButtonToggle::offsetHilightSelected = -1;

void * CSWGuiButtonToggle::vtableOptionsCheckbox = nullptr;

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
        initializeToggleParams = reinterpret_cast<InitializeToggleParamsFn>(GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "Initialize"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "Destructor_2"));

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
        offsetToggleEvent = GameVersion::GetOffset("CSWGuiButtonToggle", "toggle_event");
        offsetBitFlags = GameVersion::GetOffset("CSWGuiButtonToggle", "bit_flags");
        offsetSelected = GameVersion::GetOffset("CSWGuiButtonToggle", "selected");
        offsetHilightSelected = GameVersion::GetOffset("CSWGuiButtonToggle", "hilight_selected");
        classSize = GameVersion::GetClassSize("CSWGuiButtonToggle");
        vtableOptionsCheckbox = GameVersion::GetClassVtable("CSWGuiOptionsCheckbox");

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

CSWGuiButtonToggle::CSWGuiButtonToggle()
    : CSWGuiButton(nullptr)
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

CSWGuiButtonToggle::~CSWGuiButtonToggle()
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

CSWGuiBorder* CSWGuiButtonToggle::GetSelectedBorder() {
    if (!objectPtr || offsetSelected < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetSelected);
}

CSWGuiBorder* CSWGuiButtonToggle::GetHilightSelectedBorder() {
    if (!objectPtr || offsetHilightSelected < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetHilightSelected);
}

CSWGuiControl::GuiEvent CSWGuiButtonToggle::GetToggleEvent() {
    if (!objectPtr || offsetToggleEvent < 0) {
        return CSWGuiControl::HoverEnter;
    }
    return getObjectProperty<CSWGuiControl::GuiEvent>(objectPtr, offsetToggleEvent);
}

void CSWGuiButtonToggle::SetToggleEvent(CSWGuiControl::GuiEvent toggleEvent) {
    if (!objectPtr || offsetToggleEvent < 0) return;
    setObjectProperty<CSWGuiControl::GuiEvent>(objectPtr, offsetToggleEvent, toggleEvent);
}

int CSWGuiButtonToggle::GetBitFlags() {
    if (!objectPtr || offsetBitFlags < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBitFlags);
}

void CSWGuiButtonToggle::SetBitFlags(int bitFlags) {
    if (!objectPtr || offsetBitFlags < 0) return;
    setObjectProperty<int>(objectPtr, offsetBitFlags, bitFlags);
}

bool CSWGuiButtonToggle::GetSelected() {
    return (GetBitFlags() & 1) != 0;
}

void CSWGuiButtonToggle::SetSelected(UINT selected) {
    if (!objectPtr || !setSelected) return;
    setSelected(objectPtr, selected);
}

void CSWGuiButtonToggle::Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                                    CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightParams,
                                    CSWGuiBorderParams* selectedParams, CSWGuiBorderParams* hilightSelectedParams) {
    if (!objectPtr || !initializeToggleParams) return;
    initializeToggleParams(objectPtr, extent,
        textParams ? textParams->GetPtr() : nullptr,
        borderParams ? borderParams->GetPtr() : nullptr,
        hilightParams ? hilightParams->GetPtr() : nullptr,
        selectedParams ? selectedParams->GetPtr() : nullptr,
        hilightSelectedParams ? hilightSelectedParams->GetPtr() : nullptr);
}

void CSWGuiButtonToggle::SetOptionsCheckbox() {
    if (!objectPtr || !vtableOptionsCheckbox) return;
    setObjectProperty<void *>(objectPtr, 0, vtableOptionsCheckbox);
}
