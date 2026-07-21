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

namespace KotorPatcher {
namespace Platform {

    namespace {
        std::size_t PageSize() {
            long ps = sysconf(_SC_PAGESIZE);
            return ps > 0 ? static_cast<std::size_t>(ps) : 4096;
        }
    }

    void Log(const char* message) {
        // Windows routes diagnostics to the debugger; the Linux analogue is stderr.
        // A log-to-file channel returns with the Steam delivery, where stderr is
        // swallowed.
        std::fputs(message, stderr);
    }

    void* AllocExec(std::size_t size) {
        void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return mem == MAP_FAILED ? nullptr : mem;
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
