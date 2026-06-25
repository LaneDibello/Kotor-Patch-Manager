#include "CSWGuiTextParams.h"
#include "CResRef.h"
#include "CExoString.h"
#include "CSWGuiText.h"
#include "GameVersion.h"

CSWGuiTextParams::RestoreDefaultColorFn CSWGuiTextParams::restoreDefaultColor = nullptr;
CSWGuiTextParams::SetAlignmentFn        CSWGuiTextParams::setAlignment        = nullptr;
CSWGuiTextParams::SetBaseFontFn         CSWGuiTextParams::setBaseFont         = nullptr;
CSWGuiTextParams::SetColorFn            CSWGuiTextParams::setColor            = nullptr;
CSWGuiTextParams::SetDefaultColorFn     CSWGuiTextParams::setDefaultColor     = nullptr;
CSWGuiTextParams::SetFontFn             CSWGuiTextParams::setFont             = nullptr;
CSWGuiTextParams::SetFontRenameFn       CSWGuiTextParams::setFontRename       = nullptr;
CSWGuiTextParams::SetStrRefFn           CSWGuiTextParams::setStrRef           = nullptr;
CSWGuiTextParams::SetTextFn             CSWGuiTextParams::setText             = nullptr;
CSWGuiTextParams::SetTextObjectFn       CSWGuiTextParams::setTextObject       = nullptr;
CSWGuiTextParams::AssignFn              CSWGuiTextParams::assign              = nullptr;
CSWGuiTextParams::ConstructorFn         CSWGuiTextParams::constructor         = nullptr;

bool CSWGuiTextParams::functionsInitialized = false;
bool CSWGuiTextParams::offsetsInitialized = false;

int CSWGuiTextParams::classSize = -1;

int CSWGuiTextParams::offsetText         = -1;
int CSWGuiTextParams::offsetStrRef       = -1;
int CSWGuiTextParams::offsetFont         = -1;
int CSWGuiTextParams::offsetColor        = -1;
int CSWGuiTextParams::offsetDefaultColor = -1;
int CSWGuiTextParams::offsetTextObject   = -1;
int CSWGuiTextParams::offsetOpacity      = -1;

