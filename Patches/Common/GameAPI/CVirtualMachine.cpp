#include "CVirtualMachine.h"

bool CVirtualMachine::functionsInitialized = false;
bool CVirtualMachine::offsetsInitialized = false;
CVirtualMachine::StackPopIntFn CVirtualMachine::stackPopInt = nullptr;
CVirtualMachine::StackPopFloatFn CVirtualMachine::stackPopFloat = nullptr;
CVirtualMachine::StackPopVectorFn CVirtualMachine::stackPopVector = nullptr;
CVirtualMachine::StackPopStringFn CVirtualMachine::stackPopString = nullptr;
CVirtualMachine::StackPopEngineStructureFn CVirtualMachine::stackPopEngineStructure = nullptr;
CVirtualMachine::StackPopObjectFn CVirtualMachine::stackPopObject = nullptr;
CVirtualMachine::StackPopCommandFn CVirtualMachine::stackPopCommand = nullptr;
CVirtualMachine::StackPushIntFn CVirtualMachine::stackPushInt = nullptr;
CVirtualMachine::StackPushFloatFn CVirtualMachine::stackPushFloat = nullptr;
CVirtualMachine::StackPushVectorFn CVirtualMachine::stackPushVector = nullptr;
CVirtualMachine::StackPushStringFn CVirtualMachine::stackPushString = nullptr;
CVirtualMachine::StackPushEngineStructureFn CVirtualMachine::stackPushEngineStructure = nullptr;
CVirtualMachine::StackPushObjectFn CVirtualMachine::stackPushObject = nullptr;
CVirtualMachine::RunScriptFn CVirtualMachine::runScript = nullptr;

CVirtualMachine::CVirtualMachine(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CVirtualMachine::~CVirtualMachine() {
    // Base class destructor handles objectPtr cleanup
}

void CVirtualMachine::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CVirtualMachine] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        stackPopInt = reinterpret_cast<StackPopIntFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopInteger"));
        stackPopFloat = reinterpret_cast<StackPopFloatFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopFloat"));
        stackPopVector = reinterpret_cast<StackPopVectorFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopVector"));
        stackPopString = reinterpret_cast<StackPopStringFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopString"));
        stackPopEngineStructure = reinterpret_cast<StackPopEngineStructureFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopEngineStructure"));
        stackPopObject = reinterpret_cast<StackPopObjectFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopObject"));
        stackPopCommand = reinterpret_cast<StackPopCommandFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopCommand"));
        stackPushInt = reinterpret_cast<StackPushIntFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushInteger"));
        stackPushFloat = reinterpret_cast<StackPushFloatFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushFloat"));
        stackPushVector = reinterpret_cast<StackPushVectorFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushVector"));
        stackPushString = reinterpret_cast<StackPushStringFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushString"));
        stackPushEngineStructure = reinterpret_cast<StackPushEngineStructureFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushEngineStructure"));
        stackPushObject = reinterpret_cast<StackPushObjectFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushObject"));
        runScript = reinterpret_cast<RunScriptFn>(GameVersion::GetFunctionAddress("CVirtualMachine", "RunScript"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CVirtualMachine] WARNING: %s\n", e.what());
    }

    functionsInitialized = true;
}

void CVirtualMachine::InitializeOffsets() {
    // CVirtualMachine has no offsets
    offsetsInitialized = true;
}

CVirtualMachine* CVirtualMachine::GetInstance() {
    void** objectPtrAddr = static_cast<void**>(GameVersion::GetGlobalPointer("VIRTUAL_MACHINE_PTR"));
    if (!objectPtrAddr || !*objectPtrAddr) {
        OutputDebugStringA("[CVirtualMachine] ERROR: VIRTUAL_MACHINE_PTR is null\n");
        return nullptr;
    }

    // Initialization will happen in constructor
    return new CVirtualMachine(*objectPtrAddr);
}

bool CVirtualMachine::StackPopInteger(int* output) {
    if (!stackPopInt || !objectPtr) {
        return false;
    }
    return stackPopInt(objectPtr, output) != 0;
}

bool CVirtualMachine::StackPopFloat(float* output) {
    if (!stackPopFloat || !objectPtr) {
        return false;
    }
    return stackPopFloat(objectPtr, output) != 0;
}

bool CVirtualMachine::StackPopVector(Vector* output) {
    if (!stackPopVector || !objectPtr) {
        return false;
    }
    return stackPopVector(objectPtr, output) != 0;
}

bool CVirtualMachine::StackPopString(CExoString* output) {
    if (!stackPopString || !objectPtr) {
        return false;
    }

    return stackPopString(objectPtr, output->GetPtr()) != 0;
}

bool CVirtualMachine::StackPopEngineStructure(VirtualMachineEngineStructureTypes type, void** output) {
    if (!stackPopEngineStructure || !objectPtr) {
        return false;
    }
    return stackPopEngineStructure(objectPtr, type, output) != 0;
}

bool CVirtualMachine::StackPopObject(DWORD* output) {
    if (!stackPopObject || !objectPtr) {
        return false;
    }
    return stackPopObject(objectPtr, output) != 0;
}

bool CVirtualMachine::StackPopCommand(void** output) {
    if (!stackPopCommand || !objectPtr) {
        return false;
    }
    return stackPopCommand(objectPtr, output) != 0;
}

bool CVirtualMachine::StackPushInteger(int value) {
    if (!stackPushInt || !objectPtr) {
        return false;
    }
    return stackPushInt(objectPtr, value) != 0;
}

bool CVirtualMachine::StackPushFloat(float value) {
    if (!stackPushFloat || !objectPtr) {
        return false;
    }
    return stackPushFloat(objectPtr, value) != 0;
}

bool CVirtualMachine::StackPushVector(Vector value) {
    if (!stackPushVector || !objectPtr) {
        return false;
    }
    return stackPushVector(objectPtr, value) != 0;
}

bool CVirtualMachine::StackPushString(CExoString* value) {
    if (!stackPushString || !objectPtr) {
        return false;
    }
    return stackPushString(objectPtr, value->GetPtr()) != 0;
}

bool CVirtualMachine::StackPushEngineStructure(VirtualMachineEngineStructureTypes type, void* value) {
    if (!stackPushEngineStructure || !objectPtr) {
        return false;
    }
    return stackPushEngineStructure(objectPtr, type, value) != 0;
}

bool CVirtualMachine::StackPushObject(DWORD value) {
    if (!stackPushObject || !objectPtr) {
        return false;
    }
    return stackPushObject(objectPtr, value) != 0;
}

bool CVirtualMachine::RunScript(CExoString* scriptName, DWORD objectSelf, int usually1) {
    if (!runScript || !objectPtr) {
        return false;
    }
    return runScript(objectPtr, scriptName->GetPtr(), objectSelf, usually1) != 0;
}
