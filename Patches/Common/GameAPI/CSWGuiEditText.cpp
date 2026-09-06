#include "CSWGuiEditText.h"
#include "GameVersion.h"
#include "CExoString.h"

CSWGuiEditText::AddNewCharFn      CSWGuiEditText::addNewChar      = nullptr;
CSWGuiEditText::RemoveLastCharFn  CSWGuiEditText::removeLastChar  = nullptr;
CSWGuiEditText::SetCaretVisibleFn CSWGuiEditText::setCaretVisible = nullptr;
CSWGuiEditText::SetTextFn         CSWGuiEditText::setText         = nullptr;
CSWGuiEditText::ConstructorFn CSWGuiEditText::constructor = nullptr;
CSWGuiEditText::DestructorFn  CSWGuiEditText::destructor  = nullptr;
int CSWGuiEditText::classSize = -1;

bool CSWGuiEditText::functionsInitialized = false;
bool CSWGuiEditText::offsetsInitialized = false;

int CSWGuiEditText::offsetMaxLength = -1;
int CSWGuiEditText::offsetString = -1;

void CSWGuiEditText::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiText::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiEditText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addNewChar      = reinterpret_cast<AddNewCharFn>     (GameVersion::GetFunctionAddress("CSWGuiEditText", "AddNewChar"));
        removeLastChar  = reinterpret_cast<RemoveLastCharFn> (GameVersion::GetFunctionAddress("CSWGuiEditText", "RemoveLastChar"));
        setCaretVisible = reinterpret_cast<SetCaretVisibleFn>(GameVersion::GetFunctionAddress("CSWGuiEditText", "SetCaretVisible"));
        setText         = reinterpret_cast<SetTextFn>        (GameVersion::GetFunctionAddress("CSWGuiEditText", "SetText"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiEditText", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiEditText", "Destructor_2"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiEditText] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiEditText::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiText::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiEditText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetMaxLength = GameVersion::GetOffset("CSWGuiEditText", "max_length");
        offsetString = GameVersion::GetOffset("CSWGuiEditText", "string");
        classSize = GameVersion::GetClassSize("CSWGuiEditText");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiEditText] ERROR: %s\n", e.what());
    }
}

CSWGuiEditText::CSWGuiEditText(void* objectPtr)
    : CSWGuiText(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiEditText::CSWGuiEditText()
    : CSWGuiText(nullptr)
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

CSWGuiEditText::~CSWGuiEditText()
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

CExoString* CSWGuiEditText::GetString() {
    if (!objectPtr || offsetString < 0) {
        return nullptr;
    }
    // Inline CExoString member: wrap its in-place address.
    // The void* cast is load-bearing: a bare `(char*)objectPtr + offsetString` is an
    // exact match for CExoString(char* src), which BUILDS a new string by copying C
    // string bytes from that address -- so this handed back the inline CExoString's
    // own pointer field rendered as text instead of wrapping the string in place.
    return new CExoString(static_cast<void*>((char*)objectPtr + offsetString));
}

short CSWGuiEditText::GetMaxLength() {
    if (!objectPtr || offsetMaxLength < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetMaxLength);
}

void CSWGuiEditText::SetMaxLength(short maxLength) {
    if (!objectPtr || offsetMaxLength < 0) return;
    setObjectProperty<short>(objectPtr, offsetMaxLength, maxLength);
}

void CSWGuiEditText::AddNewChar(char* character) {
    if (!objectPtr || !addNewChar) return;
    addNewChar(objectPtr, character);
}

void CSWGuiEditText::RemoveLastChar() {
    if (!objectPtr || !removeLastChar) return;
    removeLastChar(objectPtr);
}

void CSWGuiEditText::SetCaretVisible(int visible) {
    if (!objectPtr || !setCaretVisible) return;
    setCaretVisible(objectPtr, visible);
}

void CSWGuiEditText::SetText(CExoString* text) {
    if (!objectPtr || !setText) return;
    setText(objectPtr, text ? text->GetPtr() : nullptr);
}
