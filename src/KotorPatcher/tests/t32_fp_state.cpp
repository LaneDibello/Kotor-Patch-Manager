// The patch function destroys the whole floating-point state. FXSAVE/FXRSTOR in
// the wrapper must put it back before the game resumes.
#include "wrapper_x86.h"
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
    double   g_seedX87[2], g_gotX87[2];
    uint64_t g_seedXmm[2], g_gotXmm[2];
    uint32_t g_seedMxcsr, g_gotMxcsr;
    void kick(void); void capture(void);
    int probe(void) {
        // Everything a System V callee may legally destroy.
        asm volatile(
            "fninit\n\t"                 // wipe the x87 stack and control word
            "fldz\n\t"                   // leave something wrong in ST0
            "pxor %%xmm0, %%xmm0\n\t"
            "pxor %%xmm3, %%xmm3\n\t"
            ::: "xmm0", "xmm3");
        return 0;
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
    fldl g_seedX87+8
    fldl g_seedX87+0
    movq g_seedXmm+0, %xmm0
    movq g_seedXmm+8, %xmm3
    stmxcsr g_seedMxcsr
    jmp  *g_hookEntry

.globl capture
capture:
    fstpl g_gotX87+0
    fstpl g_gotX87+8
    movq %xmm0, g_gotXmm+0
    movq %xmm3, g_gotXmm+8
    stmxcsr g_gotMxcsr
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");


int main() {
    g_seedX87[0] = 3.14159265358979; g_seedX87[1] = 2.71828182845905;
    g_seedXmm[0] = 0x5EED000000000001ull; g_seedXmm[1] = 0x5EED000000000009ull;

    const uint8_t stolen[] = { 0x90,0x90,0x90,0x90,0x90 };
    auto* site = static_cast<uint8_t*>(Platform::AllocExec(4096, (uintptr_t)&probe));
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86 gen;
    Wrappers::WrapperConfig c;
    c.patchFunction = (void*)&probe;
    c.hookAddress = (uintptr_t)site;
    c.originalBytes.assign(stolen, stolen + sizeof(stolen));

    void* w = gen.GenerateWrapper(c);
    if (!w) { printf("generation failed\n"); return 1; }
    int32_t rel = 0;
    Trampoline::ComputeRel32((uintptr_t)site, (uintptr_t)w, rel);
    site[0] = 0xE9; std::memcpy(site+1, &rel, 4);
    uint8_t* resume = site + sizeof(stolen);
    Trampoline::ComputeRel32((uintptr_t)resume, (uintptr_t)&capture, rel);
    resume[0] = 0xE9; std::memcpy(resume+1, &rel, 4);
    Platform::ProtectExec(site, 4096);

    kick();

    char d[80];
    snprintf(d, sizeof(d), "(%.14f)", g_gotX87[0]);
    kptest::Check("x87 ST0 survived fninit", g_gotX87[0] == g_seedX87[0], d);
    snprintf(d, sizeof(d), "(%.14f)", g_gotX87[1]);
    kptest::Check("x87 ST1 survived fninit", g_gotX87[1] == g_seedX87[1], d);
    snprintf(d, sizeof(d), "(%#llx)", (unsigned long long)g_gotXmm[0]);
    kptest::Check("xmm0 restored", g_gotXmm[0] == g_seedXmm[0], d);
    snprintf(d, sizeof(d), "(%#llx)", (unsigned long long)g_gotXmm[1]);
    kptest::Check("xmm3 restored", g_gotXmm[1] == g_seedXmm[1], d);
    snprintf(d, sizeof(d), "(%#x vs %#x)", g_gotMxcsr, g_seedMxcsr);
    kptest::Check("MXCSR restored", g_gotMxcsr == g_seedMxcsr, d);
    return kptest::Report();
}
