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

            // Step 4.5: Apply STATIC hooks to executable
            messages.Add("Step 4.5/8: Applying static hooks...");

            // Collect all static hooks from all patches
            var allStaticHooks = new List<Hook>();
            foreach (var patchId in installOrder)
            {
                var entry = patchEntries[patchId];
                var staticHooks = entry.Hooks.Where(h => h.Type == HookType.Static).ToList();

                if (staticHooks.Count > 0)
                {
                    allStaticHooks.AddRange(staticHooks);
                    messages.Add($"  {patchId}: {staticHooks.Count} static hook(s)");
                }
            }

            // Apply all static hooks
            if (allStaticHooks.Count > 0)
            {
                var applyResult = StaticHookApplicator.ApplyStaticHooks(
                    options.GameExePath,
                    allStaticHooks);

                if (!applyResult.Success)
                {
                    // Restore backup on failure
                    if (backup != null)
                    {
                        var restoreResult = BackupManager.RestoreBackup(backup);
                        if (restoreResult.Success)
                        {
                            messages.Add("  Backup restored after static hook failure");
                        }
                    }

                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Static hook application failed: {applyResult.Error}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                messages.Add($"  Successfully applied {allStaticHooks.Count} static hook(s) to executable");
            }
            else
            {
                messages.Add("  No static hooks to apply");
            }

            // Step 5: Extract patch DLLs (for DETOUR hooks and DLL-only patches)
            messages.Add("Step 5/8: Extracting patch DLLs...");
            var patchesDir = Path.Combine(gameDir, "patches");

            var extractedDlls = new Dictionary<string, string>();
            foreach (var patchId in installOrder)
            {
                var entry = patchEntries[patchId];
                var hasDetourHooks = entry.Hooks.Any(h => h.Type == HookType.Detour);
                var hasDllOnlyPatch = entry.Hooks.Count == 0; // DLL-only patch (no hooks)

                // Try to extract DLL if it exists (supports DETOUR hooks and DLL-only patches)
                var extractResult = _repository.ExtractPatchDll(patchId, patchesDir);

                if (extractResult.Success && extractResult.Data != null)
                {
                    // DLL exists and was extracted successfully
                    Directory.CreateDirectory(patchesDir);
                    extractedDlls[patchId] = extractResult.Data;

                    if (hasDetourHooks)
                    {
                        messages.Add($"  Extracted: {patchId}.dll (DETOUR hooks)");
                    }
                    else if (hasDllOnlyPatch)
                    {
                        messages.Add($"  Extracted: {patchId}.dll (DLL-only patch)");
                    }
                    else
                    {
                        messages.Add($"  Extracted: {patchId}.dll");
                    }
                }
                else
                {
                    // No DLL in archive
                    if (hasDetourHooks)
                    {
                        // DETOUR hooks require a DLL - this is an error
                        if (backup != null)
                        {
                            BackupManager.RestoreBackup(backup);
                        }

                        return new InstallResult
                        {
                            Success = false,
                            Error = $"Failed to extract {patchId}: {extractResult.Error} (DETOUR hooks require DLL)",
                            DetectedVersion = gameVersion,
                            Backup = backup,
                            Messages = messages
                        };
                    }
                    else
                    {
                        // SIMPLE patch - no DLL required
                        messages.Add($"  Skipped: {patchId} (SIMPLE patch, no DLL)");
                    }
                }
            }

            // Step 6: Generate patch_config.toml
            messages.Add("Step 6/8: Generating patch_config.toml...");
            var config = new PatchConfig
            {
                TargetVersionSha = gameVersion.Hash  // NEW: Set target version SHA
            };

            foreach (var patchId in installOrder)
            {
                // Load version-specific hooks for this patch
                var hooksResult = _repository.LoadHooksForVersion(patchId, gameVersion.Hash);
                if (!hooksResult.Success || hooksResult.Data == null)
                {
                    // Cleanup on failure
                    if (backup != null)
                    {
                        BackupManager.RestoreBackup(backup);
                    }

                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Failed to load hooks for {patchId}: {hooksResult.Error}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                var hooks = hooksResult.Data;
                messages.Add($"  {patchId}: {hooksResult.Data}");

                // Get DLL path if this patch has DETOUR hooks, otherwise use empty string
                var dllPath = extractedDlls.ContainsKey(patchId)
                    ? Path.GetRelativePath(gameDir, extractedDlls[patchId])
                    : string.Empty;

                config.AddPatch(patchId, dllPath, hooks);
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

            // Step 6.5: Copy address database to game directory
            messages.Add("Step 6.5/8: Copying address database...");

            // Find AddressDatabases directory in same directory as executing assembly
            // (copied there by build system via .csproj)
            var assemblyDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) ?? string.Empty;
            var addressDbSourceDirNormalized = Path.Combine(assemblyDir, "AddressDatabases");

            if (!Directory.Exists(addressDbSourceDirNormalized))
            {
                if (backup != null)
                {
                    BackupManager.RestoreBackup(backup);
                }

                return new InstallResult
                {
                    Success = false,
                    Error = $"Address database directory not found: {addressDbSourceDirNormalized}",
                    DetectedVersion = gameVersion,
                    Backup = backup,
                    Messages = messages
                };
            }

            // Find matching address database by SHA
            var addressDbFiles = Directory.GetFiles(addressDbSourceDirNormalized, "*.db");
            string? matchingAddressDb = null;

            foreach (var dbFile in addressDbFiles)
            {
                // Query SQLite database to check sha256_hash field
                try
                {
                    using var connection = new Microsoft.Data.Sqlite.SqliteConnection($"Data Source={dbFile};Mode=ReadOnly");
                    connection.Open();
                    using var command = connection.CreateCommand();
                    command.CommandText = "SELECT sha256_hash FROM game_version";
                    var reader = command.ExecuteReader();

                    if (reader.HasRows)
                    {
                        while(reader.Read())
                        {
                            var sha = reader.GetString(0);
                            if (sha == gameVersion.Hash)
                            {
                                matchingAddressDb = dbFile;
                                break;
                            }
                        }
                    }
                }
                catch
                {
                    // Skip databases that can't be read
                    continue;
                }
            }

            if (matchingAddressDb == null)
            {
                if (backup != null)
                {
                    BackupManager.RestoreBackup(backup);
                }

                return new InstallResult
                {
                    Success = false,
                    Error = $"No address database found for game version SHA: {gameVersion.Hash.Substring(0, 16)}...",
                    DetectedVersion = gameVersion,
                    Backup = backup,
                    Messages = messages
                };
            }

            // Copy to game directory as addresses.db (generic name)
            var addressDbDest = Path.Combine(gameDir, "addresses.db");
            File.Copy(matchingAddressDb, addressDbDest, overwrite: true);
            messages.Add($"  Copied: {Path.GetFileName(matchingAddressDb)} -> addresses.db");

            // Step 7: Copy patcher DLL and SQLite
            messages.Add("Step 7/8: Installing patcher DLL and dependencies...");

            // Copy KotorPatcher.dll to game directory if path provided
            if (!string.IsNullOrEmpty(options.PatcherDllPath))
            {
                if (!File.Exists(options.PatcherDllPath))
                {
                    return new InstallResult
                    {
                        Success = false,
                        Error = $"KotorPatcher.dll not found at: {options.PatcherDllPath}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                var destPath = Path.Combine(gameDir, "KotorPatcher.dll");
                File.Copy(options.PatcherDllPath, destPath, overwrite: true);
                messages.Add($"  ✓ Copied KotorPatcher.dll to game directory");

                // Copy sqlite3.dll (should be in same directory as KotorPatcher.dll)
                var patcherDir = Path.GetDirectoryName(options.PatcherDllPath);
                if (patcherDir != null)
                {
                    var sqliteDllSource = Path.Combine(patcherDir, "sqlite3.dll");
                    if (File.Exists(sqliteDllSource))
                    {
                        var sqliteDllDest = Path.Combine(gameDir, "sqlite3.dll");
                        File.Copy(sqliteDllSource, sqliteDllDest, overwrite: true);
                        messages.Add($"  ✓ Copied sqlite3.dll to game directory");
                    }
                    else
                    {
                        messages.Add($"  ⚠️ Warning: sqlite3.dll not found at: {sqliteDllSource}");
                    }
                }
            }
            else
            {
                messages.Add($"  ⚠️ Warning: KotorPatcher.dll path not provided");
                messages.Add($"  ⚠️ Make sure KotorPatcher.dll and sqlite3.dll are in game directory");
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
