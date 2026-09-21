using Xunit;

using KPatchCore.Parsers;

namespace KPatchCore.Tests;

/// <summary>
/// The hooks files in this repository, parsed as the manager parses them.
/// </summary>
/// <remarks>
/// Whether a change to the parser or to what a hook may say would strand something already
/// shipped is a question about the tree rather than about a unit, so this reads the real
/// files rather than a fixture.
/// </remarks>
public class ShippedHooksTests
{
    public static TheoryData<string> HooksFiles()
    {
        var data = new TheoryData<string>();
        foreach (var path in HooksFilePaths())
        {
            data.Add(Path.GetRelativePath(PatchesDirectory(), path));
        }

        return data;
    }

    [Theory]
    [MemberData(nameof(HooksFiles))]
    public void EveryShippedHooksFileParses(string relativePath)
    {
        var result = HooksParser.ParseFile(Path.Combine(PatchesDirectory(), relativePath));

        Assert.True(result.Success, result.Error);
    }

    [Fact]
    public void ThereIsSomethingToCheck()
    {
        // The theory above passes vacuously against an empty or wrong directory.
        Assert.NotEmpty(HooksFiles());
    }

    private static IEnumerable<string> HooksFilePaths() =>
        Directory.GetFiles(PatchesDirectory(), "*hooks.toml", SearchOption.AllDirectories)
            .OrderBy(path => path, StringComparer.Ordinal);

    private static string PatchesDirectory() => Path.Combine(RepositoryRoot(), "Patches");

    // The test assembly runs out of bin/<config>/<tfm>, and the solution file is the closest
    // thing to a marker the repository root has.
    private static string RepositoryRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);

        while (directory is not null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "KotorPatchManager.sln")))
            {
                return directory.FullName;
            }

            directory = directory.Parent;
        }

        throw new InvalidOperationException(
            $"No KotorPatchManager.sln above {AppContext.BaseDirectory}");
    }
}
