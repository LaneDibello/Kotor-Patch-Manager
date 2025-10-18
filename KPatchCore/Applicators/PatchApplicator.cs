using KPatchCore.Common;
using KPatchCore.Detectors;
using KPatchCore.Managers;
using KPatchCore.Models;
using KPatchCore.Validators;

namespace KPatchCore.Applicators;

/// <summary>
/// Orchestrates the full patch installation process
/// </summary>
public class PatchApplicator
{
    private readonly PatchRepository _repository;

    /// <summary>
    /// Installation options
    /// </summary>
    public sealed class InstallOptions
    {
        /// <summary>
        /// Path to the game executable
        /// </summary>
        public required string GameExePath { get; init; }

        /// <summary>
        /// Patch IDs to install
        /// </summary>
        public required List<string> PatchIds { get; init; }

        /// <summary>
        /// Whether to create a backup before installation
        /// </summary>
        public bool CreateBackup { get; init; } = true;

        /// <summary>
        /// Path to KotorPatcher.dll (if null, assumes it's in same directory as game exe)
        /// </summary>
        public string? PatcherDllPath { get; init; }

        /// <summary>
        /// Path to KPatchLauncher.exe to copy to game directory (if null, no launcher copied)
        /// </summary>
        public string? LauncherExePath { get; init; }

        /// <summary>
        /// Whether to copy the launcher to the game directory
        /// </summary>
        public bool CopyLauncher { get; init; } = true;
    }

    /// <summary>
    /// Installation result with detailed information
    /// </summary>
    public sealed class InstallResult
    {
        /// <summary>
        /// Whether installation succeeded
        /// </summary>
        public required bool Success { get; init; }

        /// <summary>
        /// Error message if failed
        /// </summary>
        public string? Error { get; init; }

        /// <summary>
        /// List of patches that were installed
        /// </summary>
        public List<string> InstalledPatches { get; init; } = new();

        /// <summary>
        /// Backup information (if backup was created)
        /// </summary>
        public BackupInfo? Backup { get; init; }

        /// <summary>
        /// Detected game version
        /// </summary>
        public GameVersion? DetectedVersion { get; init; }

        /// <summary>
        /// Path to generated patch_config.toml
        /// </summary>
        public string? ConfigPath { get; init; }

        /// <summary>
        /// Additional messages
        /// </summary>
        public List<string> Messages { get; init; } = new();
    }

    public PatchApplicator(PatchRepository repository)
    {
        _repository = repository ?? throw new ArgumentNullException(nameof(repository));
    }

    /// <summary>
    /// Installs patches to a game
    /// </summary>
    /// <param name="options">Installation options</param>
    /// <returns>Installation result</returns>
    public InstallResult InstallPatches(InstallOptions options)
    {
        var messages = new List<string>();
        BackupInfo? backup = null;

        try
        {
            // Step 1: Validate inputs
            messages.Add("Step 1/7: Validating inputs...");
            if (!File.Exists(options.GameExePath))
            {
                return new InstallResult
                {
                    Success = false,
                    Error = $"Game executable not found: {options.GameExePath}",
                    Messages = messages
                };
            }

            var gameDir = Path.GetDirectoryName(options.GameExePath);
            if (gameDir == null)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = "Invalid game executable path",
                    Messages = messages
                };
            }

