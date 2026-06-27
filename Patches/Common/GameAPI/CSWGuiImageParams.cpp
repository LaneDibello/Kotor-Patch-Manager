#include "CSWGuiImageParams.h"
#include "CResRef.h"
#include "CSWGuiImage.h"
#include "GameVersion.h"

CSWGuiImageParams::SetImageFn       CSWGuiImageParams::setImage       = nullptr;
CSWGuiImageParams::SetImageObjectFn CSWGuiImageParams::setImageObject = nullptr;
CSWGuiImageParams::SetAlignmentFn   CSWGuiImageParams::setAlignment   = nullptr;
CSWGuiImageParams::SetDrawStyleFn   CSWGuiImageParams::setDrawStyle   = nullptr;

bool CSWGuiImageParams::functionsInitialized = false;
bool CSWGuiImageParams::offsetsInitialized = false;

int CSWGuiImageParams::offsetResRef      = -1;
int CSWGuiImageParams::offsetAngle       = -1;
int CSWGuiImageParams::offsetAlpha       = -1;
int CSWGuiImageParams::offsetColor       = -1;
int CSWGuiImageParams::offsetImageObject = -1;

void CSWGuiImageParams::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiImageParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        setImage       = reinterpret_cast<SetImageFn>      (GameVersion::GetFunctionAddress("CSWGuiImageParams", "SetImage"));
        setImageObject = reinterpret_cast<SetImageObjectFn>(GameVersion::GetFunctionAddress("CSWGuiImageParams", "SetImageObject"));
        setAlignment   = reinterpret_cast<SetAlignmentFn>  (GameVersion::GetFunctionAddress("CSWGuiImageParams", "SetAlignment"));
        setDrawStyle   = reinterpret_cast<SetDrawStyleFn>  (GameVersion::GetFunctionAddress("CSWGuiImageParams", "SetDrawStyle"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiImageParams] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiImageParams::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiImageParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetResRef      = GameVersion::GetOffset("CSWGuiImageParams", "resref");
        offsetAngle       = GameVersion::GetOffset("CSWGuiImageParams", "angle");
        offsetAlpha       = GameVersion::GetOffset("CSWGuiImageParams", "alpha");
        offsetColor       = GameVersion::GetOffset("CSWGuiImageParams", "color");
        offsetImageObject = GameVersion::GetOffset("CSWGuiImageParams", "image_object");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiImageParams] ERROR: %s\n", e.what());
    }
}

CSWGuiImageParams::CSWGuiImageParams(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    InitializeFunctions();
    InitializeOffsets();
}

CSWGuiImageParams::~CSWGuiImageParams()
{
    // Base class destructor handles objectPtr cleanup
}

float CSWGuiImageParams::GetAngle() {
    if (!objectPtr || offsetAngle < 0) {
        return 0.0f;
    }
    return getObjectProperty<float>(objectPtr, offsetAngle);
}

void CSWGuiImageParams::SetAngle(float angle) {
    if (!objectPtr || offsetAngle < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetAngle, angle);
}

float CSWGuiImageParams::GetAlpha() {
    if (!objectPtr || offsetAlpha < 0) {
        return 0.0f;
    }
    return getObjectProperty<float>(objectPtr, offsetAlpha);
}

void CSWGuiImageParams::SetAlpha(float alpha) {
    if (!objectPtr || offsetAlpha < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetAlpha, alpha);
}

Vector CSWGuiImageParams::GetColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetColor);
}

void CSWGuiImageParams::SetColor(const Vector& color) {
    if (!objectPtr || offsetColor < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetColor, color);
}

CResRef* CSWGuiImageParams::GetResRef() {
    if (!objectPtr || offsetResRef < 0) {
        return nullptr;
    }
    // Inline CResRef member: wrap its in-place address.
    return new CResRef(static_cast<void*>((char*)objectPtr + offsetResRef));
}

CSWGuiImage* CSWGuiImageParams::GetImageObject() {
    if (!objectPtr || offsetImageObject < 0) {
        return nullptr;
    }
    void* imagePtr = getObjectProperty<void*>(objectPtr, offsetImageObject);
    if (!imagePtr) {
        return nullptr;
    }
    return new CSWGuiImage(imagePtr);
}

void CSWGuiImageParams::SetImage(CResRef* image, int forceUpdate) {
    if (!objectPtr || !setImage) return;
    setImage(objectPtr, image ? image->GetPtr() : nullptr, forceUpdate);
}

void CSWGuiImageParams::SetImageObject(CSWGuiImage* image) {
    if (!objectPtr || !setImageObject) return;
    setImageObject(objectPtr, image ? image->GetPtr() : nullptr);
}

void CSWGuiImageParams::SetAlignment(int alignment) {
    if (!objectPtr || !setAlignment) return;
    setAlignment(objectPtr, alignment);
}

void CSWGuiImageParams::SetDrawStyle(int drawStyle) {
    if (!objectPtr || !setDrawStyle) return;
    setDrawStyle(objectPtr, drawStyle);
}
