#include "CSWSObject.h"
#include "GameVersion.h"
#include "../Common.h"

CSWSObject::AddActionToFrontFn CSWSObject::addActionToFront = nullptr;
bool CSWSObject::functionsInitialized = false;

int CSWSObject::offsetPosition = -1;
int CSWSObject::offsetOrientation = -1;
int CSWSObject::offsetAreaId = -1;
bool CSWSObject::offsetsInitialized = false;

void CSWSObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    // Call base class initialization first
    CGameObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addActionToFront = reinterpret_cast<AddActionToFrontFn>(
            GameVersion::GetFunctionAddress("CSWSObject", "AddActionToFront")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSObject] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    // Call base class offset initialization
    CGameObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetPosition = GameVersion::GetOffset("CSWSObject", "Position");
        offsetOrientation = GameVersion::GetOffset("CSWSObject", "Orientation");
        offsetAreaId = GameVersion::GetOffset("CSWSObject", "AreaId");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSObject] ERROR: %s\n", e.what());
    }
}

CSWSObject::CSWSObject(void* objectPtr)
    : CGameObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSObject::~CSWSObject() {
    // Base class destructor will handle objectPtr cleanup
}

void CSWSObject::AddActionToFront(
    DWORD param_1, USHORT param_2, DWORD param_3, void* param_4, DWORD param_5,
    void* param_6, DWORD param_7, void* param_8, DWORD param_9, void* param_10, DWORD param_11,
    void* param_12, DWORD param_13, void* param_14, DWORD param_15, void* param_16, DWORD param_17,
    void* param_18, DWORD param_19, void* param_20, DWORD param_21, void* param_22, DWORD param_23,
    void* param_24, DWORD param_25, void* param_26, DWORD param_27, void* param_28)
{
    if (!objectPtr || !addActionToFront) {
        return;
    }
    debugLog("[CSWSObject::AddActionToFront] objectPtr is %p");

    addActionToFront(objectPtr, param_1, param_2, param_3, param_4, param_5,
        param_6, param_7, param_8, param_9, param_10, param_11,
        param_12, param_13, param_14, param_15, param_16, param_17,
        param_18, param_19, param_20, param_21, param_22, param_23,
        param_24, param_25, param_26, param_27, param_28);
}

Vector CSWSObject::GetPosition() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!objectPtr || offsetPosition < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetPosition);
}

Vector CSWSObject::GetOrientation() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!objectPtr || offsetOrientation < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetOrientation);
}

DWORD CSWSObject::GetAreaId() {
    if (!objectPtr || offsetAreaId < 0) {
        return 0x7F000000;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetAreaId);
}

void CSWSObject::SetPosition(const Vector& position) {
    if (!objectPtr || offsetPosition < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetPosition, position);
}

void CSWSObject::SetOrientation(const Vector& orientation) {
    if (!objectPtr || offsetOrientation < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetOrientation, orientation);
}

void CSWSObject::SetAreaId(DWORD areaId) {
    if (!objectPtr || offsetAreaId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetAreaId, areaId);
}
