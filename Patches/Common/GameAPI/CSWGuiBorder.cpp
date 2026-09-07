#include "CSWGuiBorder.h"
#include "CSWGuiBorderParams.h"
#include "GameVersion.h"
#include "CResGFF.h"
#include "CExoString.h"

CSWGuiBorder::FillCenterFn CSWGuiBorder::fillCenter = nullptr;
CSWGuiBorder::FillTileFn   CSWGuiBorder::fillTile   = nullptr;
CSWGuiBorder::InitializeFn     CSWGuiBorder::initialize     = nullptr;
CSWGuiBorder::GetInnerExtentFn CSWGuiBorder::getInnerExtent = nullptr;
CSWGuiBorder::DrawFn           CSWGuiBorder::drawFn         = nullptr;
CSWGuiBorder::LoadFn CSWGuiBorder::loadFn = nullptr;
CSWGuiBorder::ConstructorFn CSWGuiBorder::constructor = nullptr;
CSWGuiBorder::DestructorFn  CSWGuiBorder::destructor  = nullptr;
int CSWGuiBorder::classSize = -1;
int CSWGuiBorder::offsetBorderParams = -1;

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
        initialize     = reinterpret_cast<InitializeFn>    (GameVersion::GetFunctionAddress("CSWGuiBorder", "Initialize"));
        getInnerExtent = reinterpret_cast<GetInnerExtentFn>(GameVersion::GetFunctionAddress("CSWGuiBorder", "GetInnerExtent"));
        drawFn         = reinterpret_cast<DrawFn>          (GameVersion::GetFunctionAddress("CSWGuiBorder", "Draw"));
        loadFn = reinterpret_cast<LoadFn>(GameVersion::GetFunctionAddress("CSWGuiBorder", "Load"));
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
        offsetBorderParams = GameVersion::GetOffset("CSWGuiBorder", "border_params");
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

void CSWGuiBorder::FillCenter(int height, int width, int x, int y, float alpha, Vector* color) {
    if (!objectPtr || !fillCenter) return;
    fillCenter(objectPtr, height, width, x, y, alpha, color);
}

void CSWGuiBorder::FillTile(int height, int width, int x, int y, float alpha, Vector* color) {
    if (!objectPtr || !fillTile) return;
    fillTile(objectPtr, height, width, x, y, alpha, color);
}

CSWGuiBorderParams* CSWGuiBorder::GetBorderParams() {
    if (!objectPtr || offsetBorderParams < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorderParams member: wrap its in-place address.
    return new CSWGuiBorderParams((char*)objectPtr + offsetBorderParams);
}

void CSWGuiBorder::Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams) {
    if (!objectPtr || !initialize) return;
    initialize(objectPtr, extent, borderParams ? borderParams->GetPtr() : nullptr);
}

void CSWGuiBorder::GetInnerExtent(CSWGuiExtent* outExtent) {
    if (!objectPtr || !getInnerExtent || !outExtent) return;
    getInnerExtent(objectPtr, outExtent);
}

void CSWGuiBorder::Draw(float alpha) {
    if (!objectPtr || !drawFn) return;
    drawFn(objectPtr, alpha);
}

void CSWGuiBorder::Load(CResGFF* gff, CResStruct* item, CExoString* label) {
    if (!objectPtr || !loadFn) return;
    loadFn(objectPtr, gff ? gff->GetPtr() : nullptr, item, label ? label->GetPtr() : nullptr);
}