void CSWGuiTextParams::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiTextParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        restoreDefaultColor = reinterpret_cast<RestoreDefaultColorFn>(GameVersion::GetFunctionAddress("CSWGuiTextParams", "RestoreDefaultColor"));
        setAlignment        = reinterpret_cast<SetAlignmentFn>       (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetAlignment"));
        setBaseFont         = reinterpret_cast<SetBaseFontFn>        (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetBaseFont"));
        setColor            = reinterpret_cast<SetColorFn>          (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetColor"));
        setDefaultColor     = reinterpret_cast<SetDefaultColorFn>    (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetDefaultColor"));
        setFont             = reinterpret_cast<SetFontFn>            (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetFont"));
        setFontRename       = reinterpret_cast<SetFontRenameFn>      (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetFontRename"));
        setStrRef           = reinterpret_cast<SetStrRefFn>          (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetStrRef"));
        setText             = reinterpret_cast<SetTextFn>            (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetText"));
        setTextObject       = reinterpret_cast<SetTextObjectFn>      (GameVersion::GetFunctionAddress("CSWGuiTextParams", "SetTextObject"));
        assign              = reinterpret_cast<AssignFn>             (GameVersion::GetFunctionAddress("CSWGuiTextParams", "operator="));
        constructor         = reinterpret_cast<ConstructorFn>       (GameVersion::GetFunctionAddress("CSWGuiTextParams", "Constructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiTextParams] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiTextParams::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiTextParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetText         = GameVersion::GetOffset("CSWGuiTextParams", "text");
        offsetStrRef       = GameVersion::GetOffset("CSWGuiTextParams", "str_ref");
        offsetFont         = GameVersion::GetOffset("CSWGuiTextParams", "font");
        offsetColor        = GameVersion::GetOffset("CSWGuiTextParams", "color");
        offsetDefaultColor = GameVersion::GetOffset("CSWGuiTextParams", "default_color");
        offsetTextObject   = GameVersion::GetOffset("CSWGuiTextParams", "text_object");
        offsetOpacity      = GameVersion::GetOffset("CSWGuiTextParams", "opacity");
        classSize          = GameVersion::GetClassSize("CSWGuiTextParams");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiTextParams] ERROR: %s\n", e.what());
    }
}

CSWGuiTextParams::CSWGuiTextParams(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    InitializeFunctions();
    InitializeOffsets();
}

CSWGuiTextParams::CSWGuiTextParams()
    : GameAPIObject(nullptr, false)
{
    InitializeFunctions();
    InitializeOffsets();

    if (classSize > 0 && constructor) {
        objectPtr = malloc(classSize);
        if (objectPtr) {
            constructor(objectPtr);
            shouldFree = true;
        }
    }
}

CSWGuiTextParams::~CSWGuiTextParams()
{
    // CSWGuiTextParams has no game destructor; just free if we own the memory.
    if (shouldFree && objectPtr) {
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

float CSWGuiTextParams::GetOpacity() {
    if (!objectPtr || offsetOpacity < 0) {
        return 0.0f;
    }
    return getObjectProperty<float>(objectPtr, offsetOpacity);
}

void CSWGuiTextParams::SetOpacity(float opacity) {
    if (!objectPtr || offsetOpacity < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetOpacity, opacity);
}

CExoString* CSWGuiTextParams::GetText() {
    if (!objectPtr || offsetText < 0) {
        return nullptr;
    }
    // Inline CExoString member: wrap its in-place address.
    return new CExoString(static_cast<void*>((char*)objectPtr + offsetText));
}

int CSWGuiTextParams::GetStrRef() {
    if (!objectPtr || offsetStrRef < 0) {
        return -1;
    }
    return getObjectProperty<int>(objectPtr, offsetStrRef);
}

CResRef* CSWGuiTextParams::GetFont() {
    if (!objectPtr || offsetFont < 0) {
        return nullptr;
    }
    // Inline CResRef member: wrap its in-place address.
    return new CResRef(static_cast<void*>((char*)objectPtr + offsetFont));
}

Vector CSWGuiTextParams::GetColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetColor);
}

Vector CSWGuiTextParams::GetDefaultColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetDefaultColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetDefaultColor);
}

CSWGuiText* CSWGuiTextParams::GetTextObject() {
    if (!objectPtr || offsetTextObject < 0) {
        return nullptr;
    }
    void* textPtr = getObjectProperty<void*>(objectPtr, offsetTextObject);
    if (!textPtr) {
        return nullptr;
    }
    return new CSWGuiText(textPtr);
}

void CSWGuiTextParams::RestoreDefaultColor() {
    if (!objectPtr || !restoreDefaultColor) return;
    restoreDefaultColor(objectPtr);
}

void CSWGuiTextParams::SetAlignment(int alignment) {
    if (!objectPtr || !setAlignment) return;
    setAlignment(objectPtr, alignment);
}

void CSWGuiTextParams::SetBaseFont(CResRef* font) {
    if (!objectPtr || !setBaseFont) return;
    setBaseFont(objectPtr, font ? font->GetPtr() : nullptr);
}

void CSWGuiTextParams::SetColor(Vector* color) {
    if (!objectPtr || !setColor) return;
    setColor(objectPtr, color);
}

void CSWGuiTextParams::SetDefaultColor(Vector* color) {
    if (!objectPtr || !setDefaultColor) return;
    setDefaultColor(objectPtr, color);
}

void CSWGuiTextParams::SetFont(CResRef* font) {
    if (!objectPtr || !setFont) return;
    setFont(objectPtr, font ? font->GetPtr() : nullptr);
}

void CSWGuiTextParams::SetFontRename() {
    if (!objectPtr || !setFontRename) return;
    setFontRename(objectPtr);
}

void CSWGuiTextParams::SetStrRef(unsigned long strRef) {
    if (!objectPtr || !setStrRef) return;
    setStrRef(objectPtr, strRef);
}

void CSWGuiTextParams::SetText(CExoString* text) {
    if (!objectPtr || !setText) return;
    setText(objectPtr, text ? text->GetPtr() : nullptr);
}

void CSWGuiTextParams::SetTextObject(CSWGuiText* text) {
    if (!objectPtr || !setTextObject) return;
    setTextObject(objectPtr, text ? text->GetPtr() : nullptr);
}

CSWGuiTextParams& CSWGuiTextParams::operator=(const CSWGuiTextParams& rhs) {
    if (this == &rhs) {
        return *this;
    }
    if (objectPtr && assign) {
        assign(objectPtr, rhs.GetPtr());
    }
    return *this;
}
