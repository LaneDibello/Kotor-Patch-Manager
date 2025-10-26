#include "VirtualMachine.h"

// VM Pointer and constants
void** VIRTUAL_MACHINE_PTR = (void **)0x007a3a00;
DWORD OBJECT_DEFAULT = (DWORD)0x7f000000;

// Stack Pop functions
StackPopInteger          stackPopInteger = (StackPopInteger)VM_STACK_POP_INT;
StackPopFloat            stackPopFloat = (StackPopFloat)VM_STACK_POP_FLOAT;
StackPopVector           stackPopVector = (StackPopVector)VM_STACK_POP_VECTOR;
StackPopString           stackPopString = (StackPopString)VM_STACK_POP_STRING;
StackPopEngineStructure  stackPopEngineStructure = (StackPopEngineStructure)VM_STACK_POP_ENGINE_STRUCTURE;
StackPopObject           stackPopObject = (StackPopObject)VM_STACK_POP_OBJECT;
StackPopCommand          stackPopCommand = (StackPopCommand)VM_STACK_POP_COMMAND;

// Stack Push functions
StackPushInteger          stackPushInteger = (StackPushInteger)VM_STACK_PUSH_INT;
StackPushFloat            stackPushFloat = (StackPushFloat)VM_STACK_PUSH_FLOAT;
StackPushVector           stackPushVector = (StackPushVector)VM_STACK_PUSH_VECTOR;
StackPushString           stackPushString = (StackPushString)VM_STACK_PUSH_STRING;
StackPushEngineStructure  stackPushEngineStructure = (StackPushEngineStructure)VM_STACK_PUSH_ENGINE_STRUCTURE;
StackPushObject           stackPushObject = (StackPushObject)VM_STACK_PUSH_OBJECT;
