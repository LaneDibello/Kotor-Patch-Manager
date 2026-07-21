// KotorPatcher.so: the native Linux entry point for the patcher engine.
//
// KOTOR 2's native Linux build (Aspyr) is an i386 ELF, so the Windows
// KotorPatcher.dll cannot load into it. This shared object is added to the game's
// DT_NEEDED list, so the dynamic loader maps it at startup, before the game's
// entry point runs. Whoever launches the game (Steam, a shortcut, the manager),
// our code is already inside the process.
//
// The constructor and destructor are the Linux counterparts to dllmain.cpp's
// DLL_PROCESS_ATTACH/DETACH: the constructor hands off to the shared engine (which
// reads patch_config.toml beside this module and applies the hooks); the
// destructor tears it down.

#include "patcher.h"
#include "platform.h"

namespace {

// Runs when the dynamic loader maps this library, before the game's main().
__attribute__((constructor))
void kpatchInit() {
    if (!KotorPatcher::InitializePatcher()) {
        KotorPatcher::Platform::Log("[KotorPatcher] ERROR: patcher initialization failed\n");
    } else {
        KotorPatcher::Platform::Log("[KotorPatcher] patcher initialized\n");
    }
}

// Runs when the library is unloaded at process exit.
__attribute__((destructor))
void kpatchFini() {
    KotorPatcher::CleanupPatcher();
}

} // namespace
