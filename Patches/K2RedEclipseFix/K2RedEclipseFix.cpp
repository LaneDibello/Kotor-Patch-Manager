// K2RedEclipseFix.cpp -- Red Eclipse softlock fix (Nar Shaddaa, 301NAR). See README.md.
//
// One hook: CVirtualMachine::RunScript, after its prologue. Two area scripts that the game runs in
// 012EBO and 004EBO are swapped for the scripts those modules own (see redirect()).
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

// Replacement name handed back to RunScript. Static, so it outlives the hook call.
char g_swapText[16];
ExoString g_swapExo = {g_swapText, 0};

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

// The script the module's own area file runs in place of the Override-shadowed one, or nullptr.
// Lower-case names, exact match. 003EBO (the Hawk) is never changed.
const char* redirect(const char* module, const char* script) {
    if (strcmp(module, "012ebo") == 0 && strcmp(script, "k_003ebo_enter") == 0) return "k_012_enter";
    if (strcmp(module, "004ebo") == 0 && strcmp(script, "k_ebo_user") == 0) return "k_004_heart";
    return nullptr;
}

}  // namespace

extern "C" {

// Hook inside CVirtualMachine::RunScript after its prologue. Parameter: EBP (frame pointer). [EBP+8] is the
// CExoString* script name the caller passed in. The hook runs for every script, so the common path is one name read.
void __cdecl RedEclipseRunScript(int ebp) {
    uintptr_t frame = static_cast<uint32_t>(ebp);
    uint32_t scriptPtr = 0;
    char name[16];
    if (!readU32(frame + 8, &scriptPtr) || !readName(scriptPtr, name, sizeof name)) return;
    if (strcmp(name, "k_003ebo_enter") != 0 && strcmp(name, "k_ebo_user") != 0) return;

    char module[16];
    if (!moduleName(module, sizeof module)) return;
    const char* to = redirect(module, name);
    if (!to) return;

    // Point the script argument at the replacement name. RunScript reads this slot after the hook returns.
    size_t len = strlen(to);
    memcpy(g_swapText, to, len + 1);
    g_swapExo.len = static_cast<int>(len);
    *reinterpret_cast<uint32_t*>(frame + 8) = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_swapExo));
}

}  // extern "C"
