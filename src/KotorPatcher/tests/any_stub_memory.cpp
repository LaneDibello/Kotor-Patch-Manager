// Stub memory is handed out writable and only becomes executable once sealed,
// so a stub has to be written before ProtectExec and callable after it.
#include <cstdio>
#include <cstring>

#include "check.h"
#include "platform.h"

using namespace KotorPatcher;

int main() {
    std::printf("  stub memory, %zu-bit\n", sizeof(void*) * 8);

    // mov eax, 42 ; ret. The same encoding on x86 and x86_64.
    const unsigned char code[] = { 0xB8, 0x2A, 0x00, 0x00, 0x00, 0xC3 };

    void* mem = Platform::AllocExec(64, 0);
    kptest::Check("AllocExec returned a block", mem != nullptr);
    if (!mem) return kptest::Report();

    std::memcpy(mem, code, sizeof(code));          // writable here
    kptest::Check("ProtectExec sealed it", Platform::ProtectExec(mem, 64));

    int got = reinterpret_cast<int (*)()>(mem)();  // executable here
    char detail[32];
    std::snprintf(detail, sizeof(detail), "(returned %d)", got);
    kptest::Check("the sealed stub runs", got == 42, detail);

    Platform::FreeExec(mem, 64);
    return kptest::Report();
}
