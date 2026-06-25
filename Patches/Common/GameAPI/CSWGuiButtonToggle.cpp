#include "CSWGuiButtonToggle.h"
#include "GameVersion.h"

CSWGuiButtonToggle::SetSelectedFn CSWGuiButtonToggle::setSelected = nullptr;
CSWGuiButtonToggle::ConstructorFn CSWGuiButtonToggle::constructor = nullptr;
CSWGuiButtonToggle::DestructorFn  CSWGuiButtonToggle::destructor  = nullptr;
int CSWGuiButtonToggle::classSize = -1;

bool CSWGuiButtonToggle::functionsInitialized = false;
bool CSWGuiButtonToggle::offsetsInitialized = false;

void CSWGuiButtonToggle::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiButton::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButtonToggle] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        setSelected = reinterpret_cast<SetSelectedFn>(GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "SetSelected"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiButtonToggle", "Destructor_2"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButtonToggle] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiButtonToggle::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiButton::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiButtonToggle] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiButtonToggle");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiButtonToggle] ERROR: %s\n", e.what());
    }
}

CSWGuiButtonToggle::CSWGuiButtonToggle(void* objectPtr)
    : CSWGuiButton(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiButtonToggle::CSWGuiButtonToggle()
    : CSWGuiButton(nullptr)
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

CSWGuiButtonToggle::~CSWGuiButtonToggle()
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

void CSWGuiButtonToggle::SetSelected(UINT selected) {
    if (!objectPtr || !setSelected) return;
    setSelected(objectPtr, selected);
}
