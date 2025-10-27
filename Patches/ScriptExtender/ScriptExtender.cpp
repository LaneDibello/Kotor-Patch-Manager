#include "VirtualMachine.h"
#include "fileIO.h"
#include "creatureStats.h"

const int TestScriptExtensionIndex = 772;
void __stdcall ExecuteCommandTestScriptExtension(DWORD routine, int paramCount) {
    DebugLog("[PATCH] Called Test routine %d, with %i parameters", routine, paramCount);

    if (paramCount != 3) {
        DebugLog("[PATCH] Expected 3 params in the function!");
        stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
        return;
    }

    int testInt;
    stackPopInteger(*VIRTUAL_MACHINE_PTR, &testInt);

    DebugLog("[PATCH] Test Int %i", testInt);

    float testFloat;
    stackPopFloat(*VIRTUAL_MACHINE_PTR, &testFloat);

    DebugLog("[PATCH] Test Float %f", testFloat);

    CExoString testString;
    stackPopString(*VIRTUAL_MACHINE_PTR, &testString);

    DebugLog("[PATCH] Test string \"%s\"", testString.c_string);

    stackPushInteger(*VIRTUAL_MACHINE_PTR, 1);

}

extern "C" void __cdecl InitializeExtensionCommands(DWORD* commands)
{
    DebugLog("[PATCH] Initializing Extension Commands. Commands Array: %p", commands);

    commands[TestScriptExtensionIndex] = (DWORD)&ExecuteCommandTestScriptExtension;
    commands[OpenFileIndex] = (DWORD)&ExecuteCommandOpenFile;
    commands[CloseFileIndex] = (DWORD)&ExecuteCommandCloseFile;
    commands[ReadFileIndex] = (DWORD)&ExecuteCommandReadTextFile;
    commands[WriteFileIndex] = (DWORD)&ExecuteCommandWriteTextFile;
    commands[PeakCharFileIndex] = (DWORD)&ExecuteCommandPeakCharFile;
    commands[SeekFileIndex] = (DWORD)&ExecuteCommandSeekFile;
    commands[TellFileIndex] = (DWORD)&ExecuteCommandTellFile;
    commands[GetFeatAcquiredIndex] = (DWORD)&ExecuteCommandGetFeatAcquired;
    commands[GetSpellAcquiredIndex] = (DWORD)&ExecuteCommandGetSpellAcquired;
    commands[GrantFeatIndex] = (DWORD)&ExecuteCommandGrantFeat;
    commands[GrantSpellIndex] = (DWORD)&ExecuteCommandGrantSpell;
    commands[SpawnMineIndex] = (DWORD)&ExecuteCommandSpawnMine;
    commands[GetSpellForcePointCostIndex] = (DWORD)&ExecuteCommandGetSpellForcePointCost;
    commands[SetBonusForcePointsIndex] = (DWORD)&ExecuteCommandSetBonusForcePoints;
    commands[AddBonusForcePointsIndex] = (DWORD)&ExecuteCommandAddBonusForcePoints;
    commands[GetBonusForcePointsIndex] = (DWORD)&ExecuteCommandGetBonusForcePoints;
    commands[ModifyReflexSavingThrowBaseIndex] = (DWORD)&ExecuteCommandModifyReflexSavingThrowBase;
    commands[ModifyFortitudeSavingThrowBaseIndex] = (DWORD)&ExecuteCommandModifyFortitudeSavingThrowBase;
    commands[ModifyWillSavingThrowBaseIndex] = (DWORD)&ExecuteCommandModifyWillSavingThrowBase;
    commands[AdjustCreatureAttributesIndex] = (DWORD)&ExecuteCommandAdjustCreatureAttributes;
    commands[AdjustCreatureSkillsIndex] = (DWORD)&ExecuteCommandAdjustCreatureSkills;
    commands[GetSkillRankBaseIndex] = (DWORD)&ExecuteCommandGetSkillRankBase;

}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}