#include "Common.h"
#include "Kotor1Functions.h"
#include "fileIO.h"
#include "creatureStats.h"

const int TestScriptExtensionIndex = 772;
int __stdcall ExecuteCommandTestScriptExtension(DWORD routine, int paramCount) {
    debugLog("[PATCH] Called Test routine %d, with %i parameters", routine, paramCount);

    if (paramCount != 3) {
        debugLog("[PATCH] Expected 3 params in the function!");
        virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
        return 0;
    }

    int testInt;
    virtualMachineStackPopInteger(*VIRTUAL_MACHINE_PTR, &testInt);

    debugLog("[PATCH] Test Int %i", testInt);

    float testFloat;
    virtualMachineStackPopFloat(*VIRTUAL_MACHINE_PTR, &testFloat);

    debugLog("[PATCH] Test Float %f", testFloat);

    CExoString testString;
    virtualMachineStackPopString(*VIRTUAL_MACHINE_PTR, &testString);

    debugLog("[PATCH] Test string \"%s\"", testString.c_string);

    virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, 1);

    return 0;
}

extern "C" void __cdecl InitializeExtensionCommands(DWORD* commands)
{
    debugLog("[PATCH] Initializing Extension Commands. Commands Array: %p", commands);

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
    commands[GrantFeatIndex] = (DWORD)&ExecuteCommandGrantAbility;
    commands[GrantSpellIndex] = (DWORD)&ExecuteCommandGrantAbility;
    commands[AdjustCreatureAttributesIndex] = (DWORD)&ExecuteCommandAdjustCreatureAttributes;
    commands[AdjustCreatureSkillsIndex] = (DWORD)&ExecuteCommandAdjustCreatureSkills;
    commands[GetSkillRankBaseIndex] = (DWORD)&ExecuteCommandGetSkillRankBase;

    debugLog("[PATCH] GetFeatAcquired at %p", &ExecuteCommandGetFeatAcquired);
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