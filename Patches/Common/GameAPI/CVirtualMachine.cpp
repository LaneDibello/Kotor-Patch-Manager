#include "CVirtualMachine.h"

// Static member initialization
bool CVirtualMachine::m_functionsInitialized = false;
CVirtualMachine::StackPopIntFn CVirtualMachine::m_stackPopInt = nullptr;
CVirtualMachine::StackPopFloatFn CVirtualMachine::m_stackPopFloat = nullptr;
CVirtualMachine::StackPopVectorFn CVirtualMachine::m_stackPopVector = nullptr;
CVirtualMachine::StackPopStringFn CVirtualMachine::m_stackPopString = nullptr;
CVirtualMachine::StackPopEngineStructureFn CVirtualMachine::m_stackPopEngineStructure = nullptr;
CVirtualMachine::StackPopObjectFn CVirtualMachine::m_stackPopObject = nullptr;
CVirtualMachine::StackPopCommandFn CVirtualMachine::m_stackPopCommand = nullptr;
CVirtualMachine::StackPushIntFn CVirtualMachine::m_stackPushInt = nullptr;
CVirtualMachine::StackPushFloatFn CVirtualMachine::m_stackPushFloat = nullptr;
CVirtualMachine::StackPushVectorFn CVirtualMachine::m_stackPushVector = nullptr;
CVirtualMachine::StackPushStringFn CVirtualMachine::m_stackPushString = nullptr;
CVirtualMachine::StackPushEngineStructureFn CVirtualMachine::m_stackPushEngineStructure = nullptr;
CVirtualMachine::StackPushObjectFn CVirtualMachine::m_stackPushObject = nullptr;
CVirtualMachine::RunScriptFn CVirtualMachine::m_runScript = nullptr;

CVirtualMachine::CVirtualMachine(void* vmPtr)
    : m_vmPtr(vmPtr)
{
    if (!m_functionsInitialized) {
        InitializeFunctions();
    }
}

CVirtualMachine::~CVirtualMachine() {
    // Wrapper cleanup only - does NOT delete game object
}

