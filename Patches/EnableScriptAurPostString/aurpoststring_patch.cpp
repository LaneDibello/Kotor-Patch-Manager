// EnableScriptAurPostString Patch
// Re-enables the debug AurPostString function in KOTOR 1

#include <windows.h>
#include "GameAPI/GameVersion.h"
#include "Common.h"

typedef void (__cdecl *AurPostStringFunc)(char*, int, int, float);

static AurPostStringFunc g_aurPostString = nullptr;

extern "C" void __cdecl EnableAurPostString_Hook(char* string, int* x, int* y, float* life)
{
    if (!g_aurPostString) {
        debugLog("[EnableAurPostString] AurPostString function not available\n");
        return;
    }

    if (!x || !y || !life) {
        debugLog("[EnableAurPostString] Bad parameters for AurPostString: %s, %i, %i, %.2f", string, *y, *x, *life);
        return;
    }

    debugLog("[EnableAurPostString] Running AurPostString with params %s, %i, %i, %.2f", string, *y, *x, *life);
    g_aurPostString(string, *y, *x, *life);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (!GameVersion::Initialize()) {
                debugLog("[EnableAurPostString] ERROR: GameVersion::Initialize() failed\n");
                return FALSE;
            }

            g_aurPostString = (AurPostStringFunc)GameVersion::GetFunctionAddress("Global", "AurPostString");

            if (g_aurPostString) {
                debugLog("[EnableAurPostString] Successfully loaded AurPostString function\n");
            } else {
                debugLog("[EnableAurPostString] WARNING: AurPostString not available for this version\n");
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            GameVersion::Reset();
            g_aurPostString = nullptr;
            break;
    }
    return TRUE;
}
