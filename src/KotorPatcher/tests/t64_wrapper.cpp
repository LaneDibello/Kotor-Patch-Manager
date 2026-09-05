// Executes a wrapper the x86_64 generator produced, with a synthetic hook site.
// The generated code is plain x86_64, so it runs here exactly as it would on macOS.
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
    uint64_t g_seed[16];        // values the "game" holds at the hook site
    uint64_t g_captured[16];    // what survives to the resume point
    uint64_t g_capturedFlags;
    uint64_t g_seedXmm[32];     // 16 x 2 qwords
    uint64_t g_capturedXmm[32];
    void*    g_hookEntry;
    void     kick(void);
    void     capture(void);

    // The patch function. Records what the wrapper handed it.
    uint64_t g_arg[3];
    int g_calls;
    int probe(uint64_t a, uint64_t b, uint64_t c) {
        g_arg[0] = a; g_arg[1] = b; g_arg[2] = c;
        ++g_calls;
        // Clobber everything a System V callee is allowed to clobber, to prove the
        // wrapper's save and restore is doing real work.
        asm volatile(
            "movq $0xDEADBEEF11111111, %%rcx\n\t"
            "movq $0xDEADBEEF22222222, %%rdx\n\t"
            "movq $0xDEADBEEF33333333, %%rsi\n\t"
            "movq $0xDEADBEEF44444444, %%rdi\n\t"
            "movq $0xDEADBEEF55555555, %%r8\n\t"
            "movq $0xDEADBEEF66666666, %%r9\n\t"
            "movq $0xDEADBEEF77777777, %%r10\n\t"
            "movq $0xDEADBEEF88888888, %%r11\n\t"
            "pxor %%xmm0, %%xmm0\n\t"
            "pxor %%xmm3, %%xmm3\n\t"
            "pxor %%xmm7, %%xmm7\n\t"
            "pxor %%xmm15, %%xmm15\n\t"
            ::: "rcx","rdx","rsi","rdi","r8","r9","r10","r11",
                "xmm0","xmm3","xmm7","xmm15");
        return 0;   // "not consumed"
    }
}

// The game side. kick() seeds every register, jumps to the hook site, and the
// wrapper's resume point lands on capture(), which records what came back.
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
    lea  g_seedXmm(%rip), %rax
    movups   0(%rax), %xmm0
    movups  16(%rax), %xmm1
    movups 112(%rax), %xmm7
    movups 240(%rax), %xmm15
    lea  g_seed(%rip), %rax
    movq  8(%rax), %rcx
    movq 16(%rax), %rdx
    movq 24(%rax), %rbx
    movq 40(%rax), %rbp
    movq 48(%rax), %rsi
    movq 56(%rax), %rdi
    movq 64(%rax), %r8
    movq 72(%rax), %r9
    movq 80(%rax), %r10
    movq 88(%rax), %r11
    movq 96(%rax), %r12
    movq 104(%rax), %r13
    movq 112(%rax), %r14
    movq 120(%rax), %r15
    movq  0(%rax), %rax
    jmp  *g_hookEntry(%rip)

.globl capture
capture:
    pushfq
    pop  g_capturedFlags(%rip)
    movq %rax, g_captured+0(%rip)
    movq %rcx, g_captured+8(%rip)
    movq %rdx, g_captured+16(%rip)
    movq %rbx, g_captured+24(%rip)
    movq %rbp, g_captured+40(%rip)
    movq %rsi, g_captured+48(%rip)
    movq %rdi, g_captured+56(%rip)
    movq %r8,  g_captured+64(%rip)
    movq %r9,  g_captured+72(%rip)
    movq %r10, g_captured+80(%rip)
    movq %r11, g_captured+88(%rip)
    movq %r12, g_captured+96(%rip)
    movq %r13, g_captured+104(%rip)
    movq %r14, g_captured+112(%rip)
    movq %r15, g_captured+120(%rip)
    lea  g_capturedXmm(%rip), %rax
    movups %xmm0,   0(%rax)
    movups %xmm1,  16(%rax)
    movups %xmm7, 112(%rax)
    movups %xmm15,240(%rax)
    pop  %r15
    pop  %r14
    pop  %r13
    pop  %r12
    pop  %rbp
    pop  %rbx
    ret
)");


