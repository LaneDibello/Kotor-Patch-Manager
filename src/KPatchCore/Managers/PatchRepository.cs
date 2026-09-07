using System.IO.Compression;
using KPatchCore.Launcher;
using KPatchCore.Models;
using KPatchCore.Parsers;

namespace KPatchCore.Managers;

/// <summary>
/// Manages a collection of .kpatch files and provides access to patch metadata
/// </summary>
public class PatchRepository
{
    private readonly string _patchesDirectory;
    private readonly Dictionary<string, PatchEntry> _patches = new();

    /// <summary>
    /// Represents a patch in the repository
    /// </summary>
    public sealed class PatchEntry
    {
        /// <summary>
        /// Patch manifest metadata
        /// </summary>
        public required PatchManifest Manifest { get; init; }

        /// <summary>
        /// Path to the .kpatch file
        /// </summary>
        public required string KPatchPath { get; init; }

        /// <summary>
        /// Hooks defined in this patch
        /// </summary>
        public required List<Hook> Hooks { get; init; }

        /// <summary>
        /// Whether this patch is currently loaded
        /// </summary>
        public bool IsLoaded { get; set; }
    }

    /// <summary>
    /// Creates a new patch repository
    /// </summary>
    /// <param name="patchesDirectory">Directory containing .kpatch files</param>
    public PatchRepository(string patchesDirectory)
    {
        _patchesDirectory = patchesDirectory ?? throw new ArgumentNullException(nameof(patchesDirectory));
    }

    /// <summary>
    /// Scans the patches directory and loads all .kpatch files
    /// </summary>
    /// <returns>Result indicating success or failure</returns>
    public PatchResult ScanPatches()
    {
        if (!Directory.Exists(_patchesDirectory))
        {
            return PatchResult.Fail($"Patches directory not found: {_patchesDirectory}");
        }

        _patches.Clear();
        var kpatchFiles = Directory.GetFiles(_patchesDirectory, "*.kpatch", SearchOption.TopDirectoryOnly);
        var loadedCount = 0;
        var errors = new List<string>();

        foreach (var kpatchPath in kpatchFiles)
        {
            Console.WriteLine($"DEBUG: Loading {kpatchPath}");
            var loadResult = LoadPatch(kpatchPath);
            if (loadResult.Success && loadResult.Data != null)
            {
                _patches[loadResult.Data.Manifest.Id] = loadResult.Data;
                loadedCount++;
            }
            else
            {
                errors.Add($"{Path.GetFileName(kpatchPath)}: {loadResult.Error}");
            }
        }

        var result = PatchResult.Ok($"Loaded {loadedCount} patches from {_patchesDirectory}");

        if (errors.Count > 0)
        {
            errors.ForEach(e => Console.WriteLine(e));
            result.WithMessage($"Failed to load {errors.Count} patches:");
            foreach (var error in errors)
            {
                result.WithMessage($"  - {error}");
            }
        }

        return result;
    }