            // Step 2: Detect game version
            messages.Add("Step 2/7: Detecting game version...");
            var versionResult = GameDetector.DetectVersion(options.GameExePath);
            if (!versionResult.Success || versionResult.Data == null)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = $"Failed to detect game version: {versionResult.Error}",
                    Messages = messages
                };
            }

            var gameVersion = versionResult.Data;
            messages.Add($"  Detected: {gameVersion.DisplayName}");

            // Step 3: Load and validate patches
            messages.Add("Step 3/7: Loading and validating patches...");
            var patchEntries = new Dictionary<string, PatchRepository.PatchEntry>();
            foreach (var patchId in options.PatchIds)
            {
                var patchResult = _repository.GetPatch(patchId);
                if (!patchResult.Success || patchResult.Data == null)
                {
                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Patch not found: {patchId}",
                        DetectedVersion = gameVersion,
                        Messages = messages
                    };
                }

                patchEntries[patchId] = patchResult.Data;
            }

            // Validate all patches
            var manifests = patchEntries.Values.Select(e => e.Manifest).ToList();
            var patchDict = patchEntries.ToDictionary(kv => kv.Key, kv => kv.Value.Manifest);

            // Check dependencies
            var depResult = DependencyValidator.ValidateDependencies(patchDict, options.PatchIds);
            if (!depResult.Success)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = depResult.Error,
                    DetectedVersion = gameVersion,
                    Messages = messages
                };
            }

            // Check for conflicts
            var conflictResult = DependencyValidator.ValidateNoConflicts(patchDict, options.PatchIds);
            if (!conflictResult.Success)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = conflictResult.Error,
                    DetectedVersion = gameVersion,
                    Messages = messages
                };
            }

            // Check game version compatibility
            var versionCheckResult = GameVersionValidator.ValidateAllPatchesSupported(manifests, gameVersion);
            if (!versionCheckResult.Success)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = versionCheckResult.Error,
                    DetectedVersion = gameVersion,
                    Messages = messages
                };
            }

            // Check for hook conflicts
            var hooksByPatch = patchEntries.ToDictionary(kv => kv.Key, kv => kv.Value.Hooks);
            var hookConflictResult = HookValidator.ValidateMultiPatchHooks(hooksByPatch);
            if (!hookConflictResult.Success)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = hookConflictResult.Error,
                    DetectedVersion = gameVersion,
                    Messages = messages
                };
            }

            // Calculate install order
            var orderResult = DependencyValidator.CalculateInstallOrder(patchDict, options.PatchIds);
            if (!orderResult.Success || orderResult.Data == null)
            {
                return new InstallResult
                {
                    Success = false,
                    Error = $"Failed to calculate install order: {orderResult.Error}",
                    DetectedVersion = gameVersion,
                    Messages = messages
                };
            }

            var installOrder = orderResult.Data;
            messages.Add($"  Install order: {string.Join(" -> ", installOrder)}");

            // Step 4: Create backup
            if (options.CreateBackup)
            {
                messages.Add("Step 4/7: Creating backup...");
                var backupResult = BackupManager.CreateBackup(
                    options.GameExePath,
                    gameVersion,
                    options.PatchIds
                );

                if (!backupResult.Success || backupResult.Data == null)
                {
                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Failed to create backup: {backupResult.Error}",
                        DetectedVersion = gameVersion,
                        Messages = messages
                    };
                }

                backup = backupResult.Data;
                messages.Add($"  Backup created: {Path.GetFileName(backup.BackupPath)}");
            }
            else
            {
                messages.Add("Step 4/7: Skipping backup (disabled)");
            }

            // Step 5: Extract patch DLLs
            messages.Add("Step 5/7: Extracting patch DLLs...");
            var patchesDir = Path.Combine(gameDir, "patches");
            Directory.CreateDirectory(patchesDir);

            var extractedDlls = new Dictionary<string, string>();
            foreach (var patchId in installOrder)
            {
                var extractResult = _repository.ExtractPatchDll(patchId, patchesDir);
                if (!extractResult.Success || extractResult.Data == null)
                {
                    // Cleanup and restore backup on failure
                    if (backup != null)
                    {
                        BackupManager.RestoreBackup(backup);
                    }

                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Failed to extract {patchId}: {extractResult.Error}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                extractedDlls[patchId] = extractResult.Data;
                messages.Add($"  Extracted: {patchId}.dll");
            }

            // Step 6: Generate patch_config.toml
            messages.Add("Step 6/7: Generating patch_config.toml...");
            var config = new PatchConfig();
            foreach (var patchId in installOrder)
            {
                var entry = patchEntries[patchId];
                var dllPath = Path.GetRelativePath(gameDir, extractedDlls[patchId]);
                config.AddPatch(patchId, dllPath, entry.Hooks);
            }

            var configPath = Path.Combine(gameDir, "patch_config.toml");
            var configResult = ConfigGenerator.GenerateConfigFile(config, configPath);
            if (!configResult.Success)
            {
                // Cleanup on failure
                if (backup != null)
                {
                    BackupManager.RestoreBackup(backup);
                }

                return new InstallResult
                {
                    Success = false,
                    Error = $"Failed to generate config: {configResult.Error}",
                    DetectedVersion = gameVersion,
                    Backup = backup,
                    Messages = messages
                };
            }

            messages.Add($"  Config generated: patch_config.toml");

            // Step 7: Copy patcher DLL and launcher
            messages.Add("Step 7/7: Installing patcher and launcher...");

            // Copy KotorPatcher.dll to game directory if path provided
            if (!string.IsNullOrEmpty(options.PatcherDllPath))
            {
                var destPath = Path.Combine(gameDir, "KotorPatcher.dll");
                File.Copy(options.PatcherDllPath, destPath, overwrite: true);
                messages.Add($"  Copied KotorPatcher.dll to game directory");
            }
            else
            {
                messages.Add($"  ⚠️ Warning: KotorPatcher.dll path not provided");
                messages.Add($"  ⚠️ Make sure KotorPatcher.dll is in game directory");
            }

            // Copy KPatchLauncher.exe and its dependencies to game directory if requested
            if (options.CopyLauncher && !string.IsNullOrEmpty(options.LauncherExePath))
            {
                var launcherSourceDir = Path.GetDirectoryName(options.LauncherExePath);
                if (launcherSourceDir == null || !Directory.Exists(launcherSourceDir))
                {
                    messages.Add($"  ⚠️ Warning: Launcher directory not found, skipping launcher copy");
                    messages.Add($"  ⚠️ You will need to use an external DLL injector");
                }
                else
                {
                    // Copy all necessary launcher files (KPatchLauncher.exe needs its dependencies)
                    var launcherFiles = new[]
                    {
                        "KPatchLauncher.exe",
                        "KPatchLauncher.dll",
                        "KPatchLauncher.runtimeconfig.json",
                        "KPatchLauncher.deps.json",
                        "KPatchCore.dll",
                        "Tomlyn.dll"
                    };

                    var copiedCount = 0;
                    foreach (var fileName in launcherFiles)
                    {
                        var sourcePath = Path.Combine(launcherSourceDir, fileName);
                        if (File.Exists(sourcePath))
                        {
                            var destPath = Path.Combine(gameDir, fileName);
                            File.Copy(sourcePath, destPath, overwrite: true);
                            copiedCount++;
                        }
                    }

                    messages.Add($"  Copied {copiedCount} launcher files to game directory");
                    messages.Add($"  ✓ Run KPatchLauncher.exe to start the game with patches");
                }
            }
            else if (options.CopyLauncher)
            {
                messages.Add($"  ⚠️ Warning: Launcher path not provided, skipping launcher copy");
                messages.Add($"  ⚠️ You will need to use an external DLL injector");
            }
            else
            {
                messages.Add($"  Skipping launcher copy (disabled)");
            }

            return new InstallResult
            {
                Success = true,
                InstalledPatches = installOrder,
                Backup = backup,
                DetectedVersion = gameVersion,
                ConfigPath = configPath,
                Messages = messages
            };
        }
        catch (Exception ex)
        {
            // Restore backup on unexpected failure
            if (backup != null)
            {
                try
                {
                    BackupManager.RestoreBackup(backup);
                    messages.Add("Restored backup after failure");
                }
                catch
                {
                    // Ignore backup restoration errors
                }
            }

            return new InstallResult
            {
                Success = false,
                Error = $"Installation failed: {ex.Message}",
                Backup = backup,
                Messages = messages
            };
        }
    }
}
