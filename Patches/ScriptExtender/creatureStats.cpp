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
		BYTE classIndex = *((BYTE*)creatureStats + 0x89)
		creatureStatsAddKnownSpell(creatureStats, classIndex, (DWORD)ability)
	}

	return 0;
}

int __stdcall ExecuteCommandSetBonusForcePoints(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandAddBonusForcePoints(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandGetBonusForcePoints(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandModifyReflexSavingThrowBase(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandModifyFortitudeSavingThrowBase(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandModifyWillSavingThrowBase(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount)
{


	return 0;
}

int __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount)
{


	return 0;
}
