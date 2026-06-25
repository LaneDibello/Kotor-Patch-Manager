#include "CSWGuiNavigable.h"
#include "GameVersion.h"

bool CSWGuiNavigable::functionsInitialized = false;
bool CSWGuiNavigable::offsetsInitialized = false;
CSWGuiNavigable::ConstructorFn CSWGuiNavigable::constructor = nullptr;
CSWGuiNavigable::DestructorFn  CSWGuiNavigable::destructor  = nullptr;
CSWGuiNavigable::GetMoveToControlFn CSWGuiNavigable::getMoveToControl = nullptr;
CSWGuiNavigable::SetMoveToControlFn CSWGuiNavigable::setMoveToControl = nullptr;
int CSWGuiNavigable::classSize = -1;
int CSWGuiNavigable::offsetUp    = -1;
int CSWGuiNavigable::offsetLeft  = -1;
int CSWGuiNavigable::offsetDown  = -1;
int CSWGuiNavigable::offsetRight = -1;

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
        getMoveToControl = reinterpret_cast<GetMoveToControlFn>(GameVersion::GetFunctionAddress("CSWGuiNavigable", "GetMoveToControl"));
        setMoveToControl = reinterpret_cast<SetMoveToControlFn>(GameVersion::GetFunctionAddress("CSWGuiNavigable", "SetMoveToControl"));

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
        offsetUp    = GameVersion::GetOffset("CSWGuiNavigable", "up");
        offsetLeft  = GameVersion::GetOffset("CSWGuiNavigable", "left");
        offsetDown  = GameVersion::GetOffset("CSWGuiNavigable", "down");
        offsetRight = GameVersion::GetOffset("CSWGuiNavigable", "right");
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

int CSWGuiNavigable::offsetForDirection(CSWGuiNavDirection direction) {
    switch (direction) {
        case NavUp:    return offsetUp;
        case NavLeft:  return offsetLeft;
        case NavDown:  return offsetDown;
        case NavRight: return offsetRight;
        default:       return -1;
    }
}

int CSWGuiNavigable::GetMoveToControl(CSWGuiNavDirection direction) {
    if (!objectPtr || !getMoveToControl) return -1;
    return getMoveToControl(objectPtr, static_cast<int>(direction));
}

void CSWGuiNavigable::SetMoveToControl(CSWGuiNavDirection direction, int controlIndex) {
    if (!objectPtr || !setMoveToControl) return;
    setMoveToControl(objectPtr, static_cast<int>(direction), controlIndex);
}

int CSWGuiNavigable::GetMoveToControlIndex(CSWGuiNavDirection direction) {
    int off = offsetForDirection(direction);
    if (!objectPtr || off < 0) return -1;
    return getObjectProperty<int>(objectPtr, off);
}

void CSWGuiNavigable::SetMoveToControlIndex(CSWGuiNavDirection direction, int controlIndex) {
    int off = offsetForDirection(direction);
    if (!objectPtr || off < 0) return;
    setObjectProperty<int>(objectPtr, off, controlIndex);
}
