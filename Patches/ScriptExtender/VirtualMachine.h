#pragma once
#include "Common.h"
#pragma pack(push, 4)

extern void** VIRTUAL_MACHINE_PTR;

extern DWORD OBJECT_DEFAULT;

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

extern StackPopInteger          stackPopInteger;
extern StackPopFloat            stackPopFloat;
extern StackPopVector           stackPopVector;
extern StackPopString           stackPopString;
extern StackPopEngineStructure  stackPopEngineStructure;
extern StackPopObject           stackPopObject;
extern StackPopCommand          stackPopCommand;


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

extern StackPushInteger          stackPushInteger;
extern StackPushFloat            stackPushFloat;
extern StackPushVector           stackPushVector;
extern StackPushString           stackPushString;
extern StackPushEngineStructure  stackPushEngineStructure;
extern StackPushObject           stackPushObject;

#pragma pack(pop)