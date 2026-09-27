using Xunit;

using KPatchCore.Models;

namespace KPatchCore.Tests;

/// <summary>
/// Holds <see cref="Parameter.TryParseConstant"/> to the corpus the patcher's own parser is
/// held to. See the header of constant-sources.tsv for why the grammar exists twice.
/// </summary>
public class ConstantGrammarTests
{
    public static TheoryData<string, string, string> Corpus()
    {
        var data = new TheoryData<string, string, string>();
        foreach (var line in File.ReadLines(RepositoryFiles.ConstantCorpus))
        {
            if (line.Length == 0 || line[0] == '#') continue;

            var fields = line.Split('\t');
            if (fields.Length != 3) continue;

            data.Add(fields[0], fields[1], fields[2]);
        }

        return data;
    }

    [Theory]
    [MemberData(nameof(Corpus))]
    public void TheGrammarMatchesTheCorpus(string source, string outcome, string value)
    {
        // The C++ side distinguishes "not a constant" from "a constant that will not parse",
        // because the first falls through to the register forms and the second must not. This
        // side folds both into false, the caller having no such decision to make.
        var parsed = Parameter.TryParseConstant(StripPrefix(source), out var actual);

        Assert.Equal(outcome == "parsed", parsed);

        if (outcome == "parsed")
        {
            Assert.Equal(ulong.Parse(value), actual);
        }
    }

    [Fact]
    public void TheCorpusHasRows()
    {
        // Guards the theory: an unreadable corpus would otherwise check nothing.
        Assert.NotEmpty(Corpus());
    }

    // TryParseConstant takes the text after "const:", the prefix being the caller's business.
    // A source without the prefix is not a constant at all, and its whole text is passed so
    // that it fails as one.
    private static string StripPrefix(string source) =>
        source.StartsWith("const:", StringComparison.Ordinal) ? source["const:".Length..] : source;
}
