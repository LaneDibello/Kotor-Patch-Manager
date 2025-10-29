#include "creatureStats.h"

int __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount)
{
	int outcome = 0;

	int feat;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &feat))
		return -2001;

	DWORD creature;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &creature))
		return -2001;

	void* game_object_array = serverExoAppGetObjectArray(getServerExoApp());

	void* creatureObject;
	gameObjectArrayGetGameObject(game_object_array, creature, &creatureObject);

	// Game object vtable[12] = AsSWSCreature
	void* serverCreature = callVirtualFunction<void*>(creatureObject, 12);
	if (!serverCreature)
	{
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, outcome);
		return 0;
	}

	void* creatureStats = getServerCreatureStats(serverCreature);

	outcome = creatureStatsHasFeat(creatureStats, feat);

	if (!virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, outcome))
		return -2000;

	return 0;
}

int __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount)
{
	int outcome = 0;

	int spell;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &spell))
		return -2001;

	DWORD creature;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &creature))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), creature);

	if (serverCreature) {
		void* creatureStats = getServerCreatureStats(serverCreature);
		outcome = creatureStatsHasSpell(creatureStats, 0, (DWORD)spell, 0);
	}

	if (!virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, outcome))
		return -2000;

	return 0;
}

int __stdcall ExecuteCommandGrantAbility(DWORD routine, int paramCount)
{
	int ability;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &ability))
		return -2001;

	DWORD creature;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &creature))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), creature);

	if (!serverCreature)
		return 0;

	void* creatureStats = getServerCreatureStats(serverCreature);
	
	if (routine == GrantFeatIndex)
		creatureStatsAddFeat(creatureStats, (USHORT)ability);
	else if (routine == GrantSpellIndex)
	{
		// Give to last class for now
		// In the future consider alternate options to guarantee Jedi class
		BYTE classIndex = *((BYTE*)creatureStats + 0x89) - 1;
		debugLog("Class Index was %d", classIndex);
		creatureStatsAddKnownSpell(creatureStats, classIndex, (DWORD)ability);
	}

	return 0;
}

int __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount)
{
	DWORD object;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &object))
		return -2001;

	int attribute;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &attribute))
		return -2001;

	int amount;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &amount))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), object);

	if (!serverCreature)
		return 0;

	void* creatureStats = getServerCreatureStats(serverCreature);

	BYTE baseCurrent = 0;
	switch ((Attributes)attribute) {
	case STR:
		baseCurrent = *((BYTE*)creatureStats + 0xe9);
		creatureStatsSetSTRBase(creatureStats, (BYTE)(amount + (int)baseCurrent));
		break;
	case DEX:
		baseCurrent = *((BYTE*)creatureStats + 0xeb);
		creatureStatsSetDEXBase(creatureStats, (BYTE)(amount + (int)baseCurrent));
		break;
	case CON:
		baseCurrent = *((BYTE*)creatureStats + 0xed);
		creatureStatsSetCONBase(creatureStats, (BYTE)(amount + (int)baseCurrent), 1);
		break;
	case INTEL:
		baseCurrent = *((BYTE*)creatureStats + 0xef);
		creatureStatsSetINTBase(creatureStats, (BYTE)(amount + (int)baseCurrent));
		break;
	case WIS:
		baseCurrent = *((BYTE*)creatureStats + 0xf1);
		creatureStatsSetWISBase(creatureStats, (BYTE)(amount + (int)baseCurrent));
		break;
	case CHA:
		baseCurrent = *((BYTE*)creatureStats + 0xf3);
		creatureStatsSetCHABase(creatureStats, (BYTE)(amount + (int)baseCurrent));
		break;
	}

	return 0;
}

int __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount)
{
	DWORD object;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &object))
		return -2001;

	int skill;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &skill))
		return -2001;

	int amount;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &amount))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), object);

	if (!serverCreature)
		return 0;

	void* creatureStats = getServerCreatureStats(serverCreature);

	BYTE baseCurrent = creatureStatsGetSkillRank(creatureStats, (BYTE)skill, NULL, 1);
	creatureStatsSetSkillRank(creatureStats, (BYTE)skill, (BYTE)((int)baseCurrent + amount));

	return 0;
}

int __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount)
{
	int skill;
	if (!virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &skill))
		return -2001;

	DWORD object;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &object))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), object);

	if (!serverCreature) {
		virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, -1);
		return 0;
	}

	void* creatureStats = getServerCreatureStats(serverCreature);

	BYTE skillBase = creatureStatsGetSkillRank(creatureStats, (BYTE)skill, NULL, 1);

	if (!virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, skillBase))
		return -2000;

	return 0;
}
