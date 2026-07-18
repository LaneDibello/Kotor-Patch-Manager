// KProxy DLL: loads KotorPatcher via the game's own PE loader.
//
// KOTOR 1 and 2 statically import binkw32.dll, and Wine/Proton has no builtin for
// it, so the loader picks up this DLL from the game folder during startup, before
// the entry point runs. Whoever launches the game (Steam, a shortcut, the manager),
// our code is already inside the process. bink_forwards.def forwards every Bink
// export to binkw32Hooked.dll (the real Bink, renamed at install) so video still
// plays; the one thing added is loading KotorPatcher.dll.
//
// On Windows the manager injects KotorPatcher.dll directly.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        // KotorPatcher.dll sits next to us in the game folder. The exe's own
        // directory is first in the DLL search path, so the bare name resolves.
        if (!LoadLibraryA("KotorPatcher.dll")) {
            OutputDebugStringA("[KProxy] failed to load KotorPatcher.dll\n");
        }
    }
    return TRUE;
}
