#pragma once
#include <cstdio>

// Shared reporting for the code-generator tests. Each test is a small program
// that returns non-zero when something failed, so `make test` can just run them.

namespace kptest {

    inline int g_failures = 0;

    inline void Check(const char* what, bool passed, const char* detail = "") {
        if (!passed) ++g_failures;
        std::printf("    %-44s %s %s\n", what, passed ? "PASS" : "FAIL", detail);
    }

    inline int Report() {
        std::printf(g_failures ? "  FAILURES: %d\n" : "  all pass\n", g_failures);
        return g_failures != 0;
    }

} // namespace kptest
