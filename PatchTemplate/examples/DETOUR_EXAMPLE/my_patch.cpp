// Example DETOUR Patch DLL
// This demonstrates a simple hook function with parameter extraction

#include <windows.h>
#include <stdio.h>

// Hook function that will be called when the game reaches the hooked address
// Parameters are extracted according to hooks.toml configuration
//
// extern "C" __cdecl is required for proper export
// Function name must match the "function" field in hooks.toml
extern "C" void __cdecl MyHookFunction(void* param1, int param2)
{
    // Debug output to see when hook is called
    char debugMsg[256];
    sprintf_s(debugMsg, "[MyPatch] Hook called! param1=%p, param2=%d\n",
        param1, param2);
    OutputDebugStringA(debugMsg);

    // Your patch logic goes here
    // - Call game functions
    // - Modify game state
    // - Implement new features
    // etc.

    // Example: Call an original game function
    // typedef void (__cdecl *GameFunction)(int);
    // GameFunction originalFunc = (GameFunction)0x00405678;
    // originalFunc(param2 * 2);
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // DLL is being loaded into the process
            // You can initialize things here if needed
            OutputDebugStringA("[MyPatch] DLL loaded\n");
            break;

        case DLL_PROCESS_DETACH:
            // DLL is being unloaded
            // Clean up resources here if needed
            OutputDebugStringA("[MyPatch] DLL unloaded\n");
            break;
    }
    return TRUE;
}
