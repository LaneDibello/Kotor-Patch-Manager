// EnableScriptAurPostString Patch
// Re-enables the debug AurPostString function in KOTOR 1
//
// MIGRATED TO NEW GAMEAPI SYSTEM:
// - Uses GameVersion manager for version-independent address lookup
// - Supports multiple game versions without recompilation

#include <windows.h>
#include "GameAPI/GameVersion.h"

// Function pointer type for AurPostString
typedef void (__cdecl *AurPostStringFunc)(char*, int, int, float);

// Cached function pointer (loaded once at initialization)
static AurPostStringFunc g_aurPostString = nullptr;

extern "C" void __cdecl EnableAurPostString_Hook(char* string, int x, int y, float life)
{
    // Check if function is available
    if (!g_aurPostString) {
        OutputDebugStringA("[EnableAurPostString] AurPostString function not available\n");
        return;
    }

    // Call the game function (note: x and y are swapped in the original)
    g_aurPostString(string, y, x, life);
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // Initialize GameVersion system (reads from KOTOR_VERSION_SHA env var and addresses.toml)
            if (!GameVersion::Initialize()) {
                OutputDebugStringA("[EnableAurPostString] ERROR: GameVersion::Initialize() failed\n");
                return FALSE;
            }

            // Load function address from address database
            g_aurPostString = (AurPostStringFunc)GameVersion::GetFunctionAddress("Global", "AurPostString");

            if (g_aurPostString) {
                OutputDebugStringA("[EnableAurPostString] Successfully loaded AurPostString function\n");
            } else {
                OutputDebugStringA("[EnableAurPostString] WARNING: AurPostString not available for this version\n");
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            // Cleanup
            GameVersion::Reset();
            g_aurPostString = nullptr;
            break;
    }
    return TRUE;
}
