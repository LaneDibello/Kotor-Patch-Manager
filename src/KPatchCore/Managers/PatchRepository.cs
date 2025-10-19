using System.IO.Compression;
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

            // Load hooks.toml
            var hooksEntry = archive.GetEntry("hooks.toml");
            if (hooksEntry == null)
            {
                return PatchResult<PatchEntry>.Fail("Missing hooks.toml in patch archive");
            }

            List<Hook> hooks;
            using (var stream = hooksEntry.Open())
            using (var reader = new StreamReader(stream))
            {
                var hooksContent = reader.ReadToEnd();
                var parseResult = HooksParser.ParseString(hooksContent);
                if (!parseResult.Success || parseResult.Data == null)
                {
                    return PatchResult<PatchEntry>.Fail($"Failed to parse hooks: {parseResult.Error}");
                }
                hooks = parseResult.Data;
            }

            // Check if this patch has any DETOUR hooks (which require a DLL)
            var hasDetourHooks = hooks.Any(h => h.Type == HookType.Detour);

            // Verify binary exists only if DETOUR hooks are present
            if (hasDetourHooks)
            {
                var binaryPath = "binaries/windows_x86.dll";
                var binaryEntry = archive.GetEntry(binaryPath);

                // Try backslash path if forward slash didn't work (Windows ZIP compatibility)
                if (binaryEntry == null)
                {
                    binaryPath = "binaries\\windows_x86.dll";
                    binaryEntry = archive.GetEntry(binaryPath);
                }

                if (binaryEntry == null)
                {
                    return PatchResult<PatchEntry>.Fail($"Missing binaries/windows_x86.dll in patch archive (required for DETOUR hooks)");
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
    /// Extracts a patch DLL to a target directory
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <param name="targetDirectory">Where to extract the DLL</param>
    /// <returns>Result containing path to extracted DLL or error</returns>
    public PatchResult<string> ExtractPatchDll(string patchId, string targetDirectory)
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

            // Try both forward slash and backslash (Windows ZIP compatibility)
            var binaryPath = "binaries/windows_x86.dll";
            var binaryEntry = archive.GetEntry(binaryPath);

            if (binaryEntry == null)
            {
                binaryPath = "binaries\\windows_x86.dll";
                binaryEntry = archive.GetEntry(binaryPath);
            }

            if (binaryEntry == null)
            {
                return PatchResult<string>.Fail($"Binary not found in archive: binaries/windows_x86.dll");
            }

            Directory.CreateDirectory(targetDirectory);
            var targetPath = Path.Combine(targetDirectory, $"{patchId}.dll");

            using (var sourceStream = binaryEntry.Open())
            using (var targetStream = File.Create(targetPath))
            {
                sourceStream.CopyTo(targetStream);
            }

            return PatchResult<string>.Ok(targetPath, $"Extracted {patchId}.dll");
        }
        catch (Exception ex)
        {
            return PatchResult<string>.Fail($"Failed to extract DLL: {ex.Message}");
        }
    }

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
}
