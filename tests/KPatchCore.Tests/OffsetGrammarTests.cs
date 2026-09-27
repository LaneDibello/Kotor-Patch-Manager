using Xunit;

using KPatchCore.Models;

namespace KPatchCore.Tests;

/// <summary>
/// Holds <see cref="Parameter.TryParseSignedOffset"/> to the corpus the patcher's own parser
/// is held to. See the header of signed-offsets.tsv for why the grammar exists twice.
/// </summary>
public class OffsetGrammarTests
{
    public static TheoryData<string, string, string> Corpus()
    {
        var data = new TheoryData<string, string, string>();
        foreach (var line in File.ReadLines(RepositoryFiles.OffsetCorpus))
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
    public void TheGrammarMatchesTheCorpus(string text, string outcome, string value)
    {
        var parsed = Parameter.TryParseSignedOffset(text, out var actual);

        Assert.Equal(outcome == "parsed", parsed);

        if (outcome == "parsed")
        {
            Assert.Equal(int.Parse(value), actual);
        }
    }

    [Fact]
    public void TheCorpusHasRows()
    {
        // Guards the theory: an unreadable corpus would otherwise check nothing.
        Assert.NotEmpty(Corpus());
    }
}
