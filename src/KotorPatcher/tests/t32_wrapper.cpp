// Runs a wrapper the i386 generator produced: checks the stack is 16-byte aligned
// at the call, and that both a register source and a stack source still arrive
// correctly now that extraction is anchored on EBX.
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
    uint32_t g_gameEsp, g_espAtEntry, g_arg0, g_arg1;
    uint32_t g_captured[8];
    void kick(void);
    void capture(void);
    void probe(void);
}

asm(R"(
.text
.globl probe
probe:
    movl %esp, g_espAtEntry      /* includes the return address the CALL pushed */
    movl 4(%esp), %eax
    movl %eax, g_arg0
    movl 8(%esp), %eax
    movl %eax, g_arg1
    xorl %eax, %eax
    ret

.globl kick
kick:
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp
    movl %esp, g_gameEsp
    movl $0xAA000000, %eax
    movl $0xAA000001, %ecx
    movl $0xAA000002, %edx
    movl $0xAA000003, %ebx
    movl $0xAA000005, %ebp
    movl $0xAA000006, %esi
    movl $0xAA000007, %edi
    jmp  *g_hookEntry

.globl capture
capture:
    movl %eax, g_captured+0
    movl %ecx, g_captured+4
    movl %edx, g_captured+8
    movl %ebx, g_captured+12
    movl %ebp, g_captured+20
    movl %esi, g_captured+24
    movl %edi, g_captured+28
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");


int main() {
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    auto* site = static_cast<uint8_t*>(
        Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&probe)));
    if (!site) { printf("alloc failed\n"); return 1; }
    g_hookEntry = site;

    Wrappers::WrapperGenerator_x86 gen;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&probe);
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = {
        { "eax",   ParameterType::UINT },      // register source
        { "esp+4", ParameterType::POINTER },   // stack source: the path re-anchored on EBX
    };

    void* wrapper = gen.GenerateWrapper(config);
    if (!wrapper) { printf("generation failed\n"); return 1; }

    int32_t rel = 0;
    Trampoline::ComputeRel32((uintptr_t)site, (uintptr_t)wrapper, rel);
    site[0] = 0xE9; std::memcpy(site + 1, &rel, 4);
    uint8_t* resume = site + sizeof(stolen);
    Trampoline::ComputeRel32((uintptr_t)resume, (uintptr_t)&capture, rel);
    resume[0] = 0xE9; std::memcpy(resume + 1, &rel, 4);
    Platform::ProtectExec(site, 4096);

    kick();

    char d[80];
    // The CALL pushed a return address, so an aligned call leaves esp+4 aligned here.
    snprintf(d, sizeof(d), "(esp at entry %#x)", g_espAtEntry);
    kptest::Check("stack 16-byte aligned at the call", ((g_espAtEntry + 4) % 16) == 0, d);
    kptest::Check("register source eax arrived", g_arg0 == 0xAA000000);
    snprintf(d, sizeof(d), "(got %#x want %#x)", g_arg1, g_gameEsp + 4);
    kptest::Check("stack source esp+4 arrived", g_arg1 == g_gameEsp + 4, d);

    const char* n[8] = {"eax","ecx","edx","ebx","","ebp","esi","edi"};
    for (int i = 0; i < 8; ++i) {
        if (i == 4) continue;
        char lbl[48]; snprintf(lbl, sizeof(lbl), "%s restored at the resume point", n[i]);
        kptest::Check(lbl, g_captured[i] == (0xAA000000u + i));
    }
    return kptest::Report();
}
