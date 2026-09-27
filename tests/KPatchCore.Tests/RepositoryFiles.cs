namespace KPatchCore.Tests;

/// <summary>
/// Locates files in the working tree for the tests that read the real thing rather than a
/// fixture.
/// </summary>
internal static class RepositoryFiles
{
    /// <summary>The patches directory, whose hooks files several tests parse.</summary>
    public static string Patches => Path.Combine(Root, "Patches");

    /// <summary>The grammar a constant parameter source is held to, on both sides.</summary>
    public static string ConstantCorpus =>
        Path.Combine(Root, "src", "KotorPatcher", "tests", "constant-sources.tsv");

    // The test assembly runs out of bin/<config>/<tfm>, and the solution file is the closest
    // thing to a marker the repository root has.
    private static string Root
    {
        get
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
}
