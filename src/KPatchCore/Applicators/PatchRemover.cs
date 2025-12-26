using KPatchCore.Common;
using KPatchCore.Models;

namespace KPatchCore.Applicators;

/// <summary>
/// Handles patch removal and restoration
/// </summary>
public static class PatchRemover
{
    /// <summary>
    /// Safely deletes a file if it exists and adds a message
    /// </summary>
    /// <param name="gameDir">Game directory</param>
    /// <param name="fileName">File name to delete</param>
    /// <param name="removedFiles">List to add removed file names to</param>
    /// <param name="messages">List to add messages to</param>
    private static void SafeDeleteFile(string gameDir, string fileName, List<string> removedFiles, List<string> messages)
    {
        var filePath = Path.Combine(gameDir, fileName);
        if (File.Exists(filePath))
        {
            File.Delete(filePath);
            removedFiles.Add(fileName);
            messages.Add($"  Removed {fileName}");
        }
    }

    /// <summary>
    /// Removal result with detailed information
    /// </summary>
    public sealed class RemovalResult
    {
        /// <summary>
        /// Whether removal succeeded
        /// </summary>
        public required bool Success { get; init; }

        /// <summary>
        /// Error message if failed
        /// </summary>
        public string? Error { get; init; }

        /// <summary>
        /// List of files that were removed
        /// </summary>
        public List<string> RemovedFiles { get; init; } = new();

        /// <summary>
        /// Whether backup was restored
        /// </summary>
        public bool BackupRestored { get; init; }

        /// <summary>
        /// Additional messages
        /// </summary>
        public List<string> Messages { get; init; } = new();
    }

    /// <summary>
    /// Removes all patches from a game installation
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Removal result</returns>
    public static RemovalResult RemoveAllPatches(string gameExePath)
    {
        var messages = new List<string>();
        var removedFiles = new List<string>();

        try
        {
            // Step 1: Validate inputs
            messages.Add("Step 1/4: Validating inputs...");
            if (!File.Exists(gameExePath))
            {
                return new RemovalResult
                {
                    Success = false,
                    Error = $"Game executable not found: {gameExePath}",
                    Messages = messages
                };
            }

            var gameDir = Path.GetDirectoryName(gameExePath);
            if (gameDir == null)
            {
                return new RemovalResult
                {
                    Success = false,
                    Error = "Invalid game executable path",
                    Messages = messages
                };
            }

            // Step 2: Find and restore backup
            messages.Add("Step 2/4: Finding backup...");
            var backupResult = BackupManager.FindLatestBackup(gameExePath);

            if (!backupResult.Success || backupResult.Data == null)
            {
                messages.Add($"  No backup found: {backupResult.Error}");
                messages.Add("  Will clean up patch files without restoring executable");
            }
            else
            {
                var backup = backupResult.Data;
                messages.Add($"  Found backup: {Path.GetFileName(backup.BackupPath)}");

                // Restore backup
                messages.Add("  Restoring backup...");
                var restoreResult = BackupManager.RestoreBackup(backup);

                if (!restoreResult.Success)
                {
                    return new RemovalResult
                    {
                        Success = false,
                        Error = $"Failed to restore backup: {restoreResult.Error}",
                        Messages = messages
                    };
                }

                messages.Add("  Backup restored successfully");

                // Clean up backup files after successful restore
                messages.Add("  Cleaning up backup files...");
                var deleteResult = BackupManager.DeleteBackup(backup);
                if (deleteResult.Success)
                {
                    messages.AddRange(deleteResult.Messages.Select(m => $"  {m}"));
                }
                else
                {
                    messages.Add($"  ⚠️ Warning: Failed to delete backup files: {deleteResult.Error}");
                }
            }

            // Step 3: Remove patch files
            messages.Add("Step 3/4: Removing patch files...");

            // Remove patches directory
            var patchesDir = Path.Combine(gameDir, "patches");
            if (Directory.Exists(patchesDir))
            {
                var dllFiles = Directory.GetFiles(patchesDir, "*.dll");
                foreach (var dll in dllFiles)
                {
                    File.Delete(dll);
                    removedFiles.Add(Path.GetFileName(dll));
                }

                if (Directory.GetFiles(patchesDir).Length == 0 &&
                    Directory.GetDirectories(patchesDir).Length == 0)
                {
                    Directory.Delete(patchesDir);
                    messages.Add($"  Removed patches directory");
                }
            }

            // List of files to remove from game directory
            var filesToRemove = new[]
            {
                "patch_config.toml",
                "KotorPatcher.dll",
                "KPatchLauncher.exe",
                "KPatchLauncher.dll",
                "KPatchLauncher.runtimeconfig.json",
                "KPatchLauncher.deps.json",
                "KPatchCore.dll",
                "Tomlyn.dll",
                "addresses.toml",
                "addresses.db",
                "sqlite3.dll"
            };

            // Remove each file using safe delete helper
            foreach (var fileName in filesToRemove)
            {
                SafeDeleteFile(gameDir, fileName, removedFiles, messages);
            }

            // Step 4: Verify clean state
            messages.Add("Step 4/4: Verifying clean state...");
            var configPath = Path.Combine(gameDir, "patch_config.toml");
            var isClean = !File.Exists(configPath) && !Directory.Exists(patchesDir);

            if (isClean)
            {
                messages.Add("  Game is now patch-free");
            }
            else
            {
                messages.Add("  ⚠️ Warning: Some patch files may remain");
            }

            return new RemovalResult
            {
                Success = true,
                RemovedFiles = removedFiles,
                BackupRestored = backupResult.Success && backupResult.Data != null,
                Messages = messages
            };
        }
        catch (Exception ex)
        {
            return new RemovalResult
            {
                Success = false,
                Error = $"Removal failed: {ex.Message}",
                RemovedFiles = removedFiles,
                Messages = messages
            };
        }
    }

