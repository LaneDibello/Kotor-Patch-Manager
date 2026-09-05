#include "trampoline.h"
#include <cstdio>

#include "check.h"
#include <cstdint>
#include <cstddef>
using KotorPatcher::Trampoline::ComputeRel32;
static void expect(const char* name, uintptr_t from, uintptr_t to, bool wantOk) {
    int32_t rel = 0;
    bool ok = ComputeRel32(from, to, rel);
    bool lands = ok && (from + 5 + static_cast<uintptr_t>(static_cast<intptr_t>(rel))) == to;
    bool pass = (ok == wantOk) && (!ok || lands);
    if (!pass) ++kptest::g_failures;
    printf("  %-34s ok=%d rel=%+d lands=%d  %s\n", name, ok, rel, lands, pass ? "PASS" : "FAIL");
}
int main() {
    printf("uintptr_t = %zu bits\n", sizeof(uintptr_t) * 8);
    expect("forward near", 0x401000, 0x402000, true);
    expect("backward near", 0x402000, 0x401000, true);
    if (sizeof(uintptr_t) == 4) {
        expect("32-bit wrap high->low", 0xFFFF0000u, 0x00001000u, true);
        expect("32-bit wrap low->high", 0x00001000u, 0xFFFF0000u, true);
    }
#if UINTPTR_MAX > 0xFFFFFFFFu
    else {
        expect("mac image internal", 0x100000000ull, 0x1004EB6C2ull, true);
        expect("mmap block far above image", 0x100000000ull, 0x7F1234567000ull, false);
        expect("mmap block far below image", 0x7F1234567000ull, 0x100000000ull, false);
        expect("exactly +INT32_MAX", 0x100000000ull, 0x100000000ull + 5 + 0x7FFFFFFFull, true);
        expect("one past +INT32_MAX", 0x100000000ull, 0x100000000ull + 5 + 0x80000000ull, false);
        expect("exactly -INT32_MIN", 0x800000000ull, 0x800000000ull + 5 - 0x80000000ull, true);
        expect("one past -INT32_MIN", 0x800000000ull, 0x800000000ull + 5 - 0x80000001ull, false);
    }
#endif
    return kptest::Report();
}
