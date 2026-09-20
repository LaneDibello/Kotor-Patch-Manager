// A parameter the generator cannot read has to abandon the whole wrapper.
//
// Pushing the ones it understands and skipping the one it does not would shift every
// later argument down a slot, so the patch function would be called with its arguments
// misaligned and the last of them read off whatever the stack held. That is worse than
// no hook at all, and it is what the x86 generator used to do: it logged the source and
// carried on. Both generators now refuse.

#include "check.h"
#include "patcher.h"
#include "platform.h"

#if defined(__x86_64__) || defined(_M_X64)
#include "wrappers/wrapper_x86_64.h"
using Generator = KotorPatcher::Wrappers::WrapperGenerator_x86_64;
// Valid on this generator, so the run proves the refusal is about the bad source
// rather than the config being unusable for some other reason.
static const char* kGoodSource = "rbp";
#else
#include "wrappers/wrapper_x86.h"
using Generator = KotorPatcher::Wrappers::WrapperGenerator_x86;
static const char* kGoodSource = "ebp";
#endif

using namespace KotorPatcher;

namespace {

// Somewhere for the stub's relative jumps to reach. Never executed.
void* Site() {
    static void* site = Platform::AllocExec(4096, reinterpret_cast<uintptr_t>(&Site));
    return site;
}

void Dummy() {}

void* Generate(const char* source) {
    Generator generator;
    Wrappers::WrapperConfig config;
    config.patchFunction = reinterpret_cast<void*>(&Dummy);
    config.hookAddress = reinterpret_cast<uintptr_t>(Site());
    const uint8_t stolen[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    config.originalBytes.assign(stolen, stolen + sizeof(stolen));
    config.parameters = { { source, ParameterType::UINT } };
    return generator.GenerateWrapper(config);
}

}  // namespace

int main() {
    if (!Site()) { std::printf("  alloc failed\n"); return 1; }

    kptest::Check("a source this generator knows still builds", Generate(kGoodSource) != nullptr);
    kptest::Check("an unknown register name is refused", Generate("nosuchreg") == nullptr);
    kptest::Check("a stack offset that will not parse is refused", Generate("esp+bogus") == nullptr);

    // The width the other generator uses. Accepting it here would mean generating a
    // wrapper that reads a register this architecture has no saved copy of.
#if defined(__x86_64__) || defined(_M_X64)
    kptest::Check("an 8-bit subregister is refused", Generate("al") == nullptr);
#else
    kptest::Check("a 64-bit register is refused", Generate("rax") == nullptr);
#endif

    return kptest::Report();
}
