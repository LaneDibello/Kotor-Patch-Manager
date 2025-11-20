#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoString.h"
#include "fileIO.h"
#include "creatureStats.h"
#include "clientCreatures.h"
#include "twoDA.h"

const int TestScriptExtensionIndex = 772;
int __stdcall ExecuteCommandTestScriptExtension(DWORD routine, int paramCount) {
    debugLog("[PATCH] Called Test routine %d, with %i parameters", routine, paramCount);

    if (paramCount != 3) {
        debugLog("[PATCH] Expected 3 params in the function!");
        CVirtualMachine* vm = CVirtualMachine::GetInstance();
        if (vm) {
            vm->StackPushInteger(0);
            delete vm;
        }
        return 0;
    }

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return -2001;

    int testInt;
    if (!vm->StackPopInteger(&testInt)) {
        delete vm;
        return -2001;
    }

    debugLog("[PATCH] Test Int %i", testInt);

    float testFloat;
    if (!vm->StackPopFloat(&testFloat)) {
        delete vm;
        return -2001;
    }

    debugLog("[PATCH] Test Float %f", testFloat);

    CExoString testString;
    if (!vm->StackPopString(&testString)) {
        delete vm;
        return -2001;
    }

    debugLog("[PATCH] Test string \"%s\"", testString.GetCStr());

    if (!vm->StackPushInteger(1)) {
        delete vm;
        return -2000;
    }

    delete vm;
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
    debugLog("[PATCH] GetSkillRankBase at %p", &ExecuteCommandGetSkillRankBase);

    commands[IsRunningIndex] = (DWORD)&ExecuteCommandIsRunning;
    commands[IsStealthedIndex] = (DWORD)&ExecuteCommandIsStealthed;

    commands[Get2DAStringIndex] = (DWORD)&ExecuteCommandGet2DAString;
    commands[Get2DAIntIndex] = (DWORD)&ExecuteCommandGet2DAInt;
    commands[Get2DAFloatIndex] = (DWORD)&ExecuteCommandGet2DAFloat;
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize GameVersion system (reads from KOTOR_VERSION_SHA env var and addresses.toml)
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