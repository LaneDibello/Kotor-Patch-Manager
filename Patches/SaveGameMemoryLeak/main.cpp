#include "Common.h"
#include "GameAPI/GameVersion.h"

typedef void(__cdecl* GameFreeFunc)(void*);
static GameFreeFunc g_gameFree = nullptr;

extern "C" void __cdecl FreeSaveBuffer(void* buffer) {
    if (!buffer) {
        return;
    }

    if (!g_gameFree) {
        OutputDebugStringA("[SaveGameMemoryLeak] Game free unavailable; leaking save buffer\n");
        return;
    }

    g_gameFree(buffer);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            OutputDebugStringA("[SaveGameMemoryLeak] ERROR: GameVersion::Initialize() failed\n");
            return FALSE;
        }

        if (GameVersion::HasFunction("Global", "free")) {
            g_gameFree = (GameFreeFunc)GameVersion::GetFunctionAddress("Global", "free");
            OutputDebugStringA("[SaveGameMemoryLeak] Resolved Global::free\n");
        }
        else {
            OutputDebugStringA("[SaveGameMemoryLeak] WARNING: Global::free not recorded for this version\n");
        }
        break;

    case DLL_PROCESS_DETACH:
        g_gameFree = nullptr;
        GameVersion::Reset();
        break;
    }
    return TRUE;
}
