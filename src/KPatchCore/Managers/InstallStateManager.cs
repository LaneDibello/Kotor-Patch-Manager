using System.Text.Json;
using KPatchCore.Common;
using KPatchCore.Models;

namespace KPatchCore.Managers;

/// <summary>
/// Persists KPM-managed executable identity so intentional STATIC patches do not make the game appear unknown.
/// </summary>
public static class InstallStateManager
{
    public const string StateFileName = "kpm_install_state.json";

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true
    };

    /// <summary>
    /// Gets the install-state path for the directory containing the selected executable.
    /// </summary>
    public static string? GetStatePath(string gameExePath)
    {
        var gameDir = Path.GetDirectoryName(gameExePath);
        return string.IsNullOrWhiteSpace(gameDir)
            ? null
            : Path.Combine(gameDir, StateFileName);
    }

    /// <summary>
    /// Saves or updates the managed identity file for a successful KPM installation.
    /// </summary>
    public static PatchResult<ManagedInstallState> SaveOrUpdate(
        string gameExePath,
        GameVersion originalVersion,
        IEnumerable<string> installedPatches,
        bool kproxyInstalled = false)
    {
        if (string.IsNullOrWhiteSpace(gameExePath) || !File.Exists(gameExePath))
        {
            return PatchResult<ManagedInstallState>.Fail($"Game executable not found: {gameExePath}");
        }

        var statePath = GetStatePath(gameExePath);
        if (string.IsNullOrWhiteSpace(statePath))
        {
            return PatchResult<ManagedInstallState>.Fail("Invalid game executable path");
        }

        try
        {
            var existingResult = Load(gameExePath);
            var existing = existingResult.Success ? existingResult.Data : null;
            var (currentHash, currentFileSize) = FileHasher.ComputeHashAndSize(gameExePath);

            var originalHash = !string.IsNullOrWhiteSpace(existing?.OriginalHash)
                ? existing.OriginalHash
                : !string.IsNullOrWhiteSpace(originalVersion.Hash)
                    ? originalVersion.Hash
                    : currentHash;

            var originalFileSize = existing?.OriginalFileSize > 0
                ? existing.OriginalFileSize
                : originalVersion.FileSize > 0
                    ? originalVersion.FileSize
                    : currentFileSize;

            var originalVersionForState = existing?.OriginalVersion != null &&
                existing.OriginalVersion.Version != "Unknown"
                    ? existing.OriginalVersion
                    : originalVersion;

            var state = new ManagedInstallState
            {
                SchemaVersion = existing?.SchemaVersion ?? 1,
                GameExePath = Path.GetFullPath(gameExePath),
                GameExeFileName = Path.GetFileName(gameExePath),
                OriginalHash = NormalizeHash(originalHash),
                OriginalFileSize = originalFileSize,
                OriginalVersion = originalVersionForState,
                CurrentHash = NormalizeHash(currentHash),
                CurrentFileSize = currentFileSize,
                InstalledPatches = installedPatches
                    .Where(id => !string.IsNullOrWhiteSpace(id))
                    .Distinct(StringComparer.OrdinalIgnoreCase)
                    .ToList(),
                KProxyInstalled = kproxyInstalled,
                CreatedAt = existing?.CreatedAt ?? DateTime.Now,
                UpdatedAt = DateTime.Now
            };

            File.WriteAllText(statePath, JsonSerializer.Serialize(state, JsonOptions));

            return PatchResult<ManagedInstallState>.Ok(
                state,
                $"Saved managed install state: {StateFileName}");
        }
        catch (Exception ex)
        {
            return PatchResult<ManagedInstallState>.Fail($"Failed to save managed install state: {ex.Message}");
        }
    }

    /// <summary>
    /// Loads managed identity from the directory containing the selected executable.
    /// </summary>
    public static PatchResult<ManagedInstallState> Load(string gameExePath)
    {
        var statePath = GetStatePath(gameExePath);
        if (string.IsNullOrWhiteSpace(statePath))
        {
            return PatchResult<ManagedInstallState>.Fail("Invalid game executable path");
        }

        if (!File.Exists(statePath))
        {
            return PatchResult<ManagedInstallState>.Fail($"Managed install state not found: {statePath}");
        }

        try
        {
            var state = JsonSerializer.Deserialize<ManagedInstallState>(File.ReadAllText(statePath));
            if (state == null)
            {
                return PatchResult<ManagedInstallState>.Fail("Managed install state could not be parsed");
            }

            // Deliberately do not validate the executable file name here. KPM stores
            // state per game directory, and users may rename the executable after
            // KPM has claimed the install. The stored name remains diagnostic only.

            if (state.OriginalVersion == null || string.IsNullOrWhiteSpace(state.OriginalHash))
            {
                return PatchResult<ManagedInstallState>.Fail("Managed install state is missing original game identity");
            }

            return PatchResult<ManagedInstallState>.Ok(state, $"Loaded managed install state: {StateFileName}");
        }
        catch (Exception ex)
        {
            return PatchResult<ManagedInstallState>.Fail($"Failed to load managed install state: {ex.Message}");
        }
    }

    /// <summary>
    /// Deletes the managed identity file for the selected executable's directory.
    /// </summary>
    public static PatchResult Delete(string gameExePath)
    {
        var statePath = GetStatePath(gameExePath);
        if (string.IsNullOrWhiteSpace(statePath))
        {
            return PatchResult.Fail("Invalid game executable path");
        }

        try
        {
            if (File.Exists(statePath))
            {
                File.Delete(statePath);
                return PatchResult.Ok($"Deleted {StateFileName}");
            }

            return PatchResult.Ok($"{StateFileName} not present");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to delete managed install state: {ex.Message}");
        }
    }

    private static string NormalizeHash(string hash) => hash.Trim().ToUpperInvariant();
}
