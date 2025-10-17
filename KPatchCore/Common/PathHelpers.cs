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
    /// Validates that a path looks like a valid KOTOR installation directory
    /// </summary>
    public static bool LooksLikeKotorDirectory(string path)
    {
        if (!Directory.Exists(path))
            return false;

        // Check for common KOTOR files
        var exeNames = new[] { "swkotor.exe", "swkotor2.exe", "KOTOR.exe", "KOTOR2.exe" };
        return exeNames.Any(exe => File.Exists(Path.Combine(path, exe)));
    }

    /// <summary>
    /// Finds the KOTOR executable in a directory
    /// Returns null if not found
    /// </summary>
    public static string? FindKotorExecutable(string directory)
    {
        if (!Directory.Exists(directory))
            return null;

        var exeNames = new[] { "swkotor.exe", "swkotor2.exe", "KOTOR.exe", "KOTOR2.exe" };

        foreach (var exeName in exeNames)
        {
            var fullPath = Path.Combine(directory, exeName);
            if (File.Exists(fullPath))
                return fullPath;
        }

        return null;
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
