#pragma once
#include <windows.h>
#include <stdio.h>
#include "VirtualMachine.h"

const int GetFeatAcquiredIndex = 780;
void __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount);

const int GetSpellAcquiredIndex = 781;
void __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount);

const int GrantFeatIndex = 782;
void __stdcall ExecuteCommandGrantFeat(DWORD routine, int paramCount);

const int GrantSpellIndex = 783;
void __stdcall ExecuteCommandGrantSpell(DWORD routine, int paramCount);

const int SetBonusForcePointsIndex = 784;
void __stdcall ExecuteCommandSetBonusForcePoints(DWORD routine, int paramCount);

const int AddBonusForcePointsIndex = 785;
void __stdcall ExecuteCommandAddBonusForcePoints(DWORD routine, int paramCount);

const int GetBonusForcePointsIndex = 786;
void __stdcall ExecuteCommandGetBonusForcePoints(DWORD routine, int paramCount);

const int ModifyReflexSavingThrowBaseIndex = 787;
void __stdcall ExecuteCommandModifyReflexSavingThrowBase(DWORD routine, int paramCount);

const int ModifyFortitudeSavingThrowBaseIndex = 788;
void __stdcall ExecuteCommandModifyFortitudeSavingThrowBase(DWORD routine, int paramCount);

const int ModifyWillSavingThrowBaseIndex = 789;
void __stdcall ExecuteCommandModifyWillSavingThrowBase(DWORD routine, int paramCount);

const int AdjustCreatureAttributesIndex = 790;
void __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount);

const int AdjustCreatureSkillsIndex = 791;
void __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount);

const int GetSkillRankBaseIndex = 792;
void __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount);


