#pragma once
#include "Common.h"

// CServerExoApp

typedef void* (__thiscall* ServerExoApp_GetObjectArray)(void* thisPtr);

const DWORD SERVER_EXO_APP_GET_OBJECT_ARRAY = 0x004aed7;

extern ServerExoApp_GetObjectArray serverExoAppGetObjectArray;


// CGameObjectArray

typedef void* (__thiscall* GameObjectArray_GetGameObject)(void* thisPtr);

const DWORD SERVER_GET_OBJECT_ARRAY = 0x004d8230;

extern GameObjectArray_GetGameObject gameObjectArrayGetGameObject;


// CSWCCreatureStats

typedef int(__thiscall* CreatureStats_HasFeat)(void* thisPtr, USHORT feat);
typedef int(__thiscall* CreatureStats_HasSpell)(void* thisPtr, BYTE spellList, DWORD spell, int checkUsable);
typedef void(__thiscall* CreatureStats_AddFeat)(void* thisPtr, USHORT feat);
typedef void(__thiscall* CreatureStats_AddKnownSpell)(void* thisPtr, BYTE classIndex, DWORD spell);
typedef void(__thiscall* CreatureStats_RemoveFeat)(void* thisPtr, USHORT feat);
typedef void(__thiscall* CreatureStats_SetAttributeBase)(void* thisPtr, BYTE amount);
typedef void(__thiscall* CreatureStats_SetCONBase)(void* thisPtr, BYTE amount, int setHP);
typedef void(__thiscall* CreatureStats_GetSkillRank)(void* thisPtr, BYTE skill, void* ignore1, int ignore2); // ignore params are for special cases
typedef void(__thiscall* CreatureStats_SetSkillRank)(void* thisPtr, BYTE skill, BYTE value);
typedef void(__thiscall* CreatureStats_SetMovementRate)(void* thisPtr, int rate);

const DWORD CREATURE_STATS_HAS_FEAT = 0x005a6630;
const DWORD CREATURE_STATS_HAS_SPELL = 0x005a6e70;
const DWORD CREATURE_STATS_ADD_FEAT = 0x005aa810;
const DWORD CREATURE_STATS_ADD_KNOWN_SPELL = 0x005aa9b0;
const DWORD CREATURE_STATS_REMOVE_FEAT = 0x005aa940;
const DWORD CREATURE_STATS_SET_STR_BASE = 0x005a9fe0;
const DWORD CREATURE_STATS_SET_DEX_BASE = 0x005aa020;
const DWORD CREATURE_STATS_SET_CON_BASE = 0x005aa060;
const DWORD CREATURE_STATS_SET_INT_BASE = 0x005aa0f0;
const DWORD CREATURE_STATS_SET_WIS_BASE = 0x005aa130;
const DWORD CREATURE_STATS_SET_CHA_BASE = 0x005aa170;
const DWORD CREATURE_STATS_GET_SKILL_RANK = 0x005aa570;
const DWORD CREATURE_STATS_SET_SKILL_RANK = 0x005a54c0;
const DWORD CREATURE_STATS_SET_MOVEMENT_RATE = 0x005a5680;

extern CreatureStats_HasFeat creatureStatsHasFeat;
extern CreatureStats_HasSpell creatureStatsHasSpell;
extern CreatureStats_AddFeat creatureStatsAddFeat;
extern CreatureStats_AddKnownSpell creatureStatsAddKnownSpell;
extern CreatureStats_RemoveFeat creatureStatsRemoveFeat;
extern CreatureStats_SetAttributeBase creatureStatsSetSTRBase;
extern CreatureStats_SetAttributeBase creatureStatsSetDEXBase;
extern CreatureStats_SetCONBase creatureStatsSetCONBase;
extern CreatureStats_SetAttributeBase creatureStatsSetINTBase;
extern CreatureStats_SetAttributeBase creatureStatsSetWISBase;
extern CreatureStats_SetAttributeBase creatureStatsSetCHABase;
extern CreatureStats_GetSkillRank creatureStatsGetSkillRank;
extern CreatureStats_SetSkillRank creatureStatsSetSkillRank;
extern CreatureStats_SetMovementRate creatureStatsSetMovementRate;

