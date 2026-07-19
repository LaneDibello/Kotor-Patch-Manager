// libkpatch.so: the native Linux loader for the patcher runtime.
//
// KOTOR 2's native Linux build (Aspyr) is an i386 ELF, so the Windows
// KotorPatcher.dll and its byte-address hooks do not apply to it. Instead this
// shared object is added to the game's DT_NEEDED list (see the manager's use of
// LibObjectFile ElfDynamicEditing.AddNeededLibrary), so the dynamic loader maps
// it at startup, before the game's entry point runs. Whoever launches the game
// (Steam, a shortcut, the manager), our code is already inside the process.
//
// The constructor below is our in-process entry point. Phase A only proves the
// mechanism; later phases read the patch config from here and apply hooks.

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace {

// A game's stderr is usually swallowed under Steam, so mirror the load marker to a
// file. KPATCH_LOG overrides the path; /tmp is the fallback since the game directory
// is not reliably writable.
void logLoaded() {
    const char* path = std::getenv("KPATCH_LOG");
    if (!path) path = "/tmp/libkpatch.log";
    if (FILE* log = std::fopen(path, "a")) {
        std::fprintf(log, "[libkpatch] loaded into pid %d\n", getpid());
        std::fclose(log);
    }
    std::fprintf(stderr, "[libkpatch] loaded into pid %d\n", getpid());
}

// Runs when the dynamic loader maps this library, before the game's main().
__attribute__((constructor))
void kpatchInit() {
    logLoaded();
}

} // namespace
