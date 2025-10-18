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

// This function will be called by the wrapper system
// It receives the parameters extracted from the game state
// NOTE: Exported via exports.def file to avoid hot-patch stub
extern "C" void __cdecl EnableAurPostString_Hook()
{
    // At this point in the hook (0x005cb41c):
    // EAX = char* string
    // [ESP] = int x
    // [ESP+4] = int y
    // [ESP+8] = float life

    char* string;
    int x, y;
    float life;

    // Extract parameters from registers and stack
    __asm {
        // Save EAX (contains string pointer)
        mov string, eax

        // Get x from [ESP]
        mov eax, dword ptr [esp]
        mov x, eax

        // Get y from [ESP+4]
        mov eax, dword ptr [esp+4]
        mov y, eax

        // Get life from [ESP+8]
        mov eax, dword ptr [esp+8]
        mov life, eax
    }

    // Call the actual AurPostString function
    // Note: __cdecl means parameters are pushed right to left
    AurPostStringFunc aurPostString = (AurPostStringFunc)AUR_POST_STRING_ADDR;
    aurPostString(string, y, x, life);

    // Return - wrapper system will handle register restoration
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
