#include "CSWSObject.h"
#include "GameVersion.h"
#include "../Common.h"

CSWSObject::AddActionToFrontFn CSWSObject::addActionToFront = nullptr;
bool CSWSObject::functionsInitialized = false;

void CSWSObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

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

CSWSObject::CSWSObject(void* objectPtr)
    : objectPtr(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

CSWSObject::~CSWSObject() {
    objectPtr = nullptr;
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
