#pragma once
#include "Kotor1Functions.h"
#include "VirtualFunctionCall.h"

const int GetFeatAcquiredIndex = 780;
int __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount);

const int GetSpellAcquiredIndex = 781;
int __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount);

const int GrantFeatIndex = 782;
int __stdcall ExecuteCommandGrantFeat(DWORD routine, int paramCount);

const int GrantSpellIndex = 783;
int __stdcall ExecuteCommandGrantSpell(DWORD routine, int paramCount);

const int SetBonusForcePointsIndex = 784;
int __stdcall ExecuteCommandSetBonusForcePoints(DWORD routine, int paramCount);

const int AddBonusForcePointsIndex = 785;
int __stdcall ExecuteCommandAddBonusForcePoints(DWORD routine, int paramCount);

const int GetBonusForcePointsIndex = 786;
int __stdcall ExecuteCommandGetBonusForcePoints(DWORD routine, int paramCount);

const int ModifyReflexSavingThrowBaseIndex = 787;
int __stdcall ExecuteCommandModifyReflexSavingThrowBase(DWORD routine, int paramCount);

const int ModifyFortitudeSavingThrowBaseIndex = 788;
int __stdcall ExecuteCommandModifyFortitudeSavingThrowBase(DWORD routine, int paramCount);

const int ModifyWillSavingThrowBaseIndex = 789;
int __stdcall ExecuteCommandModifyWillSavingThrowBase(DWORD routine, int paramCount);

const int AdjustCreatureAttributesIndex = 790;
int __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount);

const int AdjustCreatureSkillsIndex = 791;
int __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount);

const int GetSkillRankBaseIndex = 792;
int __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount);

inline void* getServerCreatureStats(void* serverCreature) {
	return *(void**)((char*)serverCreature + 0xa74);
}
