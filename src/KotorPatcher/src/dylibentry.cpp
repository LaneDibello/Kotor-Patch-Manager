// KotorPatcher.dylib: the macOS entry point for the patcher engine.
//
// The Aspyr macOS builds are x86_64 Mach-O, so neither KotorPatcher.dll nor the
// i386 KotorPatcher.so can load into them. This dylib is named in the game
// executable's own LC_LOAD_DYLIB list, so dyld maps it at startup, before the
// game's entry point runs. Whoever launches the game, our code is already inside
// the process. DYLD_INSERT_LIBRARIES reaches the same place without editing the
// executable, which is how a build gets tried before it is deployed.
//
// The constructor and destructor are the macOS counterparts to dllmain.cpp's
// DLL_PROCESS_ATTACH/DETACH, and are written exactly as soentry.cpp writes them:
// clang puts a constructor in __DATA,__mod_init_func, which dyld runs for the
// same reasons the ELF loader runs an .init_array entry.

#include "patcher.h"
#include "platform.h"

namespace {

// Runs when dyld maps this library, before the game's main().
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
