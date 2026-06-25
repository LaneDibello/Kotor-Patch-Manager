#include "CSWGuiImage.h"
#include "GameVersion.h"

CSWGuiImage::GetImageExtentFn CSWGuiImage::getImageExtent = nullptr;

bool CSWGuiImage::functionsInitialized = false;
bool CSWGuiImage::offsetsInitialized = false;

void CSWGuiImage::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiImage] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getImageExtent = reinterpret_cast<GetImageExtentFn>(GameVersion::GetFunctionAddress("CSWGuiImage", "GetImageExtent"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiImage] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiImage::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiImage] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiImage] ERROR: %s\n", e.what());
    }
}

CSWGuiImage::CSWGuiImage(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiImage::~CSWGuiImage()
{
    // Base class destructor handles objectPtr cleanup
}

void CSWGuiImage::GetImageExtent(CSWGuiExtent* outExtent) {
    if (!objectPtr || !getImageExtent || !outExtent) return;
    getImageExtent(objectPtr, outExtent);
}
