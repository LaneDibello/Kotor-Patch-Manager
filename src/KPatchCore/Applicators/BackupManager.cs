using System.Text.Json;
using KPatchCore.Common;
using KPatchCore.Models;

namespace KPatchCore.Applicators;

/// <summary>
/// Manages backup and restoration of game executables
/// </summary>
public static class BackupManager
{
    /// <summary>
    /// Creates a backup of the game executable with metadata
    /// </summary>
    /// <param name="exePath">Path to the executable to backup</param>
    /// <param name="detectedVersion">Optional detected game version</param>
    /// <param name="installedPatches">List of patches being installed</param>
    /// <returns>Result containing BackupInfo or error</returns>
    public static PatchResult<BackupInfo> CreateBackup(
        string exePath,
        GameVersion? detectedVersion = null,
        List<string>? installedPatches = null)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<BackupInfo>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            var (hash, fileSize) = FileHasher.ComputeHashAndSize(exePath);
            var backupPath = PathHelpers.GetBackupPath(exePath);
            File.Copy(exePath, backupPath, overwrite: false);

            var backupInfo = new BackupInfo
            {
                OriginalPath = Path.GetFullPath(exePath),
                BackupPath = backupPath,
                Hash = hash,
                FileSize = fileSize,
                CreatedAt = DateTime.Now,
                DetectedVersion = detectedVersion,
                InstalledPatches = installedPatches ?? new List<string>()
            };

            var metadataPath = $"{backupPath}.json";
            var saveResult = SaveBackupMetadata(backupInfo, metadataPath);
            if (!saveResult.Success)
            {
                // Cleanup backup file if metadata save fails
                File.Delete(backupPath);
                return PatchResult<BackupInfo>.Fail($"Failed to save backup metadata: {saveResult.Error}");
            }

            return PatchResult<BackupInfo>.Ok(
                backupInfo,
                $"Backup created: {Path.GetFileName(backupPath)}"
            );
        }
        catch (Exception ex)
        {
            return PatchResult<BackupInfo>.Fail($"Failed to create backup: {ex.Message}");
        }
    }

    /// <summary>
    /// Restores a backup to the original location
    /// </summary>
    /// <param name="backup">Backup information</param>
    /// <param name="verifyIntegrity">Whether to verify backup integrity before restoring</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult RestoreBackup(BackupInfo backup, bool verifyIntegrity = true)
    {
        if (!File.Exists(backup.BackupPath))
        {
            return PatchResult.Fail($"Backup file not found: {backup.BackupPath}");
        }

        if (verifyIntegrity && !backup.VerifyIntegrity())
        {
            return PatchResult.Fail("Backup integrity check failed - file may be corrupted");
        }

        try
        {
            // Create directory if it doesn't exist
            var directory = Path.GetDirectoryName(backup.OriginalPath);
            if (directory != null)
            {
                PathHelpers.EnsureDirectoryExists(directory);
            }

            // Restore the backup
            File.Copy(backup.BackupPath, backup.OriginalPath, overwrite: true);

            return PatchResult.Ok($"Restored backup to: {backup.OriginalPath}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to restore backup: {ex.Message}");
        }
    }

    /// <summary>
    /// Loads backup metadata from a JSON file
    /// </summary>
    /// <param name="metadataPath">Path to the .json metadata file</param>
    /// <returns>Result containing BackupInfo or error</returns>
    public static PatchResult<BackupInfo> LoadBackupMetadata(string metadataPath)
    {
        if (!File.Exists(metadataPath))
        {
            return PatchResult<BackupInfo>.Fail($"Metadata file not found: {metadataPath}");
        }

        try
        {
            var json = File.ReadAllText(metadataPath);
            var backupInfo = JsonSerializer.Deserialize<BackupInfo>(json);

            if (backupInfo == null)
            {
                return PatchResult<BackupInfo>.Fail("Failed to deserialize backup metadata");
            }

            return PatchResult<BackupInfo>.Ok(backupInfo, "Metadata loaded successfully");
        }
        catch (Exception ex)
        {
            return PatchResult<BackupInfo>.Fail($"Failed to load backup metadata: {ex.Message}");
        }
    }

    /// <summary>
    /// Saves backup metadata to a JSON file
    /// </summary>
    /// <param name="backup">Backup information to save</param>
    /// <param name="metadataPath">Path where to save the JSON file</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult SaveBackupMetadata(BackupInfo backup, string metadataPath)
    {
        try
        {
            var options = new JsonSerializerOptions
            {
                WriteIndented = true
            };

            var json = JsonSerializer.Serialize(backup, options);
            File.WriteAllText(metadataPath, json);

            return PatchResult.Ok($"Metadata saved to: {metadataPath}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to save backup metadata: {ex.Message}");
        }
    }

    /// <summary>
    /// Finds the most recent backup for a given executable
    /// </summary>
    /// <param name="exePath">Path to the original executable</param>
    /// <returns>Result containing BackupInfo or error</returns>
    public static PatchResult<BackupInfo> FindLatestBackup(string exePath)
    {
        var backupPath = PathHelpers.FindLatestBackup(exePath);

        if (backupPath == null)
        {
            return PatchResult<BackupInfo>.Fail($"No backup found for: {exePath}");
        }

        var metadataPath = $"{backupPath}.json";
        if (File.Exists(metadataPath))
        {
            return LoadBackupMetadata(metadataPath);
        }

        // If no metadata file, create minimal BackupInfo from backup file
        try
        {
            var (hash, fileSize) = FileHasher.ComputeHashAndSize(backupPath);

            var backupInfo = new BackupInfo
            {
                OriginalPath = exePath,
                BackupPath = backupPath,
                Hash = hash,
                FileSize = fileSize,
                CreatedAt = File.GetCreationTime(backupPath)
            };

            return PatchResult<BackupInfo>.Ok(
                backupInfo,
                "Found backup (no metadata file)"
            );
        }
        catch (Exception ex)
        {
            return PatchResult<BackupInfo>.Fail($"Failed to analyze backup: {ex.Message}");
        }
    }

    /// <summary>
    /// Deletes a backup and its metadata
    /// </summary>
    /// <param name="backup">Backup to delete</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult DeleteBackup(BackupInfo backup)
    {
        try
        {
            var deleted = new List<string>();

            if (File.Exists(backup.BackupPath))
            {
                File.Delete(backup.BackupPath);
                deleted.Add(Path.GetFileName(backup.BackupPath));
            }

            var metadataPath = $"{backup.BackupPath}.json";
            if (File.Exists(metadataPath))
            {
                File.Delete(metadataPath);
                deleted.Add(Path.GetFileName(metadataPath));
            }

            if (deleted.Count == 0)
            {
                return PatchResult.Ok("No files to delete");
            }

            return PatchResult.Ok($"Deleted: {string.Join(", ", deleted)}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to delete backup: {ex.Message}");
        }
    }
}