    /// <summary>
    /// Loads a single .kpatch file
    /// </summary>
    /// <param name="kpatchPath">Path to the .kpatch file</param>
    /// <returns>Result containing PatchEntry or error</returns>
    public PatchResult<PatchEntry> LoadPatch(string kpatchPath)
    {
        if (!File.Exists(kpatchPath))
        {
            return PatchResult<PatchEntry>.Fail($"Patch file not found: {kpatchPath}");
        }

        try
        {
            using var archive = ZipFile.OpenRead(kpatchPath);

            // Load manifest.toml
            var manifestEntry = archive.GetEntry("manifest.toml");
            if (manifestEntry == null)
            {
                return PatchResult<PatchEntry>.Fail("Missing manifest.toml in patch archive");
            }

            PatchManifest manifest;
            using (var stream = manifestEntry.Open())
            using (var reader = new StreamReader(stream))
            {
                var manifestContent = reader.ReadToEnd();
                var parseResult = ManifestParser.ParseString(manifestContent);
                if (!parseResult.Success || parseResult.Data == null)
                {
                    return PatchResult<PatchEntry>.Fail($"Failed to parse manifest: {parseResult.Error}");
                }
                manifest = parseResult.Data;
            }

            // Load hooks - for repository scan, we just look for any hooks files
            // The actual version-specific loading happens in LoadHooksForVersion()
            // Match pattern: *hooks.toml (e.g., "hooks.toml", "gog.hooks.toml", "steam.hooks.toml")
            var hooksEntries = archive.Entries
                .Where(e => e.FullName.EndsWith("hooks.toml", StringComparison.OrdinalIgnoreCase))
                .ToList();

            if (hooksEntries.Count == 0)
            {
                // No hooks files - could be DLL-only patch
                var patchEntry = new PatchEntry
                {
                    Manifest = manifest,
                    KPatchPath = kpatchPath,
                    Hooks = new List<Hook>(), // Empty hooks list
                    IsLoaded = false
                };

                return PatchResult<PatchEntry>.Ok(patchEntry, $"Loaded patch: {manifest.Id} (no hooks)");
            }

            // For initial repository scan, parse all hooks files to get a complete
            // patch summary. Version-specific filtering still happens during installation
            // in LoadHooksForVersion().
            var hooks = new List<Hook>();
            foreach (var hooksEntry in hooksEntries)
            {
                using var stream = hooksEntry.Open();
                using var reader = new StreamReader(stream);
                var hooksContent = reader.ReadToEnd();
                var parseResult = HooksParser.ParseString(hooksContent);
                if (!parseResult.Success || parseResult.Data == null)
                {
                    return PatchResult<PatchEntry>.Fail(
                        $"Failed to parse hooks file '{hooksEntry.FullName}': {parseResult.Error}");
                }

                hooks.AddRange(parseResult.Data);
            }

            // Check if any version of this patch has DETOUR hooks (which require a DLL)
            var hasDetourHooks = hooks.Any(h => h.Type == HookType.Detour);

            // Verify binary exists only if DETOUR hooks are present
            if (hasDetourHooks)
            {
                // The scan runs before a game is selected, so which binary is needed is not yet
                // known. Any of them satisfies the check here; the install asks for the one the
                // detected game's platform needs and fails there if it is missing.
                var hasBinary = DeploymentPolicy.AllPatchBinaryFileNames.Any(
                    name => FindBinaryEntry(archive, name) != null);

                if (!hasBinary)
                {
                    var expected = string.Join(" or ", DeploymentPolicy.AllPatchBinaryFileNames);
                    return PatchResult<PatchEntry>.Fail(
                        $"Missing binaries/{expected} in patch archive (required for DETOUR hooks)");
                }
            }

            var entry = new PatchEntry
            {
                Manifest = manifest,
                KPatchPath = kpatchPath,
                Hooks = hooks,
                IsLoaded = false
            };

            return PatchResult<PatchEntry>.Ok(entry, $"Loaded patch: {manifest.Id}");
        }
        catch (Exception ex)
        {
            return PatchResult<PatchEntry>.Fail($"Failed to load patch: {ex.Message}");
        }
    }

    /// <summary>
    /// Gets a patch by ID
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <returns>Result containing PatchEntry or error</returns>
    public PatchResult<PatchEntry> GetPatch(string patchId)
    {
        if (_patches.TryGetValue(patchId, out var patch))
        {
            return PatchResult<PatchEntry>.Ok(patch);
        }

        return PatchResult<PatchEntry>.Fail($"Patch not found: {patchId}");
    }

    /// <summary>
    /// Gets all available patches
    /// </summary>
    /// <returns>Dictionary of patch ID to PatchEntry</returns>
    public IReadOnlyDictionary<string, PatchEntry> GetAllPatches()
    {
        return _patches;
    }

    /// <summary>
    /// Extracts the patch's runtime module for a game's platform. The extracted file keeps the
    /// archived binary's extension, which is what patch_config.toml records and the engine loads,
    /// so a native game gets a .so or .dylib rather than something named .dll.
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <param name="targetDirectory">Where to extract the module</param>
    /// <param name="gameVersion">The game being patched</param>
    /// <returns>Result containing path to the extracted module or error</returns>
    public PatchResult<string> ExtractPatchDll(string patchId, string targetDirectory, GameVersion gameVersion)
    {
        var patchResult = GetPatch(patchId);
        if (!patchResult.Success || patchResult.Data == null)
        {
            return PatchResult<string>.Fail(patchResult.Error ?? "Patch not found");
        }

        var patch = patchResult.Data;

        try
        {
            using var archive = ZipFile.OpenRead(patch.KPatchPath);

            var binaryName = DeploymentPolicy.PatchBinaryFileName(gameVersion);
            var binaryEntry = FindBinaryEntry(archive, binaryName);

            if (binaryEntry == null)
            {
                // Not an error - patch may carry no module at all (SIMPLE patch), which the caller
                // tells apart from a patch that needs one for this platform and does not ship it.
                return PatchResult<string>.Fail($"No binaries/{binaryName} found in patch archive");
            }

            Directory.CreateDirectory(targetDirectory);
            var targetName = $"{patchId}{Path.GetExtension(binaryName)}";
            var targetPath = Path.Combine(targetDirectory, targetName);

            using (var sourceStream = binaryEntry.Open())
            using (var targetStream = File.Create(targetPath))
            {
                sourceStream.CopyTo(targetStream);
            }

            return PatchResult<string>.Ok(targetPath, $"Extracted {targetName}");
        }
        catch (Exception ex)
        {
            return PatchResult<string>.Fail($"Failed to extract patch binary: {ex.Message}");
        }
    }

