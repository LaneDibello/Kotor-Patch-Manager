// K2EbonHawkAreaScriptFix.cpp -- Red Eclipse softlock fix (Nar Shaddaa, 301NAR). See README.md.
//
// One hook: CVirtualMachine::RunScript, after its prologue. In module 012EBO the area script
// k_003ebo_enter (pulled in by a third-party Override/003ebo.are) is swapped for k_012_enter.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>

namespace {

// Root object of the game, kept at this address. Its chain to the active module is
// root +4 = client app, client +4 = internal app, internal +0x18 = module, module +0x38 = module name.
constexpr uintptr_t kAppGlobal = 0x00A1B4A4;

// Engine string: { char* text; int len }.
struct ExoString {
    char* text;
    int len;
};

constexpr char kShadowScript[] = "k_003ebo_enter";  // run by the Override-shadowed area
constexpr char kOwnScript[] = "k_012_enter";        // the script 012EBO owns
constexpr char kModule[] = "012ebo";

// Replacement name handed back to RunScript. Static, so it outlives the hook call. It is only ever written
// with kOwnScript, so a nested RunScript call that swaps again rewrites the same bytes.
char g_swapText[sizeof kOwnScript];
ExoString g_swapExo = {g_swapText, static_cast<int>(sizeof kOwnScript) - 1};

bool readable(uintptr_t addr, size_t n) {
    return addr != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(addr), n);
}

bool readU32(uintptr_t addr, uint32_t* out) {
    if (!readable(addr, sizeof(uint32_t))) return false;
    *out = *reinterpret_cast<const uint32_t*>(addr);
    return true;
}

// Copies an engine string into out, lower case and NUL-terminated. Fails for empty or too long names.
bool readName(uintptr_t exo, char* out, size_t cap) {
    out[0] = '\0';
    if (!readable(exo, sizeof(ExoString))) return false;
    const ExoString* s = reinterpret_cast<const ExoString*>(exo);
    if (!s->text || s->len <= 0 || static_cast<size_t>(s->len) >= cap) return false;
    if (!readable(reinterpret_cast<uintptr_t>(s->text), static_cast<size_t>(s->len))) return false;
    memcpy(out, s->text, static_cast<size_t>(s->len));
    out[s->len] = '\0';
    if (strlen(out) != static_cast<size_t>(s->len)) return false;  // embedded NUL: not a plain name
    for (char* p = out; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') *p = static_cast<char>(*p + 32);
    }
    return true;
}

// Name of the module the game is running now, lower case.
bool moduleName(char* out, size_t cap) {
    uint32_t app = 0, client = 0, internal = 0, module = 0;
    if (!readU32(kAppGlobal, &app) || !readU32(static_cast<uintptr_t>(app) + 4, &client) ||
        !readU32(static_cast<uintptr_t>(client) + 4, &internal) ||
        !readU32(static_cast<uintptr_t>(internal) + 0x18, &module)) {
        return false;
    }
    return readName(static_cast<uintptr_t>(module) + 0x38, out, cap);
}

}  // namespace

extern "C" {

// Hook inside CVirtualMachine::RunScript after its prologue. Parameter: EBP (frame pointer). [EBP+8] is the
// CExoString* script name the caller passed in. The hook runs for every script, so the common path is one name read.
void __cdecl EbonHawkRunScript(int ebp) {
    uintptr_t frame = static_cast<uint32_t>(ebp);
    uint32_t scriptPtr = 0;
    if (!readU32(frame + 8, &scriptPtr) || !readable(scriptPtr, sizeof(ExoString))) return;
    // Fast path for the many other scripts: only the length is read unless it matches.
    if (reinterpret_cast<const ExoString*>(scriptPtr)->len != static_cast<int>(sizeof kShadowScript) - 1) return;

    char name[sizeof kShadowScript + 1];
    if (!readName(scriptPtr, name, sizeof name) || strcmp(name, kShadowScript) != 0) return;

    char module[16];
    if (!moduleName(module, sizeof module) || strcmp(module, kModule) != 0) return;

    // Point the script argument at the replacement name. RunScript reads this slot after the hook returns.
    memcpy(g_swapText, kOwnScript, sizeof kOwnScript);
    *reinterpret_cast<uint32_t*>(frame + 8) = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_swapExo));
}

}  // extern "C"
