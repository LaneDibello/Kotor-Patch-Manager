#include "CSWGuiBorderParams.h"
#include "CResRef.h"
#include "CSWGuiBorder.h"
#include "GameVersion.h"

CSWGuiBorderParams::SetBorderFn      CSWGuiBorderParams::setBorder      = nullptr;
CSWGuiBorderParams::SetCornerImageFn CSWGuiBorderParams::setCornerImage = nullptr;
CSWGuiBorderParams::SetEdgeImageFn   CSWGuiBorderParams::setEdgeImage   = nullptr;
CSWGuiBorderParams::SetFillImageFn   CSWGuiBorderParams::setFillImage   = nullptr;
CSWGuiBorderParams::AssignFn         CSWGuiBorderParams::assign         = nullptr;

bool CSWGuiBorderParams::functionsInitialized = false;
bool CSWGuiBorderParams::offsetsInitialized = false;

int CSWGuiBorderParams::offsetExtent            = -1;
int CSWGuiBorderParams::offsetColor             = -1;
int CSWGuiBorderParams::offsetCornerImageResRef = -1;
int CSWGuiBorderParams::offsetEdgeImageResRef   = -1;
int CSWGuiBorderParams::offsetFillImageResRef   = -1;
int CSWGuiBorderParams::offsetBorder            = -1;

void CSWGuiBorderParams::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorderParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        setBorder      = reinterpret_cast<SetBorderFn>     (GameVersion::GetFunctionAddress("CSWGuiBorderParams", "SetBorder"));
        setCornerImage = reinterpret_cast<SetCornerImageFn>(GameVersion::GetFunctionAddress("CSWGuiBorderParams", "SetCornerImage"));
        setEdgeImage   = reinterpret_cast<SetEdgeImageFn>  (GameVersion::GetFunctionAddress("CSWGuiBorderParams", "SetEdgeImage"));
        setFillImage   = reinterpret_cast<SetFillImageFn>  (GameVersion::GetFunctionAddress("CSWGuiBorderParams", "SetFillImage"));
        assign         = reinterpret_cast<AssignFn>        (GameVersion::GetFunctionAddress("CSWGuiBorderParams", "operator="));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorderParams] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiBorderParams::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorderParams] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetExtent            = GameVersion::GetOffset("CSWGuiBorderParams", "extent");
        offsetColor             = GameVersion::GetOffset("CSWGuiBorderParams", "color");
        offsetCornerImageResRef = GameVersion::GetOffset("CSWGuiBorderParams", "corner_image_resref");
        offsetEdgeImageResRef   = GameVersion::GetOffset("CSWGuiBorderParams", "edge_image_resref");
        offsetFillImageResRef   = GameVersion::GetOffset("CSWGuiBorderParams", "fill_image_resref");
        offsetBorder            = GameVersion::GetOffset("CSWGuiBorderParams", "border");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorderParams] ERROR: %s\n", e.what());
    }
}

CSWGuiBorderParams::CSWGuiBorderParams(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    InitializeFunctions();
    InitializeOffsets();
}

CSWGuiBorderParams::~CSWGuiBorderParams()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiExtent CSWGuiBorderParams::GetExtent() {
    CSWGuiExtent result = {0, 0, 0, 0};
    if (!objectPtr || offsetExtent < 0) {
        return result;
    }
    return getObjectProperty<CSWGuiExtent>(objectPtr, offsetExtent);
}

void CSWGuiBorderParams::SetExtent(const CSWGuiExtent& extent) {
    if (!objectPtr || offsetExtent < 0) {
        return;
    }
    setObjectProperty<CSWGuiExtent>(objectPtr, offsetExtent, extent);
}

Vector CSWGuiBorderParams::GetColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetColor);
}

void CSWGuiBorderParams::SetColor(const Vector& color) {
    if (!objectPtr || offsetColor < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetColor, color);
}

CResRef* CSWGuiBorderParams::GetCornerImageResRef() {
    if (!objectPtr || offsetCornerImageResRef < 0) {
        return nullptr;
    }
    // Inline CResRef member: wrap its in-place address.
    return new CResRef(static_cast<void*>((char*)objectPtr + offsetCornerImageResRef));
}

CResRef* CSWGuiBorderParams::GetEdgeImageResRef() {
    if (!objectPtr || offsetEdgeImageResRef < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<void*>((char*)objectPtr + offsetEdgeImageResRef));
}

CResRef* CSWGuiBorderParams::GetFillImageResRef() {
    if (!objectPtr || offsetFillImageResRef < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<void*>((char*)objectPtr + offsetFillImageResRef));
}

CSWGuiBorder* CSWGuiBorderParams::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    void* borderPtr = getObjectProperty<void*>(objectPtr, offsetBorder);
    if (!borderPtr) {
        return nullptr;
    }
    return new CSWGuiBorder(borderPtr);
}

void CSWGuiBorderParams::SetBorder(CSWGuiBorder* border) {
    if (!objectPtr || !setBorder) return;
    setBorder(objectPtr, border ? border->GetPtr() : nullptr);
}

void CSWGuiBorderParams::SetCornerImage(CResRef* image, int forceUpdate) {
    if (!objectPtr || !setCornerImage) return;
    setCornerImage(objectPtr, image ? image->GetPtr() : nullptr, forceUpdate);
}

void CSWGuiBorderParams::SetEdgeImage(CResRef* image, int forceUpdate) {
    if (!objectPtr || !setEdgeImage) return;
    setEdgeImage(objectPtr, image ? image->GetPtr() : nullptr, forceUpdate);
}

void CSWGuiBorderParams::SetFillImage(CResRef* image, int forceUpdate) {
    if (!objectPtr || !setFillImage) return;
    setFillImage(objectPtr, image ? image->GetPtr() : nullptr, forceUpdate);
}

CSWGuiBorderParams& CSWGuiBorderParams::operator=(const CSWGuiBorderParams& rhs) {
    if (this == &rhs) {
        return *this;
    }
    if (objectPtr && assign) {
        assign(objectPtr, rhs.GetPtr());
    }
    return *this;
}
