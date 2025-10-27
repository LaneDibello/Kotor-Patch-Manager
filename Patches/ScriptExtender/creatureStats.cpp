#include "creatureStats.h"


void __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount)
{
	if (paramCount != 2) {
		DebugLog("[PATCH] Wrong number of params found in ExecuteCommandGetFeatAcquired. Expected 2, got %i", paramCount);
		stackPushInt(*VIRTUAL_MACHINE_PTR, 0);
		return;
	}

	int feat;
	stackPopInteger(*VIRTUAL_MACHINE_PTR, feat);

	DWORD creature;
	stackPopObject(*VIRTUAL_MACHINE_PTR, creature);


}

void __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandGrantFeat(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandGrantSpell(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandSetBonusForcePoints(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandAddBonusForcePoints(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandGetBonusForcePoints(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandModifyReflexSavingThrowBase(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandModifyFortitudeSavingThrowBase(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandModifyWillSavingThrowBase(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount)
{

}

void __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount)
{

}