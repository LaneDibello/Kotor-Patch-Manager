// A parameter's declared type decides how much of the saved register reaches the patch
// function.
//
// This matters more here than on i386, because the saved copy is 64 bits wide and the
// load used to be 64 bits regardless of type: a hook declaring "int" handed over a value
// whose upper half was whatever else the game had in that register. POINTER is the only
// type that should still take all 64.
//
// The fifth argument is deliberate. System V puts it in R8, and naming a register above
// 7 needs a REX prefix that the first four never exercise.
#include "wrapper_x86_64.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"

#include "check.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace KotorPatcher;

// Every byte distinct, and both halves non-zero, so a load of the wrong width cannot
// coincidentally match.
static const uint64_t kSeed = 0xDEADBEEFCAFEF00DULL;

extern "C" {
    void* g_hookEntry;
    void  kick(void);
    void  finish(void);

    uint64_t g_arg[5];
    int      g_calls;

    void probe(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
        g_arg[0] = a; g_arg[1] = b; g_arg[2] = c; g_arg[3] = d; g_arg[4] = e;
        ++g_calls;
    }
}

asm(R"(
.text
.globl kick
kick:
    pushq %rbx
    pushq %r15
    movabsq $0xDEADBEEFCAFEF00D, %r15
    jmp  *g_hookEntry(%rip)

.globl finish
finish:
    popq %r15
    popq %rbx
    ret
)");

namespace {

void Expect(const char* what, uint64_t got, uint64_t want) {
    char detail[80];
    std::snprintf(detail, sizeof(detail), "(got %#llx want %#llx)",
                  (unsigned long long)got, (unsigned long long)want);
    kptest::Check(what, got == want, detail);
}

}  // namespace

int main() {
    // Five bytes, which is exactly the JMP that replaces them.
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

    auto* site = static_cast<uint8_t*>(
        Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&probe)));
    if (!site) { std::printf("alloc failed\n"); return 1; }
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86_64 gen;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&probe);
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = {
        { "r15", ParameterType::BYTE },
        { "r15", ParameterType::SHORT },
        { "r15", ParameterType::UINT },
        { "r15", ParameterType::POINTER },
        { "r15", ParameterType::BYTE },
    };

    void* wrapper = gen.GenerateWrapper(config);
    if (!wrapper) { std::printf("generation failed\n"); return 1; }

    int32_t rel = 0;
    if (!Trampoline::ComputeRel32(reinterpret_cast<uintptr_t>(site),
                                  reinterpret_cast<uintptr_t>(wrapper), rel)) {
        std::printf("wrapper is out of reach of the hook site\n"); return 1;
    }
    site[0] = 0xE9; std::memcpy(site + 1, &rel, 4);
    uint8_t* resume = site + sizeof(stolen);
    if (!Trampoline::ComputeRel32(reinterpret_cast<uintptr_t>(resume),
                                  reinterpret_cast<uintptr_t>(&finish), rel)) {
        std::printf("finish is out of reach of the resume point\n"); return 1;
    }
    resume[0] = 0xE9; std::memcpy(resume + 1, &rel, 4);
    if (!Platform::ProtectExec(site, 4096)) { std::printf("could not seal the site\n"); return 1; }

    kick();

    kptest::Check("the patch function ran once", g_calls == 1);
    Expect("byte reads the low 8 bits",        g_arg[0], kSeed & 0xFF);
    Expect("short reads the low 16 bits",      g_arg[1], kSeed & 0xFFFF);
    Expect("uint reads the low 32 bits",       g_arg[2], kSeed & 0xFFFFFFFF);
    Expect("pointer keeps all 64 bits",        g_arg[3], kSeed);
    Expect("byte in R8 needs a REX prefix",    g_arg[4], kSeed & 0xFF);

    Wrappers::WrapperGenerator_x86_64 narrow;
    Wrappers::WrapperConfig stackConfig = config;
    stackConfig.parameters = { { "rsp+8", ParameterType::SHORT } };
    kptest::Check("a narrow type on a stack source is refused",
                  narrow.GenerateWrapper(stackConfig) == nullptr);

    return kptest::Report();
}
