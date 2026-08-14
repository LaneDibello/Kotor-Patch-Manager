#pragma once
#include "Common.h"
#include "VirtualFunctionCall.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CGameObjectArray.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWSCreatureStats.h"

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
	INTEL, // INT is a reserved term
	WIS,
	CHA
};