// CVirtualMachine
typedef int(__thiscall* VirtualMachine_StackPopInteger)(void* thisPtr, int* output);
typedef int(__thiscall* VirtualMachine_StackPopFloat)(void* thisPtr, float* output);
typedef int(__thiscall* VirtualMachine_StackPopVector)(void* thisPtr, Vector* output);
typedef int(__thiscall* VirtualMachine_StackPopString)(void* thisPtr, CExoString* output);
typedef int(__thiscall* VirtualMachine_StackPopEngineStructure)(void* thisPtr, VirtualMachineEngineStructureTypes type, void** output);
typedef int(__thiscall* VirtualMachine_StackPopObject)(void* thisPtr, DWORD* output);
typedef int(__thiscall* VirtualMachine_StackPopCommand)(void* thisPtr, void** output);
typedef int(__thiscall* VirtualMachine_StackPushInteger)(void* thisPtr, int input);
typedef int(__thiscall* VirtualMachine_StackPushFloat)(void* thisPtr, float output);
typedef int(__thiscall* VirtualMachine_StackPushVector)(void* thisPtr, Vector input);
typedef int(__thiscall* VirtualMachine_StackPushString)(void* thisPtr, CExoString* input);
typedef int(__thiscall* VirtualMachine_StackPushEngineStructure)(void* thisPtr, VirtualMachineEngineStructureTypes type, void* input);
typedef int(__thiscall* VirtualMachine_StackPushObject)(void* thisPtr, DWORD input);

const DWORD VIRTUAL_MACHINE_STACK_POP_INT = 0x005D1000;
const DWORD VIRTUAL_MACHINE_STACK_POP_FLOAT = 0x005D1020;
const DWORD VIRTUAL_MACHINE_STACK_POP_VECTOR = 0x005D1040;
const DWORD VIRTUAL_MACHINE_STACK_POP_STRING = 0x005D1080;
const DWORD VIRTUAL_MACHINE_STACK_POP_ENGINE_STRUCTURE = 0x005D10A0;
const DWORD VIRTUAL_MACHINE_STACK_POP_OBJECT = 0x005D10C0;
const DWORD VIRTUAL_MACHINE_STACK_POP_COMMAND = 0x005D10E0;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_INT = 0x005D1010;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_FLOAT = 0x005D1030;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_VECTOR = 0x005D1050;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_STRING = 0x005D1090;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_ENGINE_STRUCTURE = 0x005D10B0;
const DWORD VIRTUAL_MACHINE_STACK_PUSH_OBJECT = 0x005D10D0;

extern VirtualMachine_StackPopInteger virtualMachineStackPopInteger;
extern VirtualMachine_StackPopFloat virtualMachineStackPopFloat;
extern VirtualMachine_StackPopVector virtualMachineStackPopVector;
extern VirtualMachine_StackPopString virtualMachineStackPopString;
extern VirtualMachine_StackPopEngineStructure virtualMachineStackPopEngineStructure;
extern VirtualMachine_StackPopObject virtualMachineStackPopObject;
extern VirtualMachine_StackPopCommand virtualMachineStackPopCommand;
extern VirtualMachine_StackPushInteger virtualMachineStackPushInteger;
extern VirtualMachine_StackPushFloat virtualMachineStackPushFloat;
extern VirtualMachine_StackPushVector virtualMachineStackPushVector;
extern VirtualMachine_StackPushString virtualMachineStackPushString;
extern VirtualMachine_StackPushEngineStructure  virtualMachineStackPushEngineStructure;
extern VirtualMachine_StackPushObject virtualMachineStackPushObject;