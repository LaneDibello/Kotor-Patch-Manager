#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoString.h"
#include "Extensions/fileIO.h"
#include "Extensions/creatureStats.h"
#include "Extensions/clientCreatures.h"
#include "Extensions/twoDA.h"
#include "Extensions/globalModifiers.h"
#include "Extensions/trig.h"
#include "Extensions/consoleCommand.h"
#include "Extensions/lastTarget.h"
#include "Extensions/ini.h"
#include "Extensions/heartbeat.h"

const int TestScriptExtensionIndex = 772;
VirtualMachineReturnTypes __stdcall ExecuteCommandTestScriptExtension(DWORD routine, int paramCount) {
    debugLog("[ScriptExtender] Called Test routine %d, with %i parameters", routine, paramCount);

    if (paramCount != 3) {
        debugLog("[ScriptExtender] Expected 3 params in the function!");
        CVirtualMachine* vm = CVirtualMachine::GetInstance();
        if (vm) {
            vm->StackPushInteger(0);
            delete vm;
        }
        return COMMAND_NOT_FOUND;
    }

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return COMMAND_PARAM_ERROR;

    int testInt;
    if (!vm->StackPopInteger(&testInt)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    debugLog("[ScriptExtender] Test Int %i", testInt);

    float testFloat;
    if (!vm->StackPopFloat(&testFloat)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    debugLog("[ScriptExtender] Test Float %f", testFloat);

    CExoString testString;
    if (!vm->StackPopString(&testString)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    debugLog("[ScriptExtender] Test string \"%s\"", testString.GetCStr());

    if (!vm->StackPushInteger(1)) {
        delete vm;
        return COMMAND_RETURN_ERROR;
    }

    delete vm;
    return SUCCESS;
}

extern "C" void __cdecl InitializeExtensionCommands(DWORD* commands)
{
    debugLog("[ScriptExtender] Initializing Extension Commands. Commands Array: %p", commands);

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

    commands[IsRunningIndex] = (DWORD)&ExecuteCommandIsRunning;
    commands[IsStealthedIndex] = (DWORD)&ExecuteCommandIsStealthed;

    commands[Get2DAStringIndex] = (DWORD)&ExecuteCommandGet2DAString;
    commands[Get2DAIntIndex] = (DWORD)&ExecuteCommandGet2DAInt;
    commands[Get2DAFloatIndex] = (DWORD)&ExecuteCommandGet2DAFloat;

    commands[IncrementGlobalNumberIndex] = (DWORD)&ExecuteCommandAdjustGlobalNumber;
    commands[DecrementGlobalNumberIndex] = (DWORD)&ExecuteCommandAdjustGlobalNumber;

    commands[secIndex] = (DWORD)&ExecuteCommandTrig;
    commands[cscIndex] = (DWORD)&ExecuteCommandTrig;
    commands[cotIndex] = (DWORD)&ExecuteCommandTrig;
    commands[RadToDegIndex] = (DWORD)&ExecuteCommandRadToDeg;
    commands[DegToRadIndex] = (DWORD)&ExecuteCommandDegToRad;

    commands[RunConsoleCommandIndex] = (DWORD)&ExecuteCommandRunConsoleCommand;
    commands[GetPlayerLastTargetObjectIndex] = (DWORD)&ExecuteCommandetGetPlayerLastTarget;

    commands[ReadIniEntryIndex] = (DWORD)&ExecuteCommandReadIniEntry;
    commands[WriteIniEntryIndex] = (DWORD)&ExecuteCommandWriteIniEntry;

    commands[ForceHeartbeatIndex] = (DWORD)&ExecuteCommandForceHeartbeat;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[ScriptExtender] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[ScriptExtender] GameVersion initialized successfully");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}