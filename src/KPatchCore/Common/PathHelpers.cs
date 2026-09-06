namespace KPatchCore.Common;

/// <summary>
/// Path manipulation helpers with additional safety checks
/// </summary>
public static class PathHelpers
{
    /// <summary>
    /// Safely combines paths and ensures they stay within the base directory
    /// Prevents directory traversal attacks
    /// </summary>
    public static string SafeCombine(string basePath, params string[] paths)
    {
        var combined = Path.Combine(new[] { basePath }.Concat(paths).ToArray());
        var fullCombined = Path.GetFullPath(combined);
        var fullBase = Path.GetFullPath(basePath);

        if (!fullCombined.StartsWith(fullBase, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException(
                $"Path '{combined}' resolves outside base directory '{basePath}'");
        }

        return fullCombined;
    }

    /// <summary>
    /// Ensures a directory exists, creating it if necessary
    /// </summary>
    public static void EnsureDirectoryExists(string directoryPath)
    {
        if (!Directory.Exists(directoryPath))
        {
            Directory.CreateDirectory(directoryPath);
        }
    }

    /// <summary>
    /// Gets a unique backup filename for a file
    /// Example: "swkotor.exe" -> "swkotor.exe.backup.20241016_153045"
    /// </summary>
    public static string GetBackupPath(string originalPath)
    {
        var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
        return $"{originalPath}.backup.{timestamp}";
    }

    /// <summary>
    /// Gets the most recent backup file for a given original file
    /// Returns null if no backup exists
    /// </summary>
    public static string? FindLatestBackup(string originalPath)
    {
        var directory = Path.GetDirectoryName(originalPath);
        var fileName = Path.GetFileName(originalPath);

        if (directory == null)
            return null;

        var pattern = $"{fileName}.backup.*";
        var backups = Directory.GetFiles(directory, pattern)
            .Where(f => !f.EndsWith(".json", StringComparison.OrdinalIgnoreCase)) // Exclude metadata files
            .OrderByDescending(f => File.GetCreationTime(f))
            .FirstOrDefault();

        return backups;
    }

    /// <summary>
    /// Determines whether two paths refer to the same file/directory location.
    /// Comparison is done on the fully-resolved paths, is case-insensitive
    /// (matching Windows and Wine filesystem semantics), and ignores a trailing
    /// directory separator. This is a path-string comparison, not a
    /// hardlink/symlink identity check.
    /// Returns false if either path is null or empty.
    /// </summary>
    public static bool SamePath(string? a, string? b)
    {
        if (string.IsNullOrEmpty(a) || string.IsNullOrEmpty(b))
            return false;

        static string Normalize(string p) =>
            Path.TrimEndingDirectorySeparator(Path.GetFullPath(p));

        return string.Equals(Normalize(a), Normalize(b), StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>
    /// Converts an absolute path to a relative path from a base directory
    /// </summary>
    public static string GetRelativePath(string basePath, string targetPath)
    {
        var baseUri = new Uri(Path.GetFullPath(basePath) + Path.DirectorySeparatorChar);
        var targetUri = new Uri(Path.GetFullPath(targetPath));

        var relativeUri = baseUri.MakeRelativeUri(targetUri);
        return Uri.UnescapeDataString(relativeUri.ToString())
            .Replace('/', Path.DirectorySeparatorChar);
    }

    /// <summary>
    /// Names that sit directly in the game directory: the Windows builds and the native Linux one.
    /// </summary>
    private static readonly string[] DirectExecutableNames =
        { "swkotor.exe", "swkotor2.exe", "KOTOR.exe", "KOTOR2.exe", "KOTOR2" };

    /// <summary>
    /// Names inside an .app bundle's Contents/MacOS. Deliberately not the bundle's
    /// CFBundleExecutable: that is an Aspyr launcher stub which starts the real game beside it.
    /// KOTOR II's stub is called "KOTOR2", the same name the native Linux build uses, so searching
    /// a bundle with the list above would find the stub and patch the wrong file.
    /// </summary>
    private static readonly string[] BundleExecutableNames = { "KOTOR_Exe", "KOTOR2sub" };

    /// <summary>
    /// Validates that a path looks like a valid KOTOR installation directory
    /// </summary>
    public static bool LooksLikeKotorDirectory(string path) => FindKotorExecutable(path) is not null;

    /// <summary>
    /// Turns whatever the user pointed at into the executable to patch. A path that is already a
    /// file is taken as given; anything else is searched, which covers a macOS .app bundle and the
    /// folder holding one. A path with no game under it comes back unchanged, so the caller reports
    /// what the user actually chose rather than an empty box.
    /// </summary>
    public static string ResolveGameExecutable(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || File.Exists(path))
            return path;

        return FindKotorExecutable(path) ?? path;
    }

    /// <summary>
    /// Finds the KOTOR executable in a directory
    /// Returns null if not found
    /// </summary>
    public static string? FindKotorExecutable(string directory)
    {
        if (!Directory.Exists(directory))
            return null;

        foreach (var exeName in DirectExecutableNames)
        {
            var fullPath = Path.Combine(directory, exeName);
            if (File.Exists(fullPath))
                return fullPath;
        }

        // A macOS install keeps the game inside an .app, so accept either the directory holding
        // the bundle (what a Steam library looks like) or the bundle itself.
        foreach (var bundle in BundlesIn(directory))
        {
            foreach (var exeName in BundleExecutableNames)
            {
                var fullPath = Path.Combine(bundle, "Contents", "MacOS", exeName);
                if (File.Exists(fullPath))
                    return fullPath;
            }
        }

        return null;
    }

    private static IEnumerable<string> BundlesIn(string directory)
    {
        if (directory.EndsWith(".app", StringComparison.OrdinalIgnoreCase))
            return new[] { directory };

        try
        {
            return Directory.GetDirectories(directory, "*.app");
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            // A directory we cannot read is not a KOTOR install as far as this is concerned.
            // Narrower than catching everything, so a malformed path still surfaces as the bug
            // it is rather than looking like an empty folder.
            return Array.Empty<string>();
        }
    }

    /// <summary>
    /// Moves a rewritten file over the one it replaces, carrying the original's permissions with
    /// it. A freshly created file gets default permissions, so a plain move leaves a game
    /// executable that is no longer executable and a game that will not start.
    /// </summary>
    /// <remarks>
    /// Only whole-file rewrites need this. Byte patching writes through the existing file, where
    /// the mode is never at risk.
    ///
    /// A Windows host cannot do it: SetUnixFileMode throws there, so patching a Linux or macOS
    /// install from Windows leaves the executable bit to whatever the filesystem hands out.
    /// Writing in place rather than replacing would avoid that, but it gives up the property this
    /// exists for, a failed write leaving the original intact, which is the only safety net when
    /// the caller has backups turned off.
    /// </remarks>
    public static void ReplacePreservingMode(string tempPath, string destinationPath)
    {
        // Get/SetUnixFileMode are unsupported on Windows, which has no executable bit to carry.
        // The guard also satisfies CA1416.
        if (!OperatingSystem.IsWindows())
            File.SetUnixFileMode(tempPath, File.GetUnixFileMode(destinationPath));

        File.Move(tempPath, destinationPath, overwrite: true);
    }

    /// <summary>
    /// Creates a temporary directory with a unique name
    /// </summary>
    public static string CreateTempDirectory()
    {
        var tempPath = Path.Combine(Path.GetTempPath(), $"KPatch_{Guid.NewGuid():N}");
        Directory.CreateDirectory(tempPath);
        return tempPath;
    }

    /// <summary>
    /// Safely deletes a directory and all its contents
    /// Ignores errors if directory doesn't exist
    /// </summary>
    public static void SafeDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path))
            {
                Directory.Delete(path, recursive: true);
            }
        }
        catch
        {
            // Ignore errors - best effort deletion
        }
    }
}
