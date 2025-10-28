#pragma once
#include "Kotor1Functions.h"
#include "VirtualFunctionCall.h"

const int GetFeatAcquiredIndex = 780;
int __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount);

const int GetSpellAcquiredIndex = 781;
int __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount);

const int GrantFeatIndex = 782;
const int GrantSpellIndex = 783;
int __stdcall ExecuteCommandGrantAbility(DWORD routine, int paramCount);

const int AdjustCreatureAttributesIndex = 784;
int __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount);

const int AdjustCreatureSkillsIndex = 785;
int __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount);

const int GetSkillRankBaseIndex = 786;
int __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount);

inline void* getServerCreatureStats(void* serverCreature) {
	return *(void**)((char*)serverCreature + 0xa74);
}

enum Attributes {
	STR,
	DEX,
	CON,
	INT,
	WIS,
	CHA
};