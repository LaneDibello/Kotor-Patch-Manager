#include "CSWGuiSlider.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiImage.h"
#include "CSWGuiBorderParams.h"
#include "CSWGuiExtent.h"
#include "CResRef.h"

CSWGuiSlider::InitializeFn CSWGuiSlider::initialize = nullptr;
CSWGuiSlider::SetExtentFn  CSWGuiSlider::setExtent  = nullptr;

bool CSWGuiSlider::functionsInitialized = false;
bool CSWGuiSlider::offsetsInitialized = false;

int CSWGuiSlider::offsetMaxValue = -1;
int CSWGuiSlider::offsetCurValue = -1;
int CSWGuiSlider::offsetBorder = -1;
int CSWGuiSlider::offsetBorderHilight = -1;
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
        initialize = reinterpret_cast<InitializeFn>(GameVersion::GetFunctionAddress("CSWGuiSlider", "Initialize"));
        setExtent  = reinterpret_cast<SetExtentFn> (GameVersion::GetFunctionAddress("CSWGuiSlider", "SetExtent"));

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
        offsetBorder = GameVersion::GetOffset("CSWGuiSlider", "border");
        offsetBorderHilight = GameVersion::GetOffset("CSWGuiSlider", "border_hilight");
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

CSWGuiBorder* CSWGuiSlider::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder);
}

CSWGuiBorder* CSWGuiSlider::GetBorderHilight() {
    if (!objectPtr || offsetBorderHilight < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorderHilight);
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

void CSWGuiSlider::Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams,
                              CSWGuiBorderParams* borderHilightParams, CResRef* imageResRef) {
    if (!objectPtr || !initialize) return;
    initialize(objectPtr, extent,
               borderParams ? borderParams->GetPtr() : nullptr,
               borderHilightParams ? borderHilightParams->GetPtr() : nullptr,
               imageResRef ? imageResRef->GetPtr() : nullptr);
}

void CSWGuiSlider::SetExtent(CSWGuiExtent* extent) {
    if (!objectPtr || !setExtent) return;
    setExtent(objectPtr, extent);
}
