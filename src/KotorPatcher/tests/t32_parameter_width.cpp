// A parameter's declared type decides how much of the saved register reaches the patch
// function.
//
// BYTE and SHORT did nothing for most of this generator's life: every type emitted the
// same 32-bit MOV, so a hook declaring "byte" was handed the whole register and a patch
// function taking a char read three bytes of whatever else was in there. Reading one
// register four ways in a single call is the cheapest way to show the widths are real
// and that they all come off the same saved copy.
#include "wrapper_x86.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"

#include "check.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace KotorPatcher;

// Every byte distinct, so a wrong width cannot coincide with a right one.
static const uint32_t kSeed = 0xDEADBEEF;

extern "C" {
    void* g_hookEntry;
    void  kick(void);
    void  finish(void);

    uint32_t g_arg[4];
    int      g_calls;

    // cdecl, matching what the wrapper pushes.
    void probe(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        g_arg[0] = a; g_arg[1] = b; g_arg[2] = c; g_arg[3] = d;
        ++g_calls;
    }
}

asm(R"(
.text
.globl kick
kick:
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp
    movl $0xDEADBEEF, %eax
    jmp  *g_hookEntry

.globl finish
finish:
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");

namespace {

void Expect(const char* what, uint32_t got, uint32_t want) {
    char detail[64];
    std::snprintf(detail, sizeof(detail), "(got %#x want %#x)", got, want);
    kptest::Check(what, got == want, detail);
}

}  // namespace

int main() {
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

    auto* site = static_cast<uint8_t*>(
        Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&probe)));
    if (!site) { std::printf("alloc failed\n"); return 1; }
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86 gen;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&probe);
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = {
        { "eax", ParameterType::BYTE },
        { "eax", ParameterType::SHORT },
        { "eax", ParameterType::UINT },
        { "eax", ParameterType::POINTER },
    };

    void* wrapper = gen.GenerateWrapper(config);
    if (!wrapper) { std::printf("generation failed\n"); return 1; }

    int32_t rel = 0;
    Trampoline::ComputeRel32((uintptr_t)site, (uintptr_t)wrapper, rel);
    site[0] = 0xE9; std::memcpy(site + 1, &rel, 4);
    uint8_t* resume = site + sizeof(stolen);
    Trampoline::ComputeRel32((uintptr_t)resume, (uintptr_t)&finish, rel);
    resume[0] = 0xE9; std::memcpy(resume + 1, &rel, 4);
    Platform::ProtectExec(site, 4096);

    kick();

    kptest::Check("the patch function ran once", g_calls == 1);
    Expect("byte reads the low 8 bits",     g_arg[0], kSeed & 0xFF);
    Expect("short reads the low 16 bits",   g_arg[1], kSeed & 0xFFFF);
    Expect("uint reads the whole register", g_arg[2], kSeed);
    // Everything is 32 bits here, so pointer and uint are the same load. Stated as a
    // case of its own because on x86_64 they are not.
    Expect("pointer reads the whole register", g_arg[3], kSeed);

    Wrappers::WrapperGenerator_x86 narrow;
    Wrappers::WrapperConfig stackConfig = config;
    stackConfig.parameters = { { "esp+4", ParameterType::BYTE } };
    kptest::Check("a narrow type on a stack source is refused",
                  narrow.GenerateWrapper(stackConfig) == nullptr);

    return kptest::Report();
}
