#include "CVirtualMachine.h"

bool CVirtualMachine::functionsInitialized = false;
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

CVirtualMachine::CVirtualMachine(void* vmPtr)
    : vmPtr(vmPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
}

CVirtualMachine::~CVirtualMachine() {
    vmPtr = nullptr;
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

CVirtualMachine* CVirtualMachine::GetInstance() {
    if (!functionsInitialized) {
        InitializeFunctions();
    }

    void** vmPtrAddr = static_cast<void**>(GameVersion::GetGlobalPointer("VIRTUAL_MACHINE_PTR"));
    if (!vmPtrAddr || !*vmPtrAddr) {
        OutputDebugStringA("[CVirtualMachine] ERROR: VIRTUAL_MACHINE_PTR is null\n");
        return nullptr;
    }

    return new CVirtualMachine(*vmPtrAddr);
}

bool CVirtualMachine::StackPopInteger(int* output) {
    if (!stackPopInt || !vmPtr) {
        return false;
    }
    return stackPopInt(vmPtr, output) != 0;
}

bool CVirtualMachine::StackPopFloat(float* output) {
    if (!stackPopFloat || !vmPtr) {
        return false;
    }
    return stackPopFloat(vmPtr, output) != 0;
}

bool CVirtualMachine::StackPopVector(Vector* output) {
    if (!stackPopVector || !vmPtr) {
        return false;
    }
    return stackPopVector(vmPtr, output) != 0;
}

bool CVirtualMachine::StackPopString(CExoString* output) {
    if (!stackPopString || !vmPtr) {
        return false;
    }

    return stackPopString(vmPtr, output->GetPtr()) != 0;
}

bool CVirtualMachine::StackPopEngineStructure(VirtualMachineEngineStructureTypes type, void** output) {
    if (!stackPopEngineStructure || !vmPtr) {
        return false;
    }
    return stackPopEngineStructure(vmPtr, type, output) != 0;
}

bool CVirtualMachine::StackPopObject(DWORD* output) {
    if (!stackPopObject || !vmPtr) {
        return false;
    }
    return stackPopObject(vmPtr, output) != 0;
}

bool CVirtualMachine::StackPopCommand(void** output) {
    if (!stackPopCommand || !vmPtr) {
        return false;
    }
    return stackPopCommand(vmPtr, output) != 0;
}

bool CVirtualMachine::StackPushInteger(int value) {
    if (!stackPushInt || !vmPtr) {
        return false;
    }
    return stackPushInt(vmPtr, value) != 0;
}

bool CVirtualMachine::StackPushFloat(float value) {
    if (!stackPushFloat || !vmPtr) {
        return false;
    }
    return stackPushFloat(vmPtr, value) != 0;
}

bool CVirtualMachine::StackPushVector(Vector value) {
    if (!stackPushVector || !vmPtr) {
        return false;
    }
    return stackPushVector(vmPtr, value) != 0;
}

bool CVirtualMachine::StackPushString(CExoString* value) {
    if (!stackPushString || !vmPtr) {
        return false;
    }
    return stackPushString(vmPtr, value->GetPtr()) != 0;
}

bool CVirtualMachine::StackPushEngineStructure(VirtualMachineEngineStructureTypes type, void* value) {
    if (!stackPushEngineStructure || !vmPtr) {
        return false;
    }
    return stackPushEngineStructure(vmPtr, type, value) != 0;
}

bool CVirtualMachine::StackPushObject(DWORD value) {
    if (!stackPushObject || !vmPtr) {
        return false;
    }
    return stackPushObject(vmPtr, value) != 0;
}

bool CVirtualMachine::RunScript(CExoString* scriptName, DWORD objectSelf, int usually1) {
    if (!runScript || !vmPtr) {
        return false;
    }
    return runScript(vmPtr, scriptName->GetPtr(), objectSelf, usually1) != 0;
}
