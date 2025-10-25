#include "VirtualMachine.h"
#include "fileIO.h"

const int TestScriptExtensionIndex = 772;
void __stdcall ExecuteCommandTestScriptExtension(DWORD routine, int paramCount) {
    char buffer[128];
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Called Test routine %d, with %i parameters", routine, paramCount);
    OutputDebugStringA(buffer);

    if (paramCount != 3) {
        OutputDebugStringA("[PATCH] Expected 3 params in the function!");
        stackPushInteger(*VIRTUAL_MACHINE_PTR, 0);
        return;
    }

    int testInt;
    stackPopInteger(*VIRTUAL_MACHINE_PTR, &testInt);

    memset(buffer, 0, sizeof(buffer));
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Test Int %i", testInt);
    OutputDebugStringA(buffer);

    float testFloat;
    stackPopFloat(*VIRTUAL_MACHINE_PTR, &testFloat);

    memset(buffer, 0, sizeof(buffer));
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Test Float %f", testFloat);
    OutputDebugStringA(buffer);

    CExoString testString;
    stackPopString(*VIRTUAL_MACHINE_PTR, &testString);

    memset(buffer, 0, sizeof(buffer));
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Test string \"%s\"", testString.c_string);
    OutputDebugStringA(buffer);

    stackPushInteger(*VIRTUAL_MACHINE_PTR, 1);

}

extern "C" void __cdecl InitializeExtensionCommands(DWORD* commands)
{
    char buffer[128];
    sprintf_s(buffer, sizeof(buffer), "[PATCH] Initializing Extension Commands. Commands Array: %p", commands);
    OutputDebugStringA(buffer);

    commands[TestScriptExtensionIndex] = (DWORD)&ExecuteCommandTestScriptExtension;
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