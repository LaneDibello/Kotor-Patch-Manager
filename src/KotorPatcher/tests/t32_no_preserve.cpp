// preserveRegisters = false: the patch's register changes are meant to stick, but
// the wrapper's own anchor must not leak into the game.
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
    void* g_hookEntry;
    uint32_t g_gameEbx, g_arg0, g_captured[8];
    void kick(void); void capture(void); void probe(void);
}
asm(R"(
.text
.globl probe
probe:
    movl 4(%esp), %eax
    movl %eax, g_arg0
    movl $0xC0FFEE00, %ecx      /* the patch's own change, which must survive */
    xorl %eax, %eax
    ret

.globl kick
kick:
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp
    movl $0xAA000000, %eax
    movl $0xAA000001, %ecx
    movl $0xAA000003, %ebx
    movl %ebx, g_gameEbx
    jmp  *g_hookEntry

.globl capture
capture:
    movl %eax, g_captured+0
    movl %ecx, g_captured+4
    movl %ebx, g_captured+12
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");


int main() {
    const uint8_t stolen[] = { 0x90,0x90,0x90,0x90,0x90 };
    auto* site = static_cast<uint8_t*>(Platform::AllocExec(4096, (uintptr_t)&probe));
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86 gen;
    Wrappers::WrapperConfig c;
    c.patchFunction = (void*)&probe;
    c.hookAddress = (uintptr_t)site;
    c.originalBytes.assign(stolen, stolen + sizeof(stolen));
    c.preserveRegisters = false;          // the case under test
    c.preserveFlags = false;
    c.parameters = { { "eax", ParameterType::UINT } };   // needs the saved copy to exist

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

    char d[64];
    snprintf(d, sizeof(d), "(got %#x)", g_arg0);
    kptest::Check("register parameter still readable without preserve", g_arg0 == 0xAA000000, d);
    snprintf(d, sizeof(d), "(got %#x want %#x)", g_captured[3], g_gameEbx);
    kptest::Check("ebx is the game's, not the wrapper's anchor", g_captured[3] == g_gameEbx, d);
    snprintf(d, sizeof(d), "(got %#x)", g_captured[1]);
    kptest::Check("ecx keeps the patch's value, as asked", g_captured[1] == 0xC0FFEE00, d);
    return kptest::Report();
}
