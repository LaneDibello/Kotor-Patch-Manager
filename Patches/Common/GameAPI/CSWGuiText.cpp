#include "CSWGuiText.h"
#include "CSWGuiTextParams.h"
#include "GameVersion.h"

bool CSWGuiText::functionsInitialized = false;
bool CSWGuiText::offsetsInitialized = false;
CSWGuiText::ConstructorFn CSWGuiText::constructor = nullptr;
CSWGuiText::DestructorFn  CSWGuiText::destructor  = nullptr;
CSWGuiText::GetFontHeightFn  CSWGuiText::getFontHeight  = nullptr;
CSWGuiText::GetIdealHeightFn CSWGuiText::getIdealHeight = nullptr;
CSWGuiText::WrapTextFn       CSWGuiText::wrapTextFn      = nullptr;
int CSWGuiText::classSize = -1;
int CSWGuiText::offsetTextParams = -1;

void CSWGuiText::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiText", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiText", "Destructor"));
        getFontHeight  = reinterpret_cast<GetFontHeightFn> (GameVersion::GetFunctionAddress("CSWGuiText", "GetFontHeight"));
        getIdealHeight = reinterpret_cast<GetIdealHeightFn>(GameVersion::GetFunctionAddress("CSWGuiText", "GetIdealHeight"));
        wrapTextFn     = reinterpret_cast<WrapTextFn>      (GameVersion::GetFunctionAddress("CSWGuiText", "wrapText"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiText] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiText::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        offsetTextParams = GameVersion::GetOffset("CSWGuiText", "text_params");
        classSize = GameVersion::GetClassSize("CSWGuiText");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiText] ERROR: %s\n", e.what());
    }
}

CSWGuiText::CSWGuiText(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiText::CSWGuiText()
    : CSWGuiObject(nullptr)
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

CSWGuiText::~CSWGuiText()
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

CSWGuiTextParams* CSWGuiText::GetTextParams() {
    if (!objectPtr || offsetTextParams < 0) {
        return nullptr;
    }
    // Inline CSWGuiTextParams member: wrap its in-place address.
    return new CSWGuiTextParams((char*)objectPtr + offsetTextParams);
}

int CSWGuiText::GetFontHeight() {
    if (!objectPtr || !getFontHeight) return 0;
    return getFontHeight(objectPtr);
}

int CSWGuiText::GetIdealHeight() {
    if (!objectPtr || !getIdealHeight) return 0;
    return getIdealHeight(objectPtr);
}

void CSWGuiText::wrapText() {
    if (!objectPtr || !wrapTextFn) return;
    wrapTextFn(objectPtr);
}
