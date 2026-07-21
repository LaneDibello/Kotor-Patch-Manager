using KPatchCore.Common;
using KPatchCore.Detectors;
using KPatchCore.Launcher;
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
        /// Path to the KProxy (binkw32.dll) to stage when deploying via proxy
        /// so the game loads KotorPatcher under Wine/Proton.
        /// Ignored on Windows (live injection).
        /// If null on Linux, DLL-based patches won't load under Wine.
        /// </summary>
        public string? ProxyDllPath { get; init; }

        /// <summary>
        /// Path to KotorPatcher.so to stage in the game directory when the detected game
        /// is a native Linux ELF (DeploymentMethod.ElfNeeded). Ignored otherwise.
        /// </summary>
        public string? PatcherSoPath { get; init; }
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

    /// <summary>The patcher runtime module to stage for a deployment method.</summary>
    private readonly record struct PatcherModule(string FileName, string? SourcePath, bool NeedsSqlite);

    /// <summary>Selects the patcher module (and whether sqlite3.dll ships with it) for a deployment.</summary>
    private static PatcherModule ResolvePatcherModule(DeploymentMethod deployment, InstallOptions options) =>
        deployment == DeploymentMethod.ElfNeeded
            ? new PatcherModule("KotorPatcher.so", options.PatcherSoPath, NeedsSqlite: false)
            : new PatcherModule("KotorPatcher.dll", options.PatcherDllPath, NeedsSqlite: true);

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
            var versionResult = GameDetector.DetectVersion(
                options.GameExePath,
                allowManagedInstallState: true,
                requireKnownManagedStateHash: true);
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
            if (versionResult.Messages.Count > 0)
            {
                messages.AddRange(versionResult.Messages.Select(m => $"  {m}"));
            }
            else
            {
                messages.Add($"  Detected: {gameVersion.DisplayName}");
            }

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

            // Load version-specific hooks before validating or applying anything.
            // PatchEntry.Hooks is populated during repository scan and is intentionally not
            // version-specific; using it here causes multi-version static patches to always
            // use whichever hooks file appears first in the archive.
            var hooksByPatch = new Dictionary<string, List<Hook>>();
            foreach (var patchId in options.PatchIds)
            {
                var hooksResult = _repository.LoadHooksForVersion(patchId, gameVersion.Hash);
                if (!hooksResult.Success || hooksResult.Data == null)
                {
                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Failed to load hooks for {patchId}: {hooksResult.Error}",
                        DetectedVersion = gameVersion,
                        Messages = messages
                    };
                }

                hooksByPatch[patchId] = hooksResult.Data;
                foreach (var message in hooksResult.Messages)
                {
                    messages.Add($"  {patchId}: {message}");
                }
            }

            // Check for hook conflicts using only hooks that apply to the detected version.
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

            // Choose how the patcher is loaded for the detected game.
            var deployment = DeploymentPolicy.ForGame(gameVersion);

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

            // Collect all static hooks from all patches for the detected game version.
            var allStaticHooks = new List<Hook>();
            foreach (var patchId in installOrder)
            {
                var hooks = hooksByPatch[patchId];
                var staticHooks = hooks.Where(h => h.Type == HookType.Static).ToList();

                if (staticHooks.Count > 0)
                {
                    allStaticHooks.AddRange(staticHooks);
                    messages.Add($"  {patchId}: {staticHooks.Count} static hook(s)");
                }
            }

            // Apply all static hooks
            if (allStaticHooks.Count > 0)
            {
                // STATIC hooks aren't supported on the native Linux ELF yet.
                if (deployment == DeploymentMethod.ElfNeeded)
                {
                    if (backup != null)
                    {
                        BackupManager.RestoreBackup(backup);
                    }

                    return new InstallResult
                    {
                        Success = false,
                        Error = "STATIC hooks are not yet supported on the native Linux ELF.",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

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

            // Step 4.6: on a native Linux ELF, add KotorPatcher.so to the game's DT_NEEDED list.
            if (deployment == DeploymentMethod.ElfNeeded)
            {
                messages.Add("Step 4.6/8: Adding KotorPatcher.so to the game's DT_NEEDED...");
                var injectResult = ElfInjector.AddNeeded(options.GameExePath, "KotorPatcher.so");
                if (!injectResult.Success)
                {
                    if (backup != null)
                    {
                        BackupManager.RestoreBackup(backup);
                    }

                    return new InstallResult
                    {
                        Success = false,
                        Error = $"Failed to add KotorPatcher.so to the game ELF: {injectResult.Error}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                messages.Add($"  {injectResult.Messages.FirstOrDefault() ?? "DT_NEEDED updated"}");
            }

            // Step 5: Extract patch DLLs (for DETOUR hooks and DLL-only patches)
            messages.Add("Step 5/8: Extracting patch DLLs...");
            var patchesDir = Path.Combine(gameDir, "patches");

            var extractedDlls = new Dictionary<string, string>();
            foreach (var patchId in installOrder)
            {
                var hooks = hooksByPatch[patchId];
                var hasDetourHooks = hooks.Any(h => h.Type == HookType.Detour);
                var hasDllOnlyPatch = hooks.Count == 0; // DLL-only patch (no hooks)

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
                        // SIMPLE, REPLACE, or STATIC-only patch - no DLL required
                        messages.Add($"  Skipped: {patchId} (no DLL required for selected hooks)");
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
                var hooks = hooksByPatch[patchId];

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

            var (moduleName, moduleSource, needsSqlite) = ResolvePatcherModule(deployment, options);
            var destPath = Path.Combine(gameDir, moduleName);
            if (!string.IsNullOrEmpty(moduleSource))
            {
                if (!File.Exists(moduleSource))
                {
                    return new InstallResult
                    {
                        Success = false,
                        Error = $"{moduleName} not found at: {moduleSource}",
                        DetectedVersion = gameVersion,
                        Backup = backup,
                        Messages = messages
                    };
                }

                // If the module is already present, skip the copy step
                if (PathHelpers.SamePath(moduleSource, destPath))
                {
                    messages.Add($"  ✓ {moduleName} already in place (Patch Manager runs from the game directory)");
                }
                else
                {
                    try
                    {
                        File.Copy(moduleSource, destPath, overwrite: true);
                    }
                    catch (Exception ex)
                    {
                        if (backup != null)
                        {
                            BackupManager.RestoreBackup(backup);
                        }

                        return new InstallResult
                        {
                            Success = false,
                            Error = $"Failed to copy {moduleName} to game directory: {ex.Message}",
                            DetectedVersion = gameVersion,
                            Backup = backup,
                            Messages = messages
                        };
                    }

                    messages.Add($"  ✓ Copied {moduleName} to game directory");
                }

                // Stage sqlite3.dll for the DLL deployment only.
                var patcherDir = needsSqlite ? Path.GetDirectoryName(moduleSource) : null;
                if (patcherDir != null)
                {
                    var sqliteDllSource = Path.Combine(patcherDir, "sqlite3.dll");
                    var sqliteDllDest = Path.Combine(gameDir, "sqlite3.dll");
                    if (!File.Exists(sqliteDllSource))
                    {
                        messages.Add($"  ⚠️ Warning: sqlite3.dll not found at: {sqliteDllSource}");
                    }
                    else if (PathHelpers.SamePath(sqliteDllSource, sqliteDllDest))
                    {
                        messages.Add($"  ✓ sqlite3.dll already in place (Patch Manager runs from the game directory)");
                    }
                    else
                    {
                        try
                        {
                            File.Copy(sqliteDllSource, sqliteDllDest, overwrite: true);
                            messages.Add($"  ✓ Copied sqlite3.dll to game directory");
                        }
                        catch (Exception ex)
                        {
                            messages.Add($"  ⚠️ Warning: failed to copy sqlite3.dll: {ex.Message}");
                        }
                    }
                }
            }
            else
            {
                messages.Add($"  ⚠️ Warning: {moduleName} path not provided");
                messages.Add($"  ⚠️ Make sure {moduleName} is in the game directory");
            }

            // Fail loudly if the module never made it to the destination
            if (!File.Exists(destPath))
            {
                if (backup != null)
                {
                    BackupManager.RestoreBackup(backup);
                }

                return new InstallResult
                {
                    Success = false,
                    Error = $"{moduleName} is missing from the game directory after installation.",
                    DetectedVersion = gameVersion,
                    Backup = backup,
                    Messages = messages
                };
            }

            // Step 7.5: for the proxy deployment method, stage the KProxy so
            // the game loads KotorPatcher when it starts (injection does not).
            var proxyInstalled = false;
            if (deployment == DeploymentMethod.Proxy)
            {
                if (string.IsNullOrEmpty(options.ProxyDllPath))
                {
                    // Without the proxy staged nothing loads KotorPatcher, so DLL
                    // (DETOUR) patches won't take effect. Static patches still do.
                    messages.Add($"  ⚠️ Warning: KProxy (binkw32.dll) not found next to the launcher");
                    messages.Add($"  ⚠️ DLL patches won't load under Wine; static patches still apply");
                }
                else
                {
                    var proxyResult = KProxyInstaller.Install(gameDir, options.ProxyDllPath);
                    if (!proxyResult.Success)
                    {
                        // Undo any half-done rename before rolling back the executable.
                        KProxyInstaller.Uninstall(gameDir);
                        if (backup != null)
                        {
                            BackupManager.RestoreBackup(backup);
                        }

                        return new InstallResult
                        {
                            Success = false,
                            Error = $"KProxy install failed: {proxyResult.Error}",
                            DetectedVersion = gameVersion,
                            Backup = backup,
                            Messages = messages
                        };
                    }

                    proxyInstalled = true;
                    messages.Add($"  ✓ {proxyResult.Messages.FirstOrDefault()}");
                }
            }

            var stateResult = InstallStateManager.SaveOrUpdate(
                options.GameExePath,
                gameVersion,
                installOrder,
                proxyInstalled,
                elfNeededInstalled: deployment == DeploymentMethod.ElfNeeded);
            if (stateResult.Success)
            {
                messages.Add($"  {stateResult.Messages.FirstOrDefault() ?? "Managed install state saved"}");
            }
            else
            {
                messages.Add($"  ⚠️ Warning: Failed to save managed install state: {stateResult.Error}");
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
