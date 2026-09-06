#include "CSWGuiEditBox.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiBorderParams.h"
#include "CSWGuiEditText.h"
#include "CSWGuiExtent.h"
#include "CSWGuiTextParams.h"

// Note: DB uses class key "CSWGuiEditbox" (lowercase b) for lookups.
CSWGuiEditBox::GetIsSelectableFn CSWGuiEditBox::getIsSelectable = nullptr;
CSWGuiEditBox::InitializeFn      CSWGuiEditBox::initialize      = nullptr;
CSWGuiEditBox::ReSetFontFn       CSWGuiEditBox::reSetFont       = nullptr;
CSWGuiEditBox::SetEnabledFn      CSWGuiEditBox::setEnabled      = nullptr;
CSWGuiEditBox::SetExtentFn       CSWGuiEditBox::setExtent       = nullptr;
CSWGuiEditBox::SetFocusFn        CSWGuiEditBox::setFocus        = nullptr;
CSWGuiEditBox::ConstructorFn CSWGuiEditBox::constructor = nullptr;
CSWGuiEditBox::DestructorFn  CSWGuiEditBox::destructor  = nullptr;
int CSWGuiEditBox::classSize = -1;

bool CSWGuiEditBox::functionsInitialized = false;
bool CSWGuiEditBox::offsetsInitialized = false;

int CSWGuiEditBox::offsetBorder = -1;
int CSWGuiEditBox::offsetEditText = -1;

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
        getIsSelectable = reinterpret_cast<GetIsSelectableFn>(GameVersion::GetFunctionAddress("CSWGuiEditbox", "GetIsSelectable"));
        reSetFont       = reinterpret_cast<ReSetFontFn>      (GameVersion::GetFunctionAddress("CSWGuiEditbox", "ReSetFont"));
        initialize      = reinterpret_cast<InitializeFn>      (GameVersion::GetFunctionAddress("CSWGuiEditbox", "Initialize"));
        setEnabled      = reinterpret_cast<SetEnabledFn>     (GameVersion::GetFunctionAddress("CSWGuiEditbox", "SetEnabled"));
        setExtent       = reinterpret_cast<SetExtentFn>      (GameVersion::GetFunctionAddress("CSWGuiEditbox", "SetExtent"));
        setFocus        = reinterpret_cast<SetFocusFn>       (GameVersion::GetFunctionAddress("CSWGuiEditbox", "SetFocus"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiEditbox", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiEditbox", "Destructor_2"));

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
        offsetBorder = GameVersion::GetOffset("CSWGuiEditbox", "border");
        offsetEditText = GameVersion::GetOffset("CSWGuiEditbox", "edit_text");
        classSize = GameVersion::GetClassSize("CSWGuiEditbox");

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

CSWGuiEditBox::CSWGuiEditBox()
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

CSWGuiEditBox::~CSWGuiEditBox()
{
    // Put the game's vtable back before the game's destructor runs (no-op unless
    // an override was installed).
    RestoreVTable();

    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

CSWGuiBorder* CSWGuiEditBox::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder);
}

CSWGuiEditText* CSWGuiEditBox::GetEditText() {
    if (!objectPtr || offsetEditText < 0) {
        return nullptr;
    }
    // Inline CSWGuiEditText member: wrap its in-place address.
    return new CSWGuiEditText((char*)objectPtr + offsetEditText);
}

bool CSWGuiEditBox::GetIsSelectable() {
    if (!objectPtr || !getIsSelectable) return false;
    return getIsSelectable(objectPtr);
}

void CSWGuiEditBox::Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                               CSWGuiBorderParams* borderParams) {
    if (!objectPtr || !initialize) return;
    initialize(objectPtr, extent,
               textParams ? textParams->GetPtr() : nullptr,
               borderParams ? borderParams->GetPtr() : nullptr);
}

void CSWGuiEditBox::ReSetFont() {
    if (!objectPtr || !reSetFont) return;
    reSetFont(objectPtr);
}

void CSWGuiEditBox::SetEnabled(UINT enabled) {
    if (!objectPtr || !setEnabled) return;
    setEnabled(objectPtr, enabled);
}

void CSWGuiEditBox::SetExtent(CSWGuiExtent* extent) {
    if (!objectPtr || !setExtent) return;
    setExtent(objectPtr, extent);
}

void CSWGuiEditBox::SetFocus() {
    if (!objectPtr || !setFocus) return;
    setFocus(objectPtr);
}

int CSWGuiEditBox::VTableSlotCount() {
    if (GameVersion::GetTitle() == GameTitle::KOTOR1 &&
        GameVersion::GetPlatform() == GamePlatform::Windows) {
        return EDITBOX_VTABLE_SLOT_COUNT;
    }

    debugLog("[CSWGuiEditBox] WARNING: editbox vtable layout unknown for this game version; vtable overriding disabled\n");
    return -1;
}