static void expect(const char* what, uint64_t got, uint64_t want) {
    char detail[64];
    std::snprintf(detail, sizeof(detail), "(%#018lx)", (unsigned long)got);
    kptest::Check(what, got == want, got == want ? detail : "");
    if (got != want) std::printf("      got %#018lx want %#018lx\n",
                                 (unsigned long)got, (unsigned long)want);
}

int main() {
    const char* names[16] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                             "r8","r9","r10","r11","r12","r13","r14","r15"};
    for (int i = 0; i < 16; ++i) g_seed[i] = 0xA5A50000ull + i;
    for (int i = 0; i < 32; ++i) g_seedXmm[i] = 0x5EED0000ull + i;

    // originalBytes: "add rax, 7" then a NOP, so the stolen bytes leave a mark we
    // can see at the resume point, and the cut is the 5 bytes a JMP needs.
    const uint8_t stolen[] = { 0x48, 0x83, 0xC0, 0x07, 0x90 };

    // The hook site, placed near the patch function so every rel32 has room.
    auto* site = static_cast<uint8_t*>(
        Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&probe)));
    if (!site) { printf("could not allocate the hook site\n"); return 1; }
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86_64 gen;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&probe);
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = {
        { "rax", ParameterType::UINT },
        { "rcx", ParameterType::UINT },
        { "rsi", ParameterType::POINTER },
    };
    config.preserveRegisters = true;
    config.preserveFlags = true;

    void* wrapper = gen.GenerateWrapper(config);
    if (!wrapper) { printf("wrapper generation FAILED\n"); return 1; }

    // Stand in for what the patcher would write: jump into the wrapper, then let the
    // resume point (site + stolen.size()) hand control to capture().
    int32_t rel = 0;
    if (!Trampoline::ComputeRel32(reinterpret_cast<uintptr_t>(site),
                                  reinterpret_cast<uintptr_t>(wrapper), rel)) {
        printf("wrapper is out of reach of the hook site\n"); return 1;
    }
    site[0] = 0xE9; std::memcpy(site + 1, &rel, 4);
    std::memset(site + 5, 0x90, sizeof(stolen) - 5);
    uint8_t* resume = site + sizeof(stolen);
    if (!Trampoline::ComputeRel32(reinterpret_cast<uintptr_t>(resume),
                                  reinterpret_cast<uintptr_t>(&capture), rel)) {
        printf("capture is out of reach of the resume point\n"); return 1;
    }
    resume[0] = 0xE9; std::memcpy(resume + 1, &rel, 4);
    if (!Platform::ProtectExec(site, 4096)) { printf("could not seal the hook site\n"); return 1; }

    kick();

    printf("  patch function calls: %d\n", g_calls);
    if (g_calls != 1) ++kptest::g_failures;
    printf("  arguments the wrapper passed:\n");
    expect("arg0 <- rax", g_arg[0], g_seed[0]);
    expect("arg1 <- rcx", g_arg[1], g_seed[1]);
    expect("arg2 <- rsi", g_arg[2], g_seed[6]);

    printf("  registers at the resume point:\n");
    for (int i = 0; i < 16; ++i) {
        if (i == 4) continue;                       // rsp is not part of the frame
        uint64_t want = (i == 0) ? g_seed[0] + 7    // the stolen "add rax, 7" ran
                                 : g_seed[i];
        char label[32]; snprintf(label, sizeof(label), "%s", names[i]);
        expect(label, g_captured[i], want);
    }
    printf("  SSE registers:\n");
    const int xmmChecked[] = { 0, 1, 7, 15 };
    for (int j : xmmChecked) {
        char label[32]; snprintf(label, sizeof(label), "xmm%d.lo", j);
        expect(label, g_capturedXmm[j * 2], g_seedXmm[j * 2]);
    }
    return kptest::Report();
}
