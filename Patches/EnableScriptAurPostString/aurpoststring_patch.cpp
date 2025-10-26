// EnableScriptAurPostString Patch
// Re-enables the debug AurPostString function in KOTOR 1

#include <windows.h>

typedef void (__cdecl *AurPostStringFunc)(char*, int, int, float);
const DWORD AUR_POST_STRING_ADDR = 0x0044d490;

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
