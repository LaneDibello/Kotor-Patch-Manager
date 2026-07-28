#pragma once
#include <cctype>
#include <cstddef>
#include <string>

// Platform seam for the patcher engine.
//
// The engine core (config parsing, patch orchestration, trampoline layout, and
// the x86 wrapper byte emitter) is identical on Windows and Linux. Only these
// primitives differ, so the core calls through this interface and exactly one
// backend is linked: platform_win32.cpp for KotorPatcher.dll, platform_posix.cpp
// for KotorPatcher.so. Both targets are 32-bit (i686/i386), so a code address fits
// a uint32_t throughout the core.

namespace KotorPatcher {
namespace Platform {

    // Write one diagnostic line. Windows routes it to the debugger
    // (OutputDebugStringA); Linux writes it to stderr.
    void Log(const char* message);

    // Allocate `size` bytes of read/write/execute memory for generated stubs
    // (DETOUR wrappers, REPLACE code blocks), or nullptr on failure.
    void* AllocExec(std::size_t size);

    // Release memory returned by AllocExec.
    void FreeExec(void* addr, std::size_t size);

    // Overwrite `len` bytes of the target's own code at `dest` with `src`,
    // handling the unprotect -> write -> icache-flush -> reprotect sequence.
    // Returns false if the page could not be made writable. `dest` is executable
    // game code, so it is left readable+executable afterwards.
    bool WriteCode(void* dest, const void* src, std::size_t len);

    // Make the CPU observe instructions just written to `addr`..`addr+len`.
    // A no-op on x86 (coherent instruction cache); real where it is needed.
    void FlushICache(void* addr, std::size_t len);

    // Load a patch module (LoadLibraryA / dlopen), resolve an exported symbol
    // (GetProcAddress / dlsym), and release it (FreeLibrary / dlclose).
    void* LoadModule(const char* path);
    void* GetSymbol(void* module, const char* name);
    void  UnloadModule(void* module);

    // Description of why the most recent LoadModule failed (FormatMessage /
    // dlerror). Call it immediately after a null LoadModule return, before any
    // other module call clobbers the error state.
    std::string LastLoadError();

    // Directory holding this patcher module, with no trailing separator, or an
    // empty string on failure. The patch config sits beside it.
    std::string SelfModuleDir();

    // Export an environment variable for patch modules to read. Returns success.
    bool SetEnv(const char* name, const char* value);

} // namespace Platform

// Case-insensitive ASCII compare, a non-reserved stand-in for _stricmp. Returns
// <0, 0, or >0 like strcmp, so 0 means the strings are equal.
inline int StrICmp(const char* a, const char* b) {
    while (*a && *b) {
        int ca = std::tolower(static_cast<unsigned char>(*a));
        int cb = std::tolower(static_cast<unsigned char>(*b));
        if (ca != cb) return ca - cb;
        ++a;
        ++b;
    }
    return std::tolower(static_cast<unsigned char>(*a)) -
           std::tolower(static_cast<unsigned char>(*b));
}

} // namespace KotorPatcher
