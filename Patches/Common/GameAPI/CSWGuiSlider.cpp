#include "CSWGuiSlider.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiImage.h"
#include "CSWGuiBorderParams.h"
#include "CSWGuiExtent.h"
#include "CResRef.h"

CSWGuiSlider::InitializeFn       CSWGuiSlider::initialize       = nullptr;
CSWGuiSlider::SetExtentFn        CSWGuiSlider::setExtent        = nullptr;
CSWGuiSlider::SetMaxValueFn      CSWGuiSlider::setMaxValue      = nullptr;
CSWGuiSlider::SetCurValueFn      CSWGuiSlider::setCurValue      = nullptr;
CSWGuiSlider::DrawFn             CSWGuiSlider::draw             = nullptr;
CSWGuiSlider::HandleInputEventFn CSWGuiSlider::handleInputEvent = nullptr;
CSWGuiSlider::HitCheckSliderFn   CSWGuiSlider::hitCheckSlider   = nullptr;
CSWGuiSlider::ConstructorFn      CSWGuiSlider::constructor      = nullptr;
CSWGuiSlider::DestructorFn       CSWGuiSlider::destructor       = nullptr;
int CSWGuiSlider::classSize = -1;

bool CSWGuiSlider::functionsInitialized = false;
bool CSWGuiSlider::offsetsInitialized = false;

int CSWGuiSlider::offsetMaxValue = -1;
int CSWGuiSlider::offsetCurValue = -1;
int CSWGuiSlider::offsetGuiSound = -1;
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
        initialize       = reinterpret_cast<InitializeFn>      (GameVersion::GetFunctionAddress("CSWGuiSlider", "Initialize"));
        setExtent        = reinterpret_cast<SetExtentFn>       (GameVersion::GetFunctionAddress("CSWGuiSlider", "SetExtent"));
        setMaxValue      = reinterpret_cast<SetMaxValueFn>     (GameVersion::GetFunctionAddress("CSWGuiSlider", "SetMaxValue"));
        setCurValue      = reinterpret_cast<SetCurValueFn>     (GameVersion::GetFunctionAddress("CSWGuiSlider", "SetCurValue"));
        draw             = reinterpret_cast<DrawFn>            (GameVersion::GetFunctionAddress("CSWGuiSlider", "Draw"));
        handleInputEvent = reinterpret_cast<HandleInputEventFn>(GameVersion::GetFunctionAddress("CSWGuiSlider", "HandleInputEvent"));
        hitCheckSlider   = reinterpret_cast<HitCheckSliderFn>  (GameVersion::GetFunctionAddress("CSWGuiSlider", "HitCheckSlider"));
        constructor      = reinterpret_cast<ConstructorFn>     (GameVersion::GetFunctionAddress("CSWGuiSlider", "Constructor"));
        // Destructor_2 is the plain one; Destructor is the scalar-deleting variant.
        destructor       = reinterpret_cast<DestructorFn>      (GameVersion::GetFunctionAddress("CSWGuiSlider", "Destructor_2"));

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
        offsetGuiSound = GameVersion::GetOffset("CSWGuiSlider", "gui_sound");
        offsetBorder = GameVersion::GetOffset("CSWGuiSlider", "border");
        offsetBorderHilight = GameVersion::GetOffset("CSWGuiSlider", "border_hilight");
        offsetImage = GameVersion::GetOffset("CSWGuiSlider", "image");
        classSize = GameVersion::GetClassSize("CSWGuiSlider");

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

CSWGuiSlider::CSWGuiSlider()
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

CSWGuiSlider::~CSWGuiSlider()
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

BYTE CSWGuiSlider::GetGuiSound() {
    if (!objectPtr || offsetGuiSound < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetGuiSound);
}

void CSWGuiSlider::SetGuiSound(BYTE sound) {
    if (!objectPtr || offsetGuiSound < 0) return;
    setObjectProperty<BYTE>(objectPtr, offsetGuiSound, sound);
}

void CSWGuiSlider::SetMaxValue(int maxValue) {
    if (!objectPtr || !setMaxValue) return;
    setMaxValue(objectPtr, maxValue);
}

void CSWGuiSlider::SetCurValue(int curValue) {
    if (!objectPtr || !setCurValue) return;
    setCurValue(objectPtr, curValue);
}

void CSWGuiSlider::Draw(float alpha) {
    if (!objectPtr || !draw) return;
    draw(objectPtr, alpha);
}

void CSWGuiSlider::HandleInputEvent(int event, int inputPhase) {
    if (!objectPtr || !handleInputEvent) return;
    handleInputEvent(objectPtr, event, inputPhase);
}

int CSWGuiSlider::VTableSlotCount() {
    if (GameVersion::GetTitle() == GameTitle::KOTOR1 &&
        GameVersion::GetPlatform() == GamePlatform::Windows) {
        return SLIDER_VTABLE_SLOT_COUNT;
    }

    debugLog("[CSWGuiSlider] WARNING: slider vtable layout unknown for this game version; vtable overriding disabled");
    return -1;
}

int CSWGuiSlider::HitCheckSlider(int x, int y) {
    if (!objectPtr || !hitCheckSlider) return 0;
    return hitCheckSlider(objectPtr, x, y);
}
