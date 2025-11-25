#pragma once

#include "GameVersion.h"
#include "../Common.h"
#include "CExoString.h"
#include "GameAPIObject.h"

class CVirtualMachine : public GameAPIObject {
public:
    static CVirtualMachine* GetInstance();
    ~CVirtualMachine();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

    bool StackPopInteger(int* output);
    bool StackPopFloat(float* output);
    bool StackPopVector(Vector* output);
    bool StackPopString(CExoString* output);
    bool StackPopEngineStructure(VirtualMachineEngineStructureTypes type, void** output);
    bool StackPopObject(DWORD* output);
    bool StackPopCommand(void** output);

    bool StackPushInteger(int value);
    bool StackPushFloat(float value);
    bool StackPushVector(Vector value);
    bool StackPushString(CExoString* value);
    bool StackPushEngineStructure(VirtualMachineEngineStructureTypes type, void* value);
    bool StackPushObject(DWORD value);

    bool RunScript(CExoString* scriptName, DWORD objectSelf, int usually1 = 1);

    bool CanStackPopInteger() const { return stackPopInt != nullptr; }
    bool CanStackPopFloat() const { return stackPopFloat != nullptr; }
    bool CanStackPopVector() const { return stackPopVector != nullptr; }
    bool CanStackPopString() const { return stackPopString != nullptr; }
    bool CanStackPushInteger() const { return stackPushInt != nullptr; }
    bool CanStackPushString() const { return stackPushString != nullptr; }
    bool CanRunScript() const { return runScript != nullptr; }

private:
    explicit CVirtualMachine(void* vmPtr);

    typedef int(__thiscall* StackPopIntFn)(void*, int*);
    typedef int(__thiscall* StackPopFloatFn)(void*, float*);
    typedef int(__thiscall* StackPopVectorFn)(void*, Vector*);
    typedef int(__thiscall* StackPopStringFn)(void*, void*);
    typedef int(__thiscall* StackPopEngineStructureFn)(void*, VirtualMachineEngineStructureTypes, void**);
    typedef int(__thiscall* StackPopObjectFn)(void*, DWORD*);
    typedef int(__thiscall* StackPopCommandFn)(void*, void**);
    typedef int(__thiscall* StackPushIntFn)(void*, int);
    typedef int(__thiscall* StackPushFloatFn)(void*, float);
    typedef int(__thiscall* StackPushVectorFn)(void*, Vector);
    typedef int(__thiscall* StackPushStringFn)(void*, void*);
    typedef int(__thiscall* StackPushEngineStructureFn)(void*, VirtualMachineEngineStructureTypes, void*);
    typedef int(__thiscall* StackPushObjectFn)(void*, DWORD);
    typedef int(__thiscall* RunScriptFn)(void*, void*, DWORD, int);

    static StackPopIntFn stackPopInt;
    static StackPopFloatFn stackPopFloat;
    static StackPopVectorFn stackPopVector;
    static StackPopStringFn stackPopString;
    static StackPopEngineStructureFn stackPopEngineStructure;
    static StackPopObjectFn stackPopObject;
    static StackPopCommandFn stackPopCommand;
    static StackPushIntFn stackPushInt;
    static StackPushFloatFn stackPushFloat;
    static StackPushVectorFn stackPushVector;
    static StackPushStringFn stackPushString;
    static StackPushEngineStructureFn stackPushEngineStructure;
    static StackPushObjectFn stackPushObject;
    static RunScriptFn runScript;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
