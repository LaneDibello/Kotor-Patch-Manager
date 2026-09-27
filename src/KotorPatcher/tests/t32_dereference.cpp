// A bracketed parameter source reads through the register rather than handing it over.
//
// "esi" gives the patch function an object's address; "[esi+0x10]" gives it the field. Before
// this a bracketed source matched no form and was refused, so a hook wanting a field had to
// take the pointer and dereference it on the other side.
#include "wrapper_x86.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"

#include "check.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace KotorPatcher;

namespace {

    struct Fields {
        uint32_t atZero;
        uint32_t atFour;
        uint32_t atEight;
    };

    // Distinct, so a dereference landing at the wrong displacement or reading the wrong
    // width cannot match by accident.
    Fields g_fields = { 0x11223344u, 0xAABBCCEFu, 0x5566F00Du };

}  // namespace

extern "C" {
    void* g_hookEntry;
    void  kick(void);
    void  finish(void);

    uint32_t g_arg[7];
    int      g_calls;

    void probe(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f,
               uint32_t g) {
        g_arg[0] = a; g_arg[1] = b; g_arg[2] = c; g_arg[3] = d; g_arg[4] = e; g_arg[5] = f;
        g_arg[6] = g;
        ++g_calls;
    }
}

// ESI carries the struct's address, which is what a hook on a member function would find.
asm(R"(
.text
.globl kick
kick:
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp
    movl g_fieldsAddress, %esi
    pushl $0xFEEDFACE
    jmp  *g_hookEntry

.globl finish
finish:
    addl $4, %esp
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret
)");

extern "C" { void* g_fieldsAddress; }

namespace {

void Expect(const char* what, uint32_t got, uint32_t want) {
    char detail[64];
    std::snprintf(detail, sizeof(detail), "(got %#x want %#x)", got, want);
    kptest::Check(what, got == want, detail);
}

}  // namespace

int main() {
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    g_fieldsAddress = &g_fields;

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
        { "[esi]",      ParameterType::UINT },
        { "[esi+4]",    ParameterType::UINT },
        { "[esi+0x8]",  ParameterType::UINT },
        // The same field read narrow, to show the width applies to the loaded value
        // rather than to the address.
        { "[esi+4]",    ParameterType::BYTE },
        // The pushed dword, which the address form cannot reach: "esp+0" is where it sits.
        { "[esp+0]",    ParameterType::UINT },
        // Without brackets the offset moves the address instead, so what arrives is where
        // the field is.
        { "esi+4",      ParameterType::POINTER },
        { "esi-4",      ParameterType::POINTER },
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
    Expect("[esi] reads the field at zero",   g_arg[0], g_fields.atZero);
    Expect("[esi+4] reads the second field",  g_arg[1], g_fields.atFour);
    Expect("[esi+0x8] takes a hex offset",    g_arg[2], g_fields.atEight);
    Expect("a byte dereference masks",        g_arg[3], g_fields.atFour & 0xFFu);
    Expect("[esp+0] reads the pushed dword",  g_arg[4], 0xFEEDFACEu);
    const uint32_t fields = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_fields));
    Expect("esi+4 is the address of the field", g_arg[5], fields + 4);
    Expect("esi-4 reaches below the object",    g_arg[6], fields - 4);

    // An address is still an address: "esi" has to keep meaning the pointer, or every hook
    // written before brackets existed changes meaning.
    Wrappers::WrapperGenerator_x86 plain;
    Wrappers::WrapperConfig plainConfig = config;
    plainConfig.parameters = { { "esi", ParameterType::POINTER } };
    kptest::Check("an unbracketed register still builds",
                  plain.GenerateWrapper(plainConfig) != nullptr);

    Wrappers::WrapperGenerator_x86 bad;
    Wrappers::WrapperConfig badConfig = config;
    badConfig.parameters = { { "[nosuchreg+4]", ParameterType::UINT } };
    kptest::Check("a dereference of an unknown register is refused",
                  bad.GenerateWrapper(badConfig) == nullptr);

    return kptest::Report();
}
