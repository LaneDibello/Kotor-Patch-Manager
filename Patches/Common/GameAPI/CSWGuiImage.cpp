#include "CSWGuiImage.h"
#include "CSWGuiImageParams.h"
#include "GameVersion.h"

CSWGuiImage::GetImageExtentFn CSWGuiImage::getImageExtent = nullptr;
CSWGuiImage::ConstructorFn CSWGuiImage::constructor = nullptr;
CSWGuiImage::DestructorFn  CSWGuiImage::destructor  = nullptr;
int CSWGuiImage::classSize = -1;
int CSWGuiImage::offsetParams = -1;

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
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiImage", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiImage", "Destructor"));

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
        offsetParams = GameVersion::GetOffset("CSWGuiImage", "params");
        classSize = GameVersion::GetClassSize("CSWGuiImage");

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

CSWGuiImage::CSWGuiImage()
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

CSWGuiImage::~CSWGuiImage()
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

void CSWGuiImage::GetImageExtent(CSWGuiExtent* outExtent) {
    if (!objectPtr || !getImageExtent || !outExtent) return;
    getImageExtent(objectPtr, outExtent);
}

CSWGuiImageParams* CSWGuiImage::GetParams() {
    if (!objectPtr || offsetParams < 0) {
        return nullptr;
    }
    // Inline CSWGuiImageParams member: wrap its in-place address.
    return new CSWGuiImageParams((char*)objectPtr + offsetParams);
}
