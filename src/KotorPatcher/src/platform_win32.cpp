// Windows backend for the platform seam (compiled into KotorPatcher.dll).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>
#include <string>

#include "platform.h"

namespace KotorPatcher {
namespace Platform {

    void Log(const char* message) {
        OutputDebugStringA(message);
    }

    void* AllocExec(std::size_t size) {
        return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }

    void FreeExec(void* addr, std::size_t /*size*/) {
        if (addr) VirtualFree(addr, 0, MEM_RELEASE);
    }

    bool WriteCode(void* dest, const void* src, std::size_t len) {
        DWORD oldProtect = 0;
        if (!VirtualProtect(dest, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        std::memcpy(dest, src, len);
        FlushInstructionCache(GetCurrentProcess(), dest, len);
        DWORD ignored = 0;
        // Restoring the original protection can fail harmlessly; the bytes are
        // already written and the CPU has seen them.
        VirtualProtect(dest, len, oldProtect, &ignored);
        return true;
    }

    void FlushICache(void* addr, std::size_t len) {
        FlushInstructionCache(GetCurrentProcess(), addr, len);
    }

    void* LoadModule(const char* path) {
        return LoadLibraryA(path);
    }

    void* GetSymbol(void* module, const char* name) {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), name));
    }

    void UnloadModule(void* module) {
        if (module) FreeLibrary(static_cast<HMODULE>(module));
    }

    std::string LastLoadError() {
        DWORD code = GetLastError();
        LPSTR buffer = nullptr;
        DWORD len = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
        std::string msg = "error " + std::to_string(code);
        if (len && buffer) {
            msg += ": ";
            msg.append(buffer, len);
        }
        if (buffer) LocalFree(buffer);
        // FormatMessage appends a trailing CRLF; drop it so the log stays on one line.
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }
        return msg;
    }

    std::string SelfModuleDir() {
        char path[MAX_PATH];
        HMODULE self = GetModuleHandleA("KotorPatcher.dll");
        if (!self || GetModuleFileNameA(self, path, MAX_PATH) == 0) {
            return {};
        }
        std::string dir(path);
        std::size_t slash = dir.find_last_of("\\/");
        return slash == std::string::npos ? std::string{} : dir.substr(0, slash);
    }

    bool SetEnv(const char* name, const char* value) {
        return SetEnvironmentVariableA(name, value) != 0;
    }

} // namespace Platform
} // namespace KotorPatcher
