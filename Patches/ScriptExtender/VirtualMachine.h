#pragma once
#include "Common.h"
#pragma pack(push, 4)

void** VIRTUAL_MACHINE_PTR = (void **)0x007a3a00;

DWORD OBJECT_DEFAULT = (DWORD)0x7f000000;

// Structures
struct CScriptLocation {
	Vector postion;
	Vector orientation;
};

enum VirtualMachineEngineStructureTypes : int {
	UNKNOWN		  = -1,
	EFFECT		  = 0,
	SCRIPT_EVENT  = 1,
	LOCATION	  = 2,
	SCRIPT_TALENT = 3,
};

// Pop Functions
typedef int(__thiscall* StackPopInteger)(void* thisPtr, int* output);
typedef int(__thiscall* StackPopFloat)(void* thisPtr, float* output);
typedef int(__thiscall* StackPopVector)(void* thisPtr, Vector* output);
typedef int(__thiscall* StackPopString)(void* thisPtr, CExoString* output);
typedef int(__thiscall* StackPopEngineStructure)(void* thisPtr, VirtualMachineEngineStructureTypes type, void** output);
typedef int(__thiscall* StackPopObject)(void* thisPtr, DWORD* output);
typedef int(__thiscall* StackPopCommand)(void* thisPtr, void** output);

const DWORD VM_STACK_POP_INT = 0x005D1000;
const DWORD VM_STACK_POP_FLOAT = 0x005D1020;
const DWORD VM_STACK_POP_VECTOR = 0x005D1040;
const DWORD VM_STACK_POP_STRING = 0x005D1080;
const DWORD VM_STACK_POP_ENGINE_STRUCTURE = 0x005D10A0;
const DWORD VM_STACK_POP_OBJECT = 0x005D10C0;
const DWORD VM_STACK_POP_COMMAND = 0x005D10E0;

StackPopInteger          stackPopInteger = (StackPopInteger)VM_STACK_POP_INT;
StackPopFloat            stackPopFloat = (StackPopFloat)VM_STACK_POP_FLOAT;
StackPopVector           stackPopVector = (StackPopVector)VM_STACK_POP_VECTOR;
StackPopString           stackPopString = (StackPopString)VM_STACK_POP_STRING;
StackPopEngineStructure  stackPopEngineStructure = (StackPopEngineStructure)VM_STACK_POP_ENGINE_STRUCTURE;
StackPopObject           stackPopObject = (StackPopObject)VM_STACK_POP_OBJECT;
StackPopCommand          stackPopCommand = (StackPopCommand)VM_STACK_POP_COMMAND;


// Push Functions
typedef int(__thiscall* StackPushInteger)(void* thisPtr, int input);
typedef int(__thiscall* StackPushFloat)(void* thisPtr, float output);
typedef int(__thiscall* StackPushVector)(void* thisPtr, Vector input);
typedef int(__thiscall* StackPushString)(void* thisPtr, CExoString* input);
typedef int(__thiscall* StackPushEngineStructure)(void* thisPtr, VirtualMachineEngineStructureTypes type, void* input);
typedef int(__thiscall* StackPushObject)(void* thisPtr, DWORD input);

const DWORD VM_STACK_PUSH_INT = 0x005D1010;
const DWORD VM_STACK_PUSH_FLOAT = 0x005D1030;
const DWORD VM_STACK_PUSH_VECTOR = 0x005D1050;
const DWORD VM_STACK_PUSH_STRING = 0x005D1090;
const DWORD VM_STACK_PUSH_ENGINE_STRUCTURE = 0x005D10B0;
const DWORD VM_STACK_PUSH_OBJECT = 0x005D10D0;

StackPushInteger          stackPushInteger = (StackPushInteger)VM_STACK_PUSH_INT;
StackPushFloat            stackPushFloat = (StackPushFloat)VM_STACK_PUSH_FLOAT;
StackPushVector           stackPushVector = (StackPushVector)VM_STACK_PUSH_VECTOR;
StackPushString           stackPushString = (StackPushString)VM_STACK_PUSH_STRING;
StackPushEngineStructure  stackPushEngineStructure = (StackPushEngineStructure)VM_STACK_PUSH_ENGINE_STRUCTURE;
StackPushObject           stackPushObject = (StackPushObject)VM_STACK_PUSH_OBJECT;

#pragma pack(pop)