using Xunit;

using KPatchCore.Detectors;
using KPatchCore.Models;
using KPatchCore.Parsers;

namespace KPatchCore.Tests;

/// <summary>
/// The hooks files in this repository, parsed and checked as the manager would.
/// </summary>
/// <remarks>
/// Tightening what a parameter source may say is only safe if nothing already shipped says
/// something else, and that is a question about the tree rather than about a unit. These read
/// the real files so a table change that would strand an existing patch fails here first.
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

    /// <summary>
    /// Each hooks file paired with an architecture its [metadata] target_versions name.
    /// </summary>
    /// <remarks>
    /// Parsing only asks whether some generator could read a source, because a patch is read
    /// before a game is chosen. A file that names its target versions has already answered
    /// that question, so its sources can be held to the narrower standard the install will
    /// apply. This is what would catch a 64-bit register in a file that also targets a
    /// 32-bit build.
    /// </remarks>
    public static TheoryData<string, Architecture> TargetedHooksFiles()
    {
        var known = GameDetector.GetKnownVersions();
        var data = new TheoryData<string, Architecture>();

        foreach (var path in HooksFilePaths())
        {
            var architectures = HooksParser.ParseMetadata(path).TargetVersions
                .Where(known.ContainsKey)
                .Select(sha => known[sha].Architecture)
                .Distinct();

            foreach (var architecture in architectures)
            {
                data.Add(Path.GetRelativePath(PatchesDirectory(), path), architecture);
            }
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

    [Theory]
    [MemberData(nameof(TargetedHooksFiles))]
    public void EveryShippedParameterIsReadableOnTheBuildsItTargets(
        string relativePath, Architecture architecture)
    {
        var result = HooksParser.ParseFile(Path.Combine(PatchesDirectory(), relativePath));
        Assert.True(result.Success, result.Error);

        foreach (var hook in result.Data!)
        {
            for (var i = 0; i < hook.Parameters.Count; i++)
            {
                Assert.True(
                    hook.Parameters[i].IsValidFor(architecture, out var error),
                    $"{relativePath}: {hook.Function} parameter {i}: {error}");
            }
        }
    }

    [Fact]
    public void ThereIsSomethingToCheck()
    {
        // Both theories above pass vacuously against an empty or wrong directory, and the
        // second also passes for any file whose target versions this does not recognise.
        Assert.NotEmpty(HooksFiles());
        Assert.NotEmpty(TargetedHooksFiles());
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