    /// <summary>
    /// Finds a binary in the archive, accepting either separator. A .kpatch built on Windows can
    /// carry backslash entry names.
    /// </summary>
    private static ZipArchiveEntry? FindBinaryEntry(ZipArchive archive, string binaryName) =>
        archive.GetEntry($"binaries/{binaryName}") ?? archive.GetEntry($"binaries\\{binaryName}");

    /// <summary>
    /// Gets patches that match specific criteria
    /// </summary>
    /// <param name="predicate">Filter function</param>
    /// <returns>List of matching patches</returns>
    public List<PatchEntry> FindPatches(Func<PatchEntry, bool> predicate)
    {
        return _patches.Values.Where(predicate).ToList();
    }

    /// <summary>
    /// Gets the number of patches in the repository
    /// </summary>
    public int PatchCount => _patches.Count;

    /// <summary>
    /// Checks if a patch exists in the repository
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <returns>True if patch exists, false otherwise</returns>
    public bool HasPatch(string patchId) => _patches.ContainsKey(patchId);

    /// <summary>
    /// Loads hooks for a specific game version by finding and merging ALL matching hooks files
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <param name="targetVersionSha">SHA-256 hash of target game version</param>
    /// <returns>Result containing list of hooks or error</returns>
    public PatchResult<List<Hook>> LoadHooksForVersion(string patchId, string targetVersionSha)
    {
        var patchResult = GetPatch(patchId);
        if (!patchResult.Success || patchResult.Data == null)
        {
            return PatchResult<List<Hook>>.Fail(patchResult.Error ?? "Patch not found");
        }

        var patch = patchResult.Data;

        try
        {
            using var archive = ZipFile.OpenRead(patch.KPatchPath);

            // Find ALL hooks files in the archive (pattern: *hooks.toml)
            var hooksEntries = archive.Entries
                .Where(e => e.FullName.EndsWith("hooks.toml", StringComparison.OrdinalIgnoreCase))
                .ToList();

            if (hooksEntries.Count == 0)
            {
                // No hooks files - DLL-only patch
                return PatchResult<List<Hook>>.Ok(new List<Hook>(), "No hooks files (DLL-only patch)");
            }

            // Extract hooks files to temp directory for parsing
            var tempDir = Path.Combine(Path.GetTempPath(), $"kpatch_{Guid.NewGuid():N}");
            Directory.CreateDirectory(tempDir);

            var allHooks = new List<Hook>();
            var matchedFiles = new List<string>();

            try
            {
                foreach (var entry in hooksEntries)
                {
                    // Extract to temp
                    var tempPath = Path.Combine(tempDir, Path.GetFileName(entry.FullName));
                    using (var sourceStream = entry.Open())
                    using (var targetStream = File.Create(tempPath))
                    {
                        sourceStream.CopyTo(targetStream);
                    }

                    // Parse metadata to check if this file applies to our version
                    var metadata = HooksParser.ParseMetadata(tempPath);

                    // Include if: no target_versions specified OR our SHA is in the list
                    if (metadata.TargetVersions.Count == 0 ||
                        metadata.TargetVersions.Any(v => v.Equals(targetVersionSha, StringComparison.OrdinalIgnoreCase)))
                    {
                        // This file applies to our version - parse and merge hooks
                        var parseResult = HooksParser.ParseFile(tempPath);
                        if (!parseResult.Success || parseResult.Data == null)
                        {
                            return PatchResult<List<Hook>>.Fail(
                                $"Failed to parse matching hooks file '{entry.FullName}': {parseResult.Error}");
                        }

                        allHooks.AddRange(parseResult.Data);
                        matchedFiles.Add(entry.FullName);
                    }
                }

                if (allHooks.Count == 0 && matchedFiles.Count == 0)
                {
                    return PatchResult<List<Hook>>.Fail(
                        $"No hooks files found for version {targetVersionSha.Substring(0, 16)}...");
                }

                var message = $"Loaded {allHooks.Count} hook(s) from {matchedFiles.Count} file(s): " +
                             string.Join(", ", matchedFiles);

                return PatchResult<List<Hook>>.Ok(allHooks, message);
            }
            finally
            {
                // Cleanup temp directory
                try
                {
                    Directory.Delete(tempDir, true);
                }
                catch
                {
                    // Ignore cleanup errors
                }
            }
        }
        catch (Exception ex)
        {
            return PatchResult<List<Hook>>.Fail($"Failed to load hooks for version: {ex.Message}");
        }
    }
}
