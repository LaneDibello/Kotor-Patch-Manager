namespace KPatchCore.Models;

/// <summary>
/// Metadata about a backup of the game executable
/// </summary>
public sealed class BackupInfo
{
    /// <summary>
    /// Path to the original executable that was backed up
    /// </summary>
    public required string OriginalPath { get; init; }

    /// <summary>
    /// Path to the backup file
    /// </summary>
    public required string BackupPath { get; init; }

    /// <summary>
    /// SHA256 hash of the original file (for verification)
    /// </summary>
    public required string Hash { get; init; }

    /// <summary>
    /// File size in bytes
    /// </summary>
    public required long FileSize { get; init; }

    /// <summary>
    /// When the backup was created
    /// </summary>
    public required DateTime CreatedAt { get; init; }

    /// <summary>
    /// Game version that was detected
    /// </summary>
    public GameVersion? DetectedVersion { get; init; }

    /// <summary>
    /// List of patches that were installed when this backup was created
    /// </summary>
    public List<string> InstalledPatches { get; init; } = new();

    /// <summary>
    /// Verifies that the backup file exists and matches the stored hash
    /// </summary>
    public bool VerifyIntegrity()
    {
        if (!File.Exists(BackupPath))
            return false;

        try
        {
            using var stream = File.OpenRead(BackupPath);
            using var sha256 = System.Security.Cryptography.SHA256.Create();
            var hashBytes = sha256.ComputeHash(stream);
            var computedHash = Convert.ToHexString(hashBytes);

            return computedHash.Equals(Hash, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    public override string ToString() =>
        $"Backup of {Path.GetFileName(OriginalPath)} created {CreatedAt:yyyy-MM-dd HH:mm}";
}
