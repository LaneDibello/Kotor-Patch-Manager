#include "CSWGuiNavigable.h"
#include "GameVersion.h"

bool CSWGuiNavigable::functionsInitialized = false;
bool CSWGuiNavigable::offsetsInitialized = false;
CSWGuiNavigable::ConstructorFn CSWGuiNavigable::constructor = nullptr;
CSWGuiNavigable::DestructorFn  CSWGuiNavigable::destructor  = nullptr;
int CSWGuiNavigable::classSize = -1;

void CSWGuiNavigable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiNavigable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiNavigable", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiNavigable", "Destructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiNavigable] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiNavigable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiNavigable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiNavigable");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiNavigable] ERROR: %s\n", e.what());
    }
}

CSWGuiNavigable::CSWGuiNavigable(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiNavigable::CSWGuiNavigable()
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

CSWGuiNavigable::~CSWGuiNavigable()
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
