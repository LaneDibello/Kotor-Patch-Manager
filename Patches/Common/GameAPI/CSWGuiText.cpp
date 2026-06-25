#include "CSWGuiText.h"
#include "GameVersion.h"

bool CSWGuiText::functionsInitialized = false;
bool CSWGuiText::offsetsInitialized = false;
CSWGuiText::ConstructorFn CSWGuiText::constructor = nullptr;
CSWGuiText::DestructorFn  CSWGuiText::destructor  = nullptr;
int CSWGuiText::classSize = -1;

void CSWGuiText::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiText", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiText", "Destructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiText] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiText::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiText] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        classSize = GameVersion::GetClassSize("CSWGuiText");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiText] ERROR: %s\n", e.what());
    }
}

CSWGuiText::CSWGuiText(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiText::CSWGuiText()
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

CSWGuiText::~CSWGuiText()
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