    /// <summary>
    /// Removes specific patches (not supported in MVP - removes all patches)
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="patchIds">Patch IDs to remove (ignored in MVP)</param>
    /// <returns>Removal result</returns>
    public static RemovalResult RemovePatches(string gameExePath, List<string> patchIds)
    {
        // For MVP, we don't support selective removal
        // Always remove all patches and restore backup
        var result = RemoveAllPatches(gameExePath);

        if (result.Success)
        {
            result.Messages.Insert(0,
                "Note: Selective patch removal not supported in MVP. Removed all patches.");
        }

        return result;
    }

    /// <summary>
    /// Checks if a game has patches installed
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Result indicating if patches are installed</returns>
    public static PatchResult<bool> HasPatchesInstalled(string gameExePath)
    {
        if (!File.Exists(gameExePath))
        {
            return PatchResult<bool>.Fail($"Game executable not found: {gameExePath}");
        }

        var gameDir = Path.GetDirectoryName(gameExePath);
        if (gameDir == null)
        {
            return PatchResult<bool>.Fail("Invalid game executable path");
        }

        try
        {
            // Check for patch_config.toml
            var configPath = Path.Combine(gameDir, "patch_config.toml");
            var hasConfig = File.Exists(configPath);

            // Check for patches directory
            var patchesDir = Path.Combine(gameDir, "patches");
            var hasPatchesDir = Directory.Exists(patchesDir) &&
                                Directory.GetFiles(patchesDir, "*.dll").Length > 0;

            // Check if launcher exists
            var launcherPath = Path.Combine(gameDir, "KPatchLauncher.exe");
            var hasLauncher = File.Exists(launcherPath);

            var isPatched = hasConfig || hasPatchesDir || hasLauncher;

            var details = new List<string>();
            if (hasConfig) details.Add("patch_config.toml exists");
            if (hasPatchesDir) details.Add("patches directory exists");
            if (hasLauncher) details.Add("launcher exists");

            var message = isPatched
                ? $"Patches installed ({string.Join(", ", details)})"
                : "No patches detected";

            return PatchResult<bool>.Ok(isPatched, message);
        }
        catch (Exception ex)
        {
            return PatchResult<bool>.Fail($"Failed to check patch status: {ex.Message}");
        }
    }

