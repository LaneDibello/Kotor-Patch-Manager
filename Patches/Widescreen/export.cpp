#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/GameVersion.h"

extern "C" void* __cdecl getResolutionString(DWORD width, DWORD height) {
    char buffer[16];
    sprintf_s(buffer, sizeof(buffer), "%dx%d");
    CExoString str(buffer);
    return str.GetPtr();
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[Widescreen] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[Widescreen] Attached");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}