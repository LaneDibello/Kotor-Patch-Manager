using KPatchCore.Models;

namespace KPatchCore.Validators;

/// <summary>
/// Validates patch manifest structure and content
/// </summary>
public static class PatchValidator
{
    /// <summary>
    /// Validates a patch manifest
    /// </summary>
    /// <param name="manifest">Patch manifest to validate</param>
    /// <returns>Result indicating if manifest is valid</returns>
    public static PatchResult ValidateManifest(PatchManifest manifest)
    {
        var errors = new List<string>();

        // Check required fields
        if (string.IsNullOrWhiteSpace(manifest.Id))
        {
            errors.Add("Patch ID is required");
        }
        else if (!IsValidPatchId(manifest.Id))
        {
            errors.Add($"Invalid patch ID '{manifest.Id}' - use lowercase letters, numbers, and hyphens only");
        }

        if (string.IsNullOrWhiteSpace(manifest.Name))
        {
            errors.Add("Patch name is required");
        }

        if (string.IsNullOrWhiteSpace(manifest.Version))
        {
            errors.Add("Patch version is required");
        }
        else if (!IsValidVersion(manifest.Version))
        {
            errors.Add($"Invalid version format '{manifest.Version}' - use semantic versioning (e.g., 1.2.0)");
        }

        if (string.IsNullOrWhiteSpace(manifest.Author))
        {
            errors.Add("Patch author is required");
        }

        if (string.IsNullOrWhiteSpace(manifest.Description))
        {
            errors.Add("Patch description is required");
        }

        // Check for supported versions
        if (manifest.SupportedVersions.Count == 0)
        {
            errors.Add("At least one supported game version is required");
        }

        // Check for self-reference in dependencies
        if (manifest.Requires.Contains(manifest.Id))
        {
            errors.Add("Patch cannot require itself");
        }

        if (manifest.Conflicts.Contains(manifest.Id))
        {
            errors.Add("Patch cannot conflict with itself");
        }

        // Check for overlap between requires and conflicts
        var overlap = manifest.Requires.Intersect(manifest.Conflicts).ToList();
        if (overlap.Count > 0)
        {
            errors.Add($"Patch cannot both require and conflict with: {string.Join(", ", overlap)}");
        }

        if (errors.Count > 0)
        {
            return PatchResult.Fail($"Manifest validation failed: {string.Join("; ", errors)}");
        }

        return PatchResult.Ok("Manifest is valid");
    }

    /// <summary>
    /// Validates patch ID format
    /// </summary>
    /// <param name="patchId">Patch ID to validate</param>
    /// <returns>True if valid, false otherwise</returns>
    public static bool IsValidPatchId(string patchId)
    {
        if (string.IsNullOrWhiteSpace(patchId))
            return false;

        // Patch ID should be lowercase alphanumeric with hyphens
        // Examples: "widescreen-fix", "bugfix-pack", "hd-textures"
        return patchId.All(c => char.IsLower(c) || char.IsDigit(c) || c == '-') &&
               !patchId.StartsWith('-') &&
               !patchId.EndsWith('-');
    }

    /// <summary>
    /// Validates version format (simple semantic versioning check)
    /// </summary>
    /// <param name="version">Version string to validate</param>
    /// <returns>True if valid, false otherwise</returns>
    public static bool IsValidVersion(string version)
    {
        if (string.IsNullOrWhiteSpace(version))
            return false;

        // Accept semantic versioning formats: X.Y.Z, X.Y, X.Y.Z-beta, etc.
        var parts = version.Split('-')[0].Split('.');

        if (parts.Length < 2 || parts.Length > 3)
            return false;

        // Check that each part is a number
        return parts.All(part => int.TryParse(part, out _));
    }

    /// <summary>
    /// Validates that all required patches are available
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <param name="availablePatches">List of available patch IDs</param>
    /// <returns>Result indicating if all dependencies are satisfied</returns>
    public static PatchResult ValidateDependenciesAvailable(
        PatchManifest manifest,
        IEnumerable<string> availablePatches)
    {
        var available = availablePatches.ToHashSet();
        var missing = manifest.Requires.Where(req => !available.Contains(req)).ToList();

        if (missing.Count > 0)
        {
            return PatchResult.Fail(
                $"Missing required patches: {string.Join(", ", missing)}"
            );
        }

        return PatchResult.Ok("All dependencies available");
    }

    /// <summary>
    /// Validates that no conflicting patches are present
    /// </summary>
    /// <param name="manifest">Patch manifest</param>
    /// <param name="installedPatches">List of installed patch IDs</param>
    /// <returns>Result indicating if there are any conflicts</returns>
    public static PatchResult ValidateNoConflicts(
        PatchManifest manifest,
        IEnumerable<string> installedPatches)
    {
        var installed = installedPatches.ToHashSet();
        var conflicts = manifest.Conflicts.Where(conf => installed.Contains(conf)).ToList();

        if (conflicts.Count > 0)
        {
            return PatchResult.Fail(
                $"Conflicting patches are installed: {string.Join(", ", conflicts)}"
            );
        }

        return PatchResult.Ok("No conflicts detected");
    }
}
