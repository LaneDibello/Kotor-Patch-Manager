// Holds ParseConstantSource to the corpus the manager's own parser is held to. See the header
// of constant-sources.tsv for why the grammar exists twice and what keeps the two in step.
#include "wrapper_base.h"

#include "check.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using KotorPatcher::Wrappers::ConstantSource;
using KotorPatcher::Wrappers::ParseConstantSource;

namespace {

    const char* Name(ConstantSource outcome) {
        switch (outcome) {
            case ConstantSource::None:      return "none";
            case ConstantSource::Malformed: return "malformed";
            case ConstantSource::Parsed:    return "parsed";
        }
        return "?";
    }

}  // namespace

int main() {
    // The path arrives from the Makefile, so the test does not depend on where it is run.
    std::FILE* corpus = std::fopen(CONSTANT_CORPUS, "r");
    if (!corpus) {
        std::printf("  cannot open %s\n", CONSTANT_CORPUS);
        return 1;
    }

    int rows = 0;
    char line[256];
    while (std::fgets(line, sizeof(line), corpus)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char source[128] = {0};
        char expected[32] = {0};
        char value[32] = {0};
        if (std::sscanf(line, "%127[^\t]\t%31[^\t]\t%31s", source, expected, value) != 3) continue;
        ++rows;

        std::uint64_t parsed = 0;
        const ConstantSource outcome = ParseConstantSource(std::string(source), parsed);

        char label[160];
        std::snprintf(label, sizeof(label), "%s -> %s", source, expected);
        const bool outcomeOk = std::strcmp(Name(outcome), expected) == 0;

        // The value matters as much as the verdict: two parsers agreeing that something is a
        // constant while disagreeing what it equals is the same bug, one step later.
        bool valueOk = true;
        if (outcomeOk && outcome == ConstantSource::Parsed) {
            valueOk = parsed == std::strtoull(value, nullptr, 10);
        }

        char detail[96] = "";
        if (!outcomeOk) {
            std::snprintf(detail, sizeof(detail), "(got %s)", Name(outcome));
        } else if (!valueOk) {
            std::snprintf(detail, sizeof(detail), "(got %llu want %s)",
                          static_cast<unsigned long long>(parsed), value);
        }
        kptest::Check(label, outcomeOk && valueOk, detail);
    }
    std::fclose(corpus);

    // A corpus that failed to load would otherwise pass by checking nothing.
    kptest::Check("the corpus had rows", rows > 0);
    return kptest::Report();
}
