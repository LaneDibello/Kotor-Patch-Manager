#include "CSWGuiScrollBar.h"
#include "CSWGuiBorder.h"
#include "CSWGuiImage.h"
#include "GameVersion.h"

bool CSWGuiScrollBar::functionsInitialized = false;
bool CSWGuiScrollBar::offsetsInitialized = false;
CSWGuiScrollBar::ConstructorFn CSWGuiScrollBar::constructor = nullptr;
CSWGuiScrollBar::DestructorFn  CSWGuiScrollBar::destructor  = nullptr;
CSWGuiScrollBar::HitCheckScrollbarFn CSWGuiScrollBar::hitCheckScrollbar = nullptr;
CSWGuiScrollBar::SetCurValueFn       CSWGuiScrollBar::setCurValueFn     = nullptr;
CSWGuiScrollBar::SetMaxValueFn       CSWGuiScrollBar::setMaxValueFn     = nullptr;
CSWGuiScrollBar::SetVisibleValueFn   CSWGuiScrollBar::setVisibleValueFn = nullptr;
int CSWGuiScrollBar::classSize = -1;
int CSWGuiScrollBar::offsetMaxValue         = -1;
int CSWGuiScrollBar::offsetCurrentValue     = -1;
int CSWGuiScrollBar::offsetVisibleItemCount = -1;
int CSWGuiScrollBar::offsetBorder           = -1;
int CSWGuiScrollBar::offsetImage1           = -1;
int CSWGuiScrollBar::offsetImage2           = -1;

void CSWGuiScrollBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiScrollBar", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiScrollBar", "Destructor"));
        hitCheckScrollbar = reinterpret_cast<HitCheckScrollbarFn>(GameVersion::GetFunctionAddress("CSWGuiScrollBar", "HitCheckScrollbar"));
        setCurValueFn     = reinterpret_cast<SetCurValueFn>     (GameVersion::GetFunctionAddress("CSWGuiScrollBar", "setCurValue"));
        setMaxValueFn     = reinterpret_cast<SetMaxValueFn>     (GameVersion::GetFunctionAddress("CSWGuiScrollBar", "setMaxValue"));
        setVisibleValueFn = reinterpret_cast<SetVisibleValueFn> (GameVersion::GetFunctionAddress("CSWGuiScrollBar", "setVisibleValue"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiScrollBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        offsetMaxValue         = GameVersion::GetOffset("CSWGuiScrollBar", "max_value");
        offsetCurrentValue     = GameVersion::GetOffset("CSWGuiScrollBar", "current_value");
        offsetVisibleItemCount = GameVersion::GetOffset("CSWGuiScrollBar", "visible_item_count");
        offsetBorder           = GameVersion::GetOffset("CSWGuiScrollBar", "border");
        offsetImage1           = GameVersion::GetOffset("CSWGuiScrollBar", "image_1");
        offsetImage2           = GameVersion::GetOffset("CSWGuiScrollBar", "image_2");
        classSize = GameVersion::GetClassSize("CSWGuiScrollBar");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
    }
}

CSWGuiScrollBar::CSWGuiScrollBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiScrollBar::CSWGuiScrollBar()
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

CSWGuiScrollBar::~CSWGuiScrollBar()
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

int CSWGuiScrollBar::GetMaxValue() {
    if (!objectPtr || offsetMaxValue < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMaxValue);
}

int CSWGuiScrollBar::GetCurrentValue() {
    if (!objectPtr || offsetCurrentValue < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCurrentValue);
}

int CSWGuiScrollBar::GetVisibleItemCount() {
    if (!objectPtr || offsetVisibleItemCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetVisibleItemCount);
}

CSWGuiBorder* CSWGuiScrollBar::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder);
}

CSWGuiImage* CSWGuiScrollBar::GetImage1() {
    if (!objectPtr || offsetImage1 < 0) {
        return nullptr;
    }
    // Inline CSWGuiImage member: wrap its in-place address.
    return new CSWGuiImage((char*)objectPtr + offsetImage1);
}

CSWGuiImage* CSWGuiScrollBar::GetImage2() {
    if (!objectPtr || offsetImage2 < 0) {
        return nullptr;
    }
    // Inline CSWGuiImage member: wrap its in-place address.
    return new CSWGuiImage((char*)objectPtr + offsetImage2);
}

int CSWGuiScrollBar::HitCheckScrollbar(int mouseX, int mouseY) {
    if (!objectPtr || !hitCheckScrollbar) return -1;
    return hitCheckScrollbar(objectPtr, mouseX, mouseY);
}

void CSWGuiScrollBar::setCurValue(int value) {
    if (!objectPtr || !setCurValueFn) return;
    setCurValueFn(objectPtr, value);
}

void CSWGuiScrollBar::setMaxValue(int value, BYTE _unknown) {
    if (!objectPtr || !setMaxValueFn) return;
    setMaxValueFn(objectPtr, value, _unknown);
}

void CSWGuiScrollBar::setVisibleValue(int value) {
    if (!objectPtr || !setVisibleValueFn) return;
    setVisibleValueFn(objectPtr, value);
}
