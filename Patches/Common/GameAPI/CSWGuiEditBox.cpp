#include "CSWGuiEditBox.h"
#include "GameVersion.h"

// Note: DB uses class key "CSWGuiEditbox" (lowercase b) for lookups.
CSWGuiEditBox::GetIsSelectableFn CSWGuiEditBox::getIsSelectable = nullptr;
CSWGuiEditBox::ReSetFontFn       CSWGuiEditBox::reSetFont       = nullptr;
CSWGuiEditBox::SetEnabledFn      CSWGuiEditBox::setEnabled      = nullptr;
CSWGuiEditBox::SetFocusFn        CSWGuiEditBox::setFocus        = nullptr;
CSWGuiEditBox::ConstructorFn CSWGuiEditBox::constructor = nullptr;
CSWGuiEditBox::DestructorFn  CSWGuiEditBox::destructor  = nullptr;
int CSWGuiEditBox::classSize = -1;

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
        getIsSelectable = reinterpret_cast<GetIsSelectableFn>(GameVersion::GetFunctionAddress("CSWGuiEditbox", "GetIsSelectable"));
        reSetFont       = reinterpret_cast<ReSetFontFn>      (GameVersion::GetFunctionAddress("CSWGuiEditbox", "ReSetFont"));
        setEnabled      = reinterpret_cast<SetEnabledFn>     (GameVersion::GetFunctionAddress("CSWGuiEditbox", "SetEnabled"));
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
        // Offsets Here
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
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

bool CSWGuiEditBox::GetIsSelectable() {
    if (!objectPtr || !getIsSelectable) return false;
    return getIsSelectable(objectPtr);
}

void CSWGuiEditBox::ReSetFont() {
    if (!objectPtr || !reSetFont) return;
    reSetFont(objectPtr);
}

void CSWGuiEditBox::SetEnabled(UINT enabled) {
    if (!objectPtr || !setEnabled) return;
    setEnabled(objectPtr, enabled);
}

void CSWGuiEditBox::SetFocus() {
    if (!objectPtr || !setFocus) return;
    setFocus(objectPtr);
}