void CVirtualMachine::InitializeFunctions() {
    if (m_functionsInitialized) {
        return;  // Already initialized
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CVirtualMachine] Warning: GameVersion not initialized, functions will be unavailable\n");
        m_functionsInitialized = true;
        return;
    }

    // Load all function addresses from GameVersion
    m_stackPopInt = (StackPopIntFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopInteger");
    m_stackPopFloat = (StackPopFloatFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopFloat");
    m_stackPopVector = (StackPopVectorFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopVector");
    m_stackPopString = (StackPopStringFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopString");
    m_stackPopEngineStructure = (StackPopEngineStructureFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopEngineStructure");
    m_stackPopObject = (StackPopObjectFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopObject");
    m_stackPopCommand = (StackPopCommandFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPopCommand");
    m_stackPushInt = (StackPushIntFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushInteger");
    m_stackPushFloat = (StackPushFloatFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushFloat");
    m_stackPushVector = (StackPushVectorFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushVector");
    m_stackPushString = (StackPushStringFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushString");
    m_stackPushEngineStructure = (StackPushEngineStructureFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushEngineStructure");
    m_stackPushObject = (StackPushObjectFn)GameVersion::GetFunctionAddress("CVirtualMachine", "StackPushObject");
    m_runScript = (RunScriptFn)GameVersion::GetFunctionAddress("CVirtualMachine", "RunScript");

    m_functionsInitialized = true;

    // Log which functions are available
    int availableCount = 0;
    if (m_stackPopInt) availableCount++;
    if (m_stackPopFloat) availableCount++;
    if (m_stackPopVector) availableCount++;
    if (m_stackPopString) availableCount++;
    if (m_stackPushInt) availableCount++;
    if (m_stackPushString) availableCount++;
    if (m_runScript) availableCount++;

    debugLog("[CVirtualMachine] Initialized with %d/14 functions available\n", availableCount);
}

CVirtualMachine* CVirtualMachine::GetInstance() {
    // Get global VM pointer from GameVersion
    void* vmPtrAddr = GameVersion::GetGlobalPointer("VIRTUAL_MACHINE_PTR");
    if (!vmPtrAddr) {
        debugLog("[CVirtualMachine] VIRTUAL_MACHINE_PTR not available for this version\n");
        return nullptr;
    }

    // Dereference to get actual VM object
    void* vmPtr = *reinterpret_cast<void**>(vmPtrAddr);
    if (!vmPtr) {
        debugLog("[CVirtualMachine] VM instance is null\n");
        return nullptr;
    }

    return new CVirtualMachine(vmPtr);
}

// ============================================================================
// Stack Pop Operations
// ============================================================================

bool CVirtualMachine::StackPopInteger(int* output) {
    if (!m_stackPopInt) {
        debugLog("[CVirtualMachine] StackPopInteger not available for this version\n");
        if (output) *output = 0;
        return false;
    }

    int result = m_stackPopInt(m_vmPtr, output);
    return result != 0;
}

bool CVirtualMachine::StackPopFloat(float* output) {
    if (!m_stackPopFloat) {
        debugLog("[CVirtualMachine] StackPopFloat not available for this version\n");
        if (output) *output = 0.0f;
        return false;
    }

    int result = m_stackPopFloat(m_vmPtr, output);
    return result != 0;
}

bool CVirtualMachine::StackPopVector(Vector* output) {
    if (!m_stackPopVector) {
        debugLog("[CVirtualMachine] StackPopVector not available for this version\n");
        if (output) {
            output->x = 0.0f;
            output->y = 0.0f;
            output->z = 0.0f;
        }
        return false;
    }

    int result = m_stackPopVector(m_vmPtr, output);
    return result != 0;
}

bool CVirtualMachine::StackPopString(CExoString* output) {
    if (!m_stackPopString) {
        debugLog("[CVirtualMachine] StackPopString not available for this version\n");
        return false;
    }

    int result = m_stackPopString(m_vmPtr, output);
    return result != 0;
}

bool CVirtualMachine::StackPopEngineStructure(VirtualMachineEngineStructureTypes type, void** output) {
    if (!m_stackPopEngineStructure) {
        debugLog("[CVirtualMachine] StackPopEngineStructure not available for this version\n");
        if (output) *output = nullptr;
        return false;
    }

    int result = m_stackPopEngineStructure(m_vmPtr, type, output);
    return result != 0;
}

bool CVirtualMachine::StackPopObject(DWORD* output) {
    if (!m_stackPopObject) {
        debugLog("[CVirtualMachine] StackPopObject not available for this version\n");
        if (output) *output = 0;
        return false;
    }

    int result = m_stackPopObject(m_vmPtr, output);
    return result != 0;
}

bool CVirtualMachine::StackPopCommand(void** output) {
    if (!m_stackPopCommand) {
        debugLog("[CVirtualMachine] StackPopCommand not available for this version\n");
        if (output) *output = nullptr;
        return false;
    }

    int result = m_stackPopCommand(m_vmPtr, output);
    return result != 0;
}

// ============================================================================
// Stack Push Operations
// ============================================================================

bool CVirtualMachine::StackPushInteger(int value) {
    if (!m_stackPushInt) {
        debugLog("[CVirtualMachine] StackPushInteger not available for this version\n");
        return false;
    }

    int result = m_stackPushInt(m_vmPtr, value);
    return result != 0;
}

bool CVirtualMachine::StackPushFloat(float value) {
    if (!m_stackPushFloat) {
        debugLog("[CVirtualMachine] StackPushFloat not available for this version\n");
        return false;
    }

    int result = m_stackPushFloat(m_vmPtr, value);
    return result != 0;
}

bool CVirtualMachine::StackPushVector(Vector value) {
    if (!m_stackPushVector) {
        debugLog("[CVirtualMachine] StackPushVector not available for this version\n");
        return false;
    }

    int result = m_stackPushVector(m_vmPtr, value);
    return result != 0;
}

bool CVirtualMachine::StackPushString(CExoString* value) {
    if (!m_stackPushString) {
        debugLog("[CVirtualMachine] StackPushString not available for this version\n");
        return false;
    }

    int result = m_stackPushString(m_vmPtr, value);
    return result != 0;
}

bool CVirtualMachine::StackPushEngineStructure(VirtualMachineEngineStructureTypes type, void* value) {
    if (!m_stackPushEngineStructure) {
        debugLog("[CVirtualMachine] StackPushEngineStructure not available for this version\n");
        return false;
    }

    int result = m_stackPushEngineStructure(m_vmPtr, type, value);
    return result != 0;
}

bool CVirtualMachine::StackPushObject(DWORD value) {
    if (!m_stackPushObject) {
        debugLog("[CVirtualMachine] StackPushObject not available for this version\n");
        return false;
    }

    int result = m_stackPushObject(m_vmPtr, value);
    return result != 0;
}

// ============================================================================
// Script Execution
// ============================================================================

bool CVirtualMachine::RunScript(CExoString* scriptName, DWORD objectSelf, int usually1) {
    if (!m_runScript) {
        debugLog("[CVirtualMachine] RunScript not available for this version\n");
        return false;
    }

    int result = m_runScript(m_vmPtr, scriptName, objectSelf, usually1);
    return result != 0;
}
