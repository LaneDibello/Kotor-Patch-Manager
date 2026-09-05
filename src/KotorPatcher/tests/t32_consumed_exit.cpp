// i386 consumed-exit: the handler's return value in EAX has to survive the whole
// epilogue, including the FXRSTOR that borrows EAX to address its save area.
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
    void* g_hookEntry; int g_landed; int g_ret; uint32_t g_flags;
    void kick(void); void land_resume(void); void land_consumed(void);
    int probe(void) { return g_ret; }
}
asm(R"(
.text
.globl kick
kick:
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp
    movl $1, %eax
    cmpl %eax, %eax                 /* ZF=1, must still hold at the landing pad */
    jmp  *g_hookEntry
.globl land_resume
land_resume:
    movl $0, g_landed
    jmp  common
.globl land_consumed
land_consumed:
    movl $1, g_landed
common:
    pushfl
    popl g_flags
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");

static bool build(Wrappers::WrapperGenerator_x86& gen) {
    static const uint8_t stolen[] = { 0x90,0x90,0x90,0x90,0x90 };
    auto* site = static_cast<uint8_t*>(Platform::AllocExec(4096, (uintptr_t)&probe));
    if (!site) return false;
    g_hookEntry = site;
    Wrappers::WrapperConfig c;
    c.patchFunction = (void*)&probe;
    c.hookAddress = (uintptr_t)site;
    c.originalBytes.assign(stolen, stolen + sizeof(stolen));
    c.excludeFromRestore = { "eax" };
    c.consumedExitAddress = (uintptr_t)&land_consumed;
    void* w = gen.GenerateWrapper(c);
    if (!w) return false;
    int32_t rel = 0;
    Trampoline::ComputeRel32((uintptr_t)site, (uintptr_t)w, rel);
    site[0] = 0xE9; std::memcpy(site+1, &rel, 4);
    uint8_t* r = site + sizeof(stolen);
    Trampoline::ComputeRel32((uintptr_t)r, (uintptr_t)&land_resume, rel);
    r[0] = 0xE9; std::memcpy(r+1, &rel, 4);
    return Platform::ProtectExec(site, 4096);
}

int main() {
    Wrappers::WrapperGenerator_x86 gen;
    g_ret = 1; g_landed = -1;
    if (!build(gen)) { printf("build failed\n"); return 1; }
    kick();
    kptest::Check("non-zero return takes the consumed exit", g_landed == 1);

    g_ret = 0; g_landed = -1;
    if (!build(gen)) { printf("build failed\n"); return 1; }
    kick();
    kptest::Check("zero return falls through to resume", g_landed == 0);
    kptest::Check("ZF from before the hook survives", (g_flags & 0x40) != 0);
    return kptest::Report();
}
