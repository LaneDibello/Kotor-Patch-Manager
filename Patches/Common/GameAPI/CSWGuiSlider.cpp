#include "CSWGuiSlider.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiImage.h"

bool CSWGuiSlider::functionsInitialized = false;
bool CSWGuiSlider::offsetsInitialized = false;

int CSWGuiSlider::offsetMaxValue = -1;
int CSWGuiSlider::offsetCurValue = -1;
int CSWGuiSlider::offsetBorder1 = -1;
int CSWGuiSlider::offsetBorder2 = -1;
int CSWGuiSlider::offsetImage = -1;

void CSWGuiSlider::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiSlider] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiSlider] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiSlider::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiSlider] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetMaxValue = GameVersion::GetOffset("CSWGuiSlider", "max_value");
        offsetCurValue = GameVersion::GetOffset("CSWGuiSlider", "cur_value");
        offsetBorder1 = GameVersion::GetOffset("CSWGuiSlider", "border_1");
        offsetBorder2 = GameVersion::GetOffset("CSWGuiSlider", "border_2");
        offsetImage = GameVersion::GetOffset("CSWGuiSlider", "image");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiSlider] ERROR: %s\n", e.what());
    }
}

CSWGuiSlider::CSWGuiSlider(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiSlider::~CSWGuiSlider()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiBorder* CSWGuiSlider::GetBorder1() {
    if (!objectPtr || offsetBorder1 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder1);
}

CSWGuiBorder* CSWGuiSlider::GetBorder2() {
    if (!objectPtr || offsetBorder2 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder2);
}

CSWGuiImage* CSWGuiSlider::GetImage() {
    if (!objectPtr || offsetImage < 0) {
        return nullptr;
    }
    // Inline CSWGuiImage member: wrap its in-place address.
    return new CSWGuiImage((char*)objectPtr + offsetImage);
}

int CSWGuiSlider::GetMaxValue() {
    if (!objectPtr || offsetMaxValue < 0) {
        return -1;
    }
    return getObjectProperty<int>(objectPtr, offsetMaxValue);
}

int CSWGuiSlider::GetCurValue() {
    if (!objectPtr || offsetCurValue < 0) {
        return -1;
    }
    return getObjectProperty<int>(objectPtr, offsetCurValue);
}
