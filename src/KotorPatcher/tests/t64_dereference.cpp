// A bracketed parameter source reads through the register rather than handing it over.
//
// The x86_64 counterpart of t32_dereference. The interesting difference is that the address
// load lands straight in the destination argument register and is then read through, so the
// generator borrows nothing to do it.
//
// "esi" gives the patch function an object's address; "[esi+0x10]" gives it the field. Before
// this a bracketed source matched no form and was refused, so a hook wanting a field had to
// take the pointer and dereference it on the other side.
#include "wrapper_x86_64.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"

#include "check.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace KotorPatcher;

namespace {

    struct Fields {
        uint32_t atZero;
        uint32_t atFour;
        uint32_t atEight;
        float    atTwelve;
        double   atSixteen;
    };
    static_assert(offsetof(Fields, atTwelve) == 12 && offsetof(Fields, atSixteen) == 16,
                  "the sources below name these offsets");

    // Distinct, so a dereference landing at the wrong displacement or reading the wrong
    // width cannot match by accident. The floats are exact in binary, so they compare equal.
    Fields g_fields = { 0x11223344u, 0xAABBCCEFu, 0x5566F00Du, 1.5f, -2.25 };

}  // namespace

extern "C" {
    void* g_hookEntry;
    void  kick(void);
    void  finish(void);

    uint64_t g_arg[5];
    float    g_float;
    double   g_double;
    int      g_calls;

    // System V numbers the XMM arguments separately, so f and g arrive in XMM0 and XMM1.
    void probe(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, float f, double g) {
        g_arg[0] = a; g_arg[1] = b; g_arg[2] = c; g_arg[3] = d; g_arg[4] = e;
        g_float = f; g_double = g;
        ++g_calls;
    }
}

// ESI carries the struct's address, which is what a hook on a member function would find.
asm(R"(
.text
.globl kick
kick:
    pushq %rbx
    pushq %r15
    movq g_fieldsAddress(%rip), %r15
    pushq $0x0EEDFACE
    jmp  *g_hookEntry(%rip)

.globl finish
finish:
    addq $8, %rsp
    popq %r15
    popq %rbx
    ret
)");

extern "C" { void* g_fieldsAddress; }

namespace {

void Expect(const char* what, uint64_t got, uint64_t want) {
    char detail[64];
    std::snprintf(detail, sizeof(detail), "(got %#llx want %#llx)",
                  (unsigned long long)got, (unsigned long long)want);
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

    Wrappers::WrapperGenerator_x86_64 gen;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&probe);
    config.hookAddress = reinterpret_cast<uintptr_t>(site);
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = {
        { "[r15]",      ParameterType::UINT },
        { "[r15+4]",    ParameterType::UINT },
        { "[r15+0x8]",  ParameterType::UINT },
        // The same field read narrow, to show the width applies to the loaded value
        // rather than to the address.
        { "[r15+4]",    ParameterType::BYTE },
        // The pushed dword, which the address form cannot reach: "esp+0" is where it sits.
        { "[rsp+0]",    ParameterType::UINT },
        // The longest path either generator has: the address, the load through it, then
        // the move across to XMM.
        { "[r15+0xC]",  ParameterType::FLOAT },
        { "[r15+0x10]", ParameterType::DOUBLE },
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
    Expect("[r15] reads the field at zero",   g_arg[0], g_fields.atZero);
    Expect("[r15+4] reads the second field",  g_arg[1], g_fields.atFour);
    Expect("[r15+0x8] takes a hex offset",    g_arg[2], g_fields.atEight);
    Expect("a byte dereference masks",        g_arg[3], g_fields.atFour & 0xFFu);
    Expect("[rsp+0] reads the pushed qword",  g_arg[4], 0x0EEDFACEu);
    kptest::Check("a float arrives through a register",  g_float == g_fields.atTwelve);
    kptest::Check("a double arrives through a register", g_double == g_fields.atSixteen);

    // An address is still an address: "esi" has to keep meaning the pointer, or every hook
    // written before brackets existed changes meaning.
    Wrappers::WrapperGenerator_x86_64 plain;
    Wrappers::WrapperConfig plainConfig = config;
    plainConfig.parameters = { { "r15", ParameterType::POINTER } };
    kptest::Check("an unbracketed register still builds",
                  plain.GenerateWrapper(plainConfig) != nullptr);

    Wrappers::WrapperGenerator_x86_64 bad;
    Wrappers::WrapperConfig badConfig = config;
    badConfig.parameters = { { "[nosuchreg+4]", ParameterType::UINT } };
    kptest::Check("a dereference of an unknown register is refused",
                  bad.GenerateWrapper(badConfig) == nullptr);

    return kptest::Report();
}
