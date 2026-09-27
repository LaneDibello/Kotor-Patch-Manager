// Holds ParseSignedOffset to the corpus the manager's own parser is held to. See the header
// of signed-offsets.tsv for why the grammar exists twice.
#include "wrapper_base.h"

#include "check.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using KotorPatcher::Wrappers::ParseSignedOffset;

int main() {
    // The path arrives from the Makefile, so the test does not depend on where it is run.
    std::FILE* corpus = std::fopen(OFFSET_CORPUS, "r");
    if (!corpus) {
        std::printf("  cannot open %s\n", OFFSET_CORPUS);
        return 1;
    }

    int rows = 0;
    char line[256];
    while (std::fgets(line, sizeof(line), corpus)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char text[128] = {0};
        char expected[32] = {0};
        char value[32] = {0};
        if (std::sscanf(line, "%127[^\t]\t%31[^\t]\t%31s", text, expected, value) != 3) continue;
        ++rows;

        int parsed = 0;
        const bool ok = ParseSignedOffset(std::string(text), parsed);
        const bool wantParsed = std::strcmp(expected, "parsed") == 0;

        char label[160];
        std::snprintf(label, sizeof(label), "\"%s\" -> %s", text, expected);
        char detail[64] = "";
        if (ok != wantParsed) {
            std::snprintf(detail, sizeof(detail), "(got %s)", ok ? "parsed" : "malformed");
            kptest::Check(label, false, detail);
            continue;
        }

        // Agreeing that something is an offset while disagreeing on its value is the same
        // bug, one step later.
        const bool valueOk = !ok || parsed == std::strtol(value, nullptr, 10);
        if (!valueOk) {
            std::snprintf(detail, sizeof(detail), "(got %d want %s)", parsed, value);
        }
        kptest::Check(label, valueOk, detail);
    }
    std::fclose(corpus);

    // A corpus that failed to load would otherwise pass by checking nothing.
    kptest::Check("the corpus had rows", rows > 0);
    return kptest::Report();
}
