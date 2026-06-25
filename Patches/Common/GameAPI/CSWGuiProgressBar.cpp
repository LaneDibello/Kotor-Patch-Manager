#include "CSWGuiProgressBar.h"
#include "GameVersion.h"

CSWGuiProgressBar::ConstructorFn CSWGuiProgressBar::constructor = nullptr;
CSWGuiProgressBar::DestructorFn  CSWGuiProgressBar::destructor  = nullptr;
int CSWGuiProgressBar::classSize = -1;

bool CSWGuiProgressBar::functionsInitialized = false;
bool CSWGuiProgressBar::offsetsInitialized = false;

void CSWGuiProgressBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiProgressBar", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiProgressBar", "Destructor_2"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiProgressBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiProgressBar");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
    }
}

CSWGuiProgressBar::CSWGuiProgressBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiProgressBar::CSWGuiProgressBar()
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

CSWGuiProgressBar::~CSWGuiProgressBar()
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
