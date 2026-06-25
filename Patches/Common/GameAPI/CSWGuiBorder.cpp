#include "CSWGuiBorder.h"
#include "GameVersion.h"

CSWGuiBorder::FillCenterFn CSWGuiBorder::fillCenter = nullptr;
CSWGuiBorder::FillTileFn   CSWGuiBorder::fillTile   = nullptr;
CSWGuiBorder::ConstructorFn CSWGuiBorder::constructor = nullptr;
CSWGuiBorder::DestructorFn  CSWGuiBorder::destructor  = nullptr;
int CSWGuiBorder::classSize = -1;

bool CSWGuiBorder::functionsInitialized = false;
bool CSWGuiBorder::offsetsInitialized = false;

void CSWGuiBorder::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorder] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        fillCenter = reinterpret_cast<FillCenterFn>(GameVersion::GetFunctionAddress("CSWGuiBorder", "FillCenter"));
        fillTile   = reinterpret_cast<FillTileFn>  (GameVersion::GetFunctionAddress("CSWGuiBorder", "FillTile"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiBorder", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiBorder", "Destructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorder] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiBorder::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiBorder] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiBorder");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiBorder] ERROR: %s\n", e.what());
    }
}

CSWGuiBorder::CSWGuiBorder(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiBorder::CSWGuiBorder()
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

CSWGuiBorder::~CSWGuiBorder()
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

void CSWGuiBorder::FillCenter(int height, int width, int x, int y, float alpha, Vector* color) {
    if (!objectPtr || !fillCenter) return;
    fillCenter(objectPtr, height, width, x, y, alpha, color);
}

void CSWGuiBorder::FillTile(int height, int width, int x, int y, float alpha, Vector* color) {
    if (!objectPtr || !fillTile) return;
    fillTile(objectPtr, height, width, x, y, alpha, color);
}
