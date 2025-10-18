// EnableScriptAurPostString Patch
// Re-enables the debug AurPostString function in KOTOR 1
//
// This patch intercepts the ExecuteCommandAurPostString function at 0x005cb41c
// and calls the actual AurPostString display function at 0x0044d490

#include <windows.h>

// Function pointer to the actual AurPostString function
// Signature: void __cdecl AurPostString(char *string, int y, int x, float life)
typedef void (__cdecl *AurPostStringFunc)(char*, int, int, float);
const DWORD AUR_POST_STRING_ADDR = 0x0044d490;

// Attempt to use paramters
extern "C" void __cdecl EnableAurPostString_Hook(char* string, int x, int y, float life)
{
    AurPostStringFunc aurPostString = (AurPostStringFunc)AUR_POST_STRING_ADDR;
    aurPostString(string, y, x, life);
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // Optional: Initialize if needed
            break;

        case DLL_PROCESS_DETACH:
            // Optional: Cleanup if needed
            break;
    }
    return TRUE;
}
