using KPatchCore.Models;

namespace KPatchCore.Validators;

/// <summary>
/// Validates game version compatibility with patches
/// </summary>
public static class GameVersionValidator
{
    /// <summary>
    /// Validates that a patch supports the detected game version
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <param name="detectedVersion">Detected game version</param>
    /// <returns>Result indicating if the game version is supported</returns>
    public static PatchResult ValidateGameVersion(
        PatchManifest manifest,
        GameVersion detectedVersion)
    {
        // If no supported versions specified, fail
        if (manifest.SupportedVersions.Count == 0)
        {
            return PatchResult.Fail(
                $"Patch '{manifest.Id}' does not specify any supported game versions"
            );
        }

        // Check if the detected hash is in supported versions
        if (manifest.SupportedVersions.ContainsValue(detectedVersion.Hash))
        {
            // Find the version key for this hash
            var versionKey = manifest.SupportedVersions
                .FirstOrDefault(kvp => kvp.Value.Equals(detectedVersion.Hash, StringComparison.OrdinalIgnoreCase))
                .Key;

            return PatchResult.Ok(
                $"Game version is supported (matched: {versionKey ?? "unknown"})"
            );
        }

        // Game version not supported - provide helpful error message
        var supportedList = string.Join(", ", manifest.SupportedVersions.Keys);
        var hashPreview = detectedVersion.Hash.Length > 16
            ? detectedVersion.Hash.Substring(0, 16) + "..."
            : detectedVersion.Hash;
        var detectedInfo = detectedVersion.Version != "Unknown"
            ? $"{detectedVersion.DisplayName} (hash: {hashPreview})"
            : $"Unknown version (hash: {hashPreview})";

        return PatchResult.Fail(
            $"Patch '{manifest.Id}' does not support detected game version.\n" +
            $"  Detected: {detectedInfo}\n" +
            $"  Supported: {supportedList}\n" +
            $"  This patch may not work correctly with your game version."
        );
    }

    /// <summary>
    /// Validates that multiple patches all support the same game version
    /// </summary>
    /// <param name="manifests">Collection of patch manifests</param>
    /// <param name="detectedVersion">Detected game version</param>
    /// <returns>Result indicating if all patches support the game version</returns>
    public static PatchResult ValidateAllPatchesSupported(
        IEnumerable<PatchManifest> manifests,
        GameVersion detectedVersion)
    {
        var unsupportedPatches = new List<string>();

        foreach (var manifest in manifests)
        {
            var result = ValidateGameVersion(manifest, detectedVersion);
            if (!result.Success)
            {
                unsupportedPatches.Add(manifest.Id);
            }
        }

        if (unsupportedPatches.Count > 0)
        {
            var hashPreview = detectedVersion.Hash.Length > 16
                ? detectedVersion.Hash.Substring(0, 16) + "..."
                : detectedVersion.Hash;
            return PatchResult.Fail(
                $"The following patches do not support your game version:\n" +
                $"  - {string.Join("\n  - ", unsupportedPatches)}\n" +
                $"Game version: {detectedVersion.DisplayName} (hash: {hashPreview})"
            );
        }

        return PatchResult.Ok($"All patches support game version {detectedVersion.DisplayName}");
    }

    /// <summary>
    /// Checks if a patch supports a specific game version by version key
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <param name="versionKey">Version key (e.g., "kotor1_gog_103")</param>
    /// <returns>True if supported, false otherwise</returns>
    public static bool SupportsVersion(PatchManifest manifest, string versionKey)
    {
        return manifest.SupportedVersions.ContainsKey(versionKey);
    }

    /// <summary>
    /// Gets a list of supported platforms from a patch manifest
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <returns>List of platform names extracted from version keys</returns>
    public static List<string> GetSupportedPlatforms(PatchManifest manifest)
    {
        // Extract platform info from version keys (e.g., "kotor1_gog_103" -> "gog")
        return manifest.SupportedVersions.Keys
            .Select(ExtractPlatformFromKey)
            .Where(p => !string.IsNullOrEmpty(p))
            .Distinct()
            .ToList();
    }

    /// <summary>
    /// Gets a list of supported game versions from a patch manifest
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <returns>List of version numbers extracted from version keys</returns>
    public static List<string> GetSupportedGameVersions(PatchManifest manifest)
    {
        // Extract version info from version keys (e.g., "kotor1_gog_103" -> "1.03")
        return manifest.SupportedVersions.Keys
            .Select(ExtractVersionFromKey)
            .Where(v => !string.IsNullOrEmpty(v))
            .Distinct()
            .ToList();
    }

    /// <summary>
    /// Validates that the expected hash matches the actual hash
    /// </summary>
    /// <param name="expectedHash">Expected hash from manifest</param>
    /// <param name="actualHash">Actual hash from executable</param>
    /// <returns>Result indicating if hashes match</returns>
    public static PatchResult ValidateHash(string expectedHash, string actualHash)
    {
        if (expectedHash.Equals(actualHash, StringComparison.OrdinalIgnoreCase))
        {
            return PatchResult.Ok("Hash matches");
        }

        return PatchResult.Fail(
            $"Hash mismatch:\n" +
            $"  Expected: {expectedHash}\n" +
            $"  Actual:   {actualHash}\n" +
            $"This indicates a different game version or modified executable."
        );
    }

    private static string? ExtractPlatformFromKey(string versionKey)
    {
        // Expected format: "kotor1_gog_103" or "kotor2_steam_10b"
        var parts = versionKey.Split('_');
        if (parts.Length >= 2)
        {
            return parts[1]; // "gog", "steam", etc.
        }
        return null;
    }

    private static string? ExtractVersionFromKey(string versionKey)
    {
        // Expected format: "kotor1_gog_103" -> "1.03" or "kotor2_steam_10b" -> "1.0b"
        var parts = versionKey.Split('_');
        if (parts.Length >= 3)
        {
            var version = parts[2];
            // Try to format it nicely (e.g., "103" -> "1.03", "10b" -> "1.0b")
            if (version.Length >= 2 && version.All(char.IsDigit))
            {
                return $"{version[0]}.{version.Substring(1)}";
            }
            return version;
        }
        return null;
    }
}
