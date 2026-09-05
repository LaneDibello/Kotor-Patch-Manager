#include "wrapper_x86_64.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"
#include <cstdio>

#include "check.h"
#include <cstring>
#include <cstdint>
using namespace KotorPatcher;

extern "C" {
    void*    g_hookEntry;
    uint64_t g_gameRsp;          // RSP at the hook site
    uint64_t g_redZone[4];       // what the game left below RSP
    uint64_t g_redZoneAfter[4];
    uint64_t g_rax, g_flags;
    int      g_landedConsumed;
    uint64_t g_stackArg;
    int      g_probeReturn;
    uint64_t g_seenArg0;
    void kick(void);
    void land_resume(void);
    void land_consumed(void);

    int probe(uint64_t a) {
        g_seenArg0 = a;
        return g_probeReturn;
    }
}

// The game seeds its red zone, records RSP, then falls into the hook. Whichever
// landing pad the wrapper picks records what came back and unwinds.
asm(R"(
.text
.globl kick
kick:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    movq %rsp, g_gameRsp(%rip)
    /* Seed four slots of the red zone, the 128 bytes below RSP a leaf function
       may use without adjusting RSP. */
    lea  g_redZone(%rip), %rax
    movq   0(%rax), %rdx
    movq %rdx,   -8(%rsp)
    movq   8(%rax), %rdx
    movq %rdx,  -32(%rsp)
    movq  16(%rax), %rdx
    movq %rdx,  -64(%rsp)
    movq  24(%rax), %rdx
    movq %rdx, -128(%rsp)
    movq $0x1234567800000000, %rax
    cmpq %rax, %rax                 /* ZF=1, which must still be set at the landing pad */
    jmp  *g_hookEntry(%rip)

.globl land_resume
land_resume:
    movl $0, g_landedConsumed(%rip)
    jmp  common_land
.globl land_consumed
land_consumed:
    movl $1, g_landedConsumed(%rip)
common_land:
    /* Snapshot before anything that writes below RSP or touches the flags.
       Only MOV is used here, and MOV sets no flags. */
    movq %rax, g_rax(%rip)
    lea  g_redZoneAfter(%rip), %rax
    movq   -8(%rsp), %rdx
    movq %rdx,   0(%rax)
    movq  -32(%rsp), %rdx
    movq %rdx,   8(%rax)
    movq  -64(%rsp), %rdx
    movq %rdx,  16(%rax)
    movq -128(%rsp), %rdx
    movq %rdx,  24(%rax)
    pushfq
    popq g_flags(%rip)
    pop  %r15
    pop  %r14
    pop  %r13
    pop  %r12
    pop  %rbp
    pop  %rbx
    ret
)");


// Builds a hook site that jumps into a freshly generated wrapper, with the resume
// point wired to land_resume.
static bool build(Wrappers::WrapperGenerator_x86_64& gen, Wrappers::WrapperConfig config,
                  const uint8_t* stolen, std::size_t stolenLen) {
    auto* site = static_cast<uint8_t*>(
        Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&probe)));
    if (!site) return false;
    g_hookEntry = site;
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + stolenLen);
    config.patchFunction = reinterpret_cast<void*>(&probe);

    void* wrapper = gen.GenerateWrapper(config);
    if (!wrapper) return false;

    int32_t rel = 0;
    if (!Trampoline::ComputeRel32((uintptr_t)site, (uintptr_t)wrapper, rel)) return false;
    site[0] = 0xE9; std::memcpy(site + 1, &rel, 4);
    std::memset(site + 5, 0x90, stolenLen - 5);
    uint8_t* resume = site + stolenLen;
    if (!Trampoline::ComputeRel32((uintptr_t)resume, (uintptr_t)&land_resume, rel)) return false;
    resume[0] = 0xE9; std::memcpy(resume + 1, &rel, 4);
    return Platform::ProtectExec(site, 4096);
}

int main() {
    for (int i = 0; i < 4; ++i) g_redZone[i] = 0xDEC0DE0000ull + i;
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };   // five NOPs: no side effect
    Wrappers::WrapperGenerator_x86_64 gen;

    printf("  red zone and flags survive a hook\n");
    {
        Wrappers::WrapperConfig c;
        c.parameters = { { "rsp+0", ParameterType::POINTER } };
        g_probeReturn = 0;
        if (!build(gen, c, stolen, sizeof(stolen))) { printf("   build failed\n"); return 1; }
        std::memset(g_redZoneAfter, 0, sizeof(g_redZoneAfter));
        kick();
        kptest::Check("red zone untouched", std::memcmp(g_redZone, g_redZoneAfter, sizeof(g_redZone)) == 0);
        kptest::Check("ZF from before the hook survives", (g_flags & 0x40) != 0);
        char d[64]; snprintf(d, sizeof(d), "(arg %#lx vs rsp %#lx)", g_seenArg0, g_gameRsp);
        kptest::Check("rsp+0 gives the game's stack pointer", g_seenArg0 == g_gameRsp, d);
    }

    printf("  consumed-exit taken when the handler returns non-zero\n");
    {
        Wrappers::WrapperConfig c;
        c.excludeFromRestore = { "eax" };      // the Windows spelling, on purpose
        c.consumedExitAddress = reinterpret_cast<uintptr_t>(&land_consumed);
        g_probeReturn = 1;
        if (!build(gen, c, stolen, sizeof(stolen))) { printf("   build failed\n"); return 1; }
        g_landedConsumed = -1;
        kick();
        kptest::Check("landed on the consumed target", g_landedConsumed == 1);
        kptest::Check("excluded eax kept the handler's value", g_rax == 1);
    }

    printf("  fall-through when the handler returns zero\n");
    {
        Wrappers::WrapperConfig c;
        c.excludeFromRestore = { "eax" };
        c.consumedExitAddress = reinterpret_cast<uintptr_t>(&land_consumed);
        g_probeReturn = 0;
        if (!build(gen, c, stolen, sizeof(stolen))) { printf("   build failed\n"); return 1; }
        g_landedConsumed = -1;
        std::memset(g_redZoneAfter, 0, sizeof(g_redZoneAfter));
        kick();
        kptest::Check("landed on the resume point", g_landedConsumed == 0);
        kptest::Check("red zone untouched by the exit test",
           std::memcmp(g_redZone, g_redZoneAfter, sizeof(g_redZone)) == 0);
    }
    return kptest::Report();
}
