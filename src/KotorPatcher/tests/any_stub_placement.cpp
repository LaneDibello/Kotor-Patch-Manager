#include "platform.h"
#include "trampoline.h"
#include <cstdio>

#include "check.h"
#include <cstring>
#include <ctime>
using namespace KotorPatcher;
static void check(const char* name, uintptr_t game) {
    clock_t t0 = clock();
    void* mem = Platform::AllocExec(4096, game);
    double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
    if (!mem) { printf("  %-30s ALLOC FAILED (%.1f ms)\n", name, ms); ++kptest::g_failures; return; }
    uintptr_t p = reinterpret_cast<uintptr_t>(mem);
    int32_t there = 0, back = 0;
    bool ok = Trampoline::ComputeRel32(game, p, there) &&
              Trampoline::ComputeRel32(p + 4091, game, back);
    long long d = (long long)p - (long long)game;
    printf("  %-30s block=%#llx delta=%+lld reaches=%d %.1fms %s\n", name,
           (unsigned long long)p, d, ok, ms, ok ? "PASS" : "FAIL");
    if (!ok) ++kptest::g_failures;
    Platform::FreeExec(mem, 4096);
}
int main() {
    printf("uintptr_t = %zu bits\n", sizeof(uintptr_t) * 8);
#if UINTPTR_MAX > 0xFFFFFFFFu
    // Addresses a macOS game image and a typical mmap region would sit at.
    check("mac image base 0x100000000", 0x100000000ull);
    check("high address 0x7F0000000000", 0x7F0000000000ull);
#endif
    check("this function", reinterpret_cast<uintptr_t>(&main));
    void* any = Platform::AllocExec(4096, 0);
    printf("  %-30s %s\n", "unconstrained (hint 0)", any ? "PASS" : "FAIL");
    if (!any) ++kptest::g_failures; else Platform::FreeExec(any, 4096);
    return kptest::Report();
}
