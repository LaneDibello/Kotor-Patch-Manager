// POSIX backend for the platform seam (compiled into KotorPatcher.so).
//
// The target ELF is ET_EXEC, so it always loads at its link-time base with no
// ASLR slide. That is what lets the patcher use the absolute addresses from the
// config verbatim: there is no load bias to add before writing a hook.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // dladdr; g++ usually predefines this for C++.
#endif
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "platform.h"
#include "stub_placement.h"

namespace KotorPatcher {
namespace Platform {

    namespace {
        std::size_t PageSize() {
            long ps = sysconf(_SC_PAGESIZE);
            return ps > 0 ? static_cast<std::size_t>(ps) : 4096;
        }

        // Opens the diagnostics log once for the process, only when KPATCH_LOG names a
        // file. Steam redirects stderr away, so KPATCH_LOG=<path> in the launch options
        // is how a run gets captured. With it unset there is no log file, matching the
        // Windows backend, which leaves nothing on disk. Truncated per launch.
        std::FILE* OpenLogFile() {
            const char* path = std::getenv("KPATCH_LOG");
            return path ? std::fopen(path, "w") : nullptr;
        }
    }

    void Log(const char* message) {
        // Windows logs via OutputDebugString and leaves nothing on disk; stderr is the
        // Linux analogue. A file is opt-in through KPATCH_LOG (OpenLogFile), since Steam
        // redirects stderr away and would otherwise hide every line.
        std::fputs(message, stderr);

        static std::FILE* logFile = OpenLogFile();
        if (logFile) {
            std::fputs(message, logFile);
            std::fflush(logFile);
        }
    }

    // Mapped writable, never writable+executable: macOS refuses that mapping, and
    // these games are x86_64-only, so every Apple Silicon Mac runs them under
    // Rosetta and would hit the refusal. ProtectExec flips the block to executable.
    void* AllocExec(std::size_t size, std::uintptr_t nearAddress) {
        auto map = [size](void* hint) -> void* {
            void* mem = mmap(hint, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            return mem == MAP_FAILED ? nullptr : mem;
        };

        if (StubPlacement::kEveryAddressReaches || nearAddress == 0) {
            return map(nullptr);
        }

        for (std::size_t attempt = 0; attempt < StubPlacement::kSearchAttempts; ++attempt) {
            std::uintptr_t hint = 0;
            if (!StubPlacement::NextCandidate(nearAddress, attempt, hint)) break;

            // Without MAP_FIXED the hint is only a suggestion, and an occupied one is
            // answered with a mapping wherever the kernel pleased. MAP_FIXED is not an
            // option: it would silently unmap whatever already lives there.
            void* mem = map(reinterpret_cast<void*>(hint));
            if (!mem) continue;
            if (StubPlacement::NearEnough(nearAddress, reinterpret_cast<std::uintptr_t>(mem), size)) {
                return mem;
            }
            munmap(mem, size);
        }

        Platform::Log("[KotorPatcher] No free memory within a relative jump of the game\n");
        return nullptr;
    }

    bool ProtectExec(void* addr, std::size_t size) {
        if (mprotect(addr, size, PROT_READ | PROT_EXEC) != 0) {
            return false;
        }
        FlushICache(addr, size);
        return true;
    }

    void FreeExec(void* addr, std::size_t size) {
        if (addr) munmap(addr, size);
    }

    bool WriteCode(void* dest, const void* src, std::size_t len) {
        // mprotect requires a page-aligned address and acts on whole pages, so
        // round `dest` down to its page and grow the span to cover the write.
        const std::size_t ps = PageSize();
        auto addr = reinterpret_cast<std::uintptr_t>(dest);
        std::uintptr_t start = addr & ~(ps - 1);
        std::uintptr_t end = (addr + len + ps - 1) & ~(ps - 1);
        auto* page = reinterpret_cast<void*>(start);
        std::size_t span = end - start;

        if (mprotect(page, span, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            return false;
        }
        std::memcpy(dest, src, len);
        FlushICache(dest, len);
        // Game code is read+execute; the write was the only reason it was writable.
        mprotect(page, span, PROT_READ | PROT_EXEC);
        return true;
    }

    void FlushICache(void* addr, std::size_t len) {
        // x86 has a coherent instruction cache, so this compiles to nothing; it
        // stays a call so the intent is explicit and other arches stay correct.
        auto* begin = static_cast<char*>(addr);
        __builtin___clear_cache(begin, begin + len);
    }

    void* LoadModule(const char* path) {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }

    void* GetSymbol(void* module, const char* name) {
        return dlsym(module, name);
    }

    void UnloadModule(void* module) {
        if (module) dlclose(module);
    }

    std::string LastLoadError() {
        const char* err = dlerror();
        return err ? err : "unknown error";
    }

    std::string SelfModuleDir() {
        Dl_info info;
        // Resolve the path of the .so that contains this function.
        if (dladdr(reinterpret_cast<void*>(&SelfModuleDir), &info) == 0 || !info.dli_fname) {
            return {};
        }
        std::string path(info.dli_fname);
        std::size_t slash = path.find_last_of('/');
        return slash == std::string::npos ? std::string{} : path.substr(0, slash);
    }

    bool SetEnv(const char* name, const char* value) {
        return setenv(name, value, 1) == 0;
    }

} // namespace Platform
} // namespace KotorPatcher