    /// <summary>
    /// Gets information about installed patches
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Result containing installation info</returns>
    public static PatchResult<InstallationInfo> GetInstallationInfo(string gameExePath)
    {
        if (!File.Exists(gameExePath))
        {
            return PatchResult<InstallationInfo>.Fail($"Game executable not found: {gameExePath}");
        }

        var gameDir = Path.GetDirectoryName(gameExePath);
        if (gameDir == null)
        {
            return PatchResult<InstallationInfo>.Fail("Invalid game executable path");
        }

        try
        {
            var info = new InstallationInfo
            {
                GameExePath = gameExePath
            };

            // Check for backup
            var backupResult = BackupManager.FindLatestBackup(gameExePath);
            if (backupResult.Success && backupResult.Data != null)
            {
                info.HasBackup = true;
                info.BackupPath = backupResult.Data.BackupPath;
                info.BackupDate = backupResult.Data.CreatedAt;
                info.InstalledPatches = backupResult.Data.InstalledPatches;
            }

            // Check for patches directory
            var patchesDir = Path.Combine(gameDir, "patches");
            if (Directory.Exists(patchesDir))
            {
                info.PatchDlls = Directory.GetFiles(patchesDir, "*.dll")
                    .Select(Path.GetFileName)
                    .Where(name => name != null)
                    .Cast<string>()
                    .ToList();
            }

            // Check for config file
            var configPath = Path.Combine(gameDir, "patch_config.toml");
            info.HasConfig = File.Exists(configPath);
            if (info.HasConfig)
            {
                info.ConfigPath = configPath;
            }

            // Check for launcher
            var launcherPath = Path.Combine(gameDir, "KPatchLauncher.exe");
            info.LauncherInstalled = File.Exists(launcherPath);

            return PatchResult<InstallationInfo>.Ok(info);
        }
        catch (Exception ex)
        {
            return PatchResult<InstallationInfo>.Fail($"Failed to get installation info: {ex.Message}");
        }
    }

    /// <summary>
    /// Information about a patch installation
    /// </summary>
    public sealed class InstallationInfo
    {
        /// <summary>
        /// Path to the game executable
        /// </summary>
        public required string GameExePath { get; init; }

        /// <summary>
        /// Whether a backup exists
        /// </summary>
        public bool HasBackup { get; set; }

        /// <summary>
        /// Path to the backup file (if exists)
        /// </summary>
        public string? BackupPath { get; set; }

        /// <summary>
        /// Date the backup was created
        /// </summary>
        public DateTime? BackupDate { get; set; }

        /// <summary>
        /// List of installed patch IDs (from backup metadata)
        /// </summary>
        public List<string> InstalledPatches { get; set; } = new();

        /// <summary>
        /// List of patch DLL files in patches directory
        /// </summary>
        public List<string> PatchDlls { get; set; } = new();

        /// <summary>
        /// Whether patch_config.toml exists
        /// </summary>
        public bool HasConfig { get; set; }

        /// <summary>
        /// Path to patch_config.toml (if exists)
        /// </summary>
        public string? ConfigPath { get; set; }

        /// <summary>
        /// Whether KPatchLauncher.exe is installed
        /// </summary>
        public bool LauncherInstalled { get; set; }

        /// <summary>
        /// Summary string
        /// </summary>
        public string Summary =>
            $"{InstalledPatches.Count} patches, Backup: {(HasBackup ? "YES" : "NO")}, Launcher: {(LauncherInstalled ? "YES" : "NO")}";
    }
}
