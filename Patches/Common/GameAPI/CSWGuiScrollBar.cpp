#include "CSWGuiScrollBar.h"
#include "GameVersion.h"

bool CSWGuiScrollBar::functionsInitialized = false;
bool CSWGuiScrollBar::offsetsInitialized = false;
CSWGuiScrollBar::ConstructorFn CSWGuiScrollBar::constructor = nullptr;
CSWGuiScrollBar::DestructorFn  CSWGuiScrollBar::destructor  = nullptr;
int CSWGuiScrollBar::classSize = -1;

void CSWGuiScrollBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiScrollBar", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiScrollBar", "Destructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiScrollBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScrollBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiScrollBar");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScrollBar] ERROR: %s\n", e.what());
    }
}

CSWGuiScrollBar::CSWGuiScrollBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiScrollBar::CSWGuiScrollBar()
    : CSWGuiControl(nullptr)
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

CSWGuiScrollBar::~CSWGuiScrollBar()
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
