using KPatchCore.Models;

namespace KPatchCore.Validators;

/// <summary>
/// Validates patch dependencies and conflicts
/// </summary>
public static class DependencyValidator
{
    /// <summary>
    /// Validates that all dependencies are satisfied for a set of patches
    /// </summary>
    /// <param name="patches">Dictionary of patch ID to manifest</param>
    /// <param name="patchesToInstall">List of patch IDs to install</param>
    /// <returns>Result indicating if all dependencies are satisfied</returns>
    public static PatchResult ValidateDependencies(
        Dictionary<string, PatchManifest> patches,
        IEnumerable<string> patchesToInstall)
    {
        var toInstall = patchesToInstall.ToHashSet();
        var errors = new List<string>();

        foreach (var patchId in toInstall)
        {
            if (!patches.TryGetValue(patchId, out var manifest))
            {
                errors.Add($"Patch '{patchId}' not found");
                continue;
            }

            // Check if all required patches are either being installed or already available
            var missing = manifest.Requires.Where(req => !toInstall.Contains(req)).ToList();
            if (missing.Count > 0)
            {
                errors.Add(
                    $"Patch '{patchId}' requires: {string.Join(", ", missing)}"
                );
            }
        }

        if (errors.Count > 0)
        {
            return PatchResult.Fail(
                $"Dependency validation failed:\n  - {string.Join("\n  - ", errors)}"
            );
        }

        return PatchResult.Ok("All dependencies satisfied");
    }

    /// <summary>
    /// Detects circular dependencies in patch requirements
    /// </summary>
    /// <param name="patches">Dictionary of patch ID to manifest</param>
    /// <returns>Result indicating if there are circular dependencies</returns>
    public static PatchResult DetectCircularDependencies(Dictionary<string, PatchManifest> patches)
    {
        var cycles = new List<string>();

        foreach (var patchId in patches.Keys)
        {
            var visited = new HashSet<string>();
            var path = new List<string>();

            if (HasCircularDependency(patchId, patches, visited, path))
            {
                cycles.Add($"Circular dependency: {string.Join(" -> ", path)} -> {patchId}");
            }
        }

        if (cycles.Count > 0)
        {
            return PatchResult.Fail(
                $"Circular dependencies detected:\n  - {string.Join("\n  - ", cycles)}"
            );
        }

        return PatchResult.Ok("No circular dependencies detected");
    }

    /// <summary>
    /// Validates that no conflicts exist between patches being installed
    /// </summary>
    /// <param name="patches">Dictionary of patch ID to manifest</param>
    /// <param name="patchesToInstall">List of patch IDs to install</param>
    /// <returns>Result indicating if there are any conflicts</returns>
    public static PatchResult ValidateNoConflicts(
        Dictionary<string, PatchManifest> patches,
        IEnumerable<string> patchesToInstall)
    {
        var toInstall = patchesToInstall.ToHashSet();
        var errors = new List<string>();

        foreach (var patchId in toInstall)
        {
            if (!patches.TryGetValue(patchId, out var manifest))
            {
                continue;
            }

            // Check if any conflicting patches are in the install list
            var conflicts = manifest.Conflicts.Where(conf => toInstall.Contains(conf)).ToList();
            if (conflicts.Count > 0)
            {
                errors.Add(
                    $"Patch '{patchId}' conflicts with: {string.Join(", ", conflicts)}"
                );
            }
        }

        if (errors.Count > 0)
        {
            return PatchResult.Fail(
                $"Conflict validation failed:\n  - {string.Join("\n  - ", errors)}"
            );
        }

        return PatchResult.Ok("No conflicts detected");
    }

    /// <summary>
    /// Calculates the installation order based on dependencies
    /// </summary>
    /// <param name="patches">Dictionary of patch ID to manifest</param>
    /// <param name="patchesToInstall">List of patch IDs to install</param>
    /// <returns>Result containing ordered list of patch IDs or error</returns>
    public static PatchResult<List<string>> CalculateInstallOrder(
        Dictionary<string, PatchManifest> patches,
        IEnumerable<string> patchesToInstall)
    {
        var toInstall = patchesToInstall.ToHashSet();
        var ordered = new List<string>();
        var visited = new HashSet<string>();

        // Topological sort using DFS
        foreach (var patchId in toInstall)
        {
            if (!VisitPatch(patchId, patches, toInstall, visited, ordered))
            {
                return PatchResult<List<string>>.Fail(
                    $"Failed to calculate install order - possible circular dependency involving '{patchId}'"
                );
            }
        }

        return PatchResult<List<string>>.Ok(
            ordered,
            $"Install order: {string.Join(" -> ", ordered)}"
        );
    }

    private static bool HasCircularDependency(
        string patchId,
        Dictionary<string, PatchManifest> patches,
        HashSet<string> visited,
        List<string> path)
    {
        if (path.Contains(patchId))
        {
            return true; // Found a cycle
        }

        if (visited.Contains(patchId))
        {
            return false; // Already checked this path
        }

        visited.Add(patchId);
        path.Add(patchId);

        if (patches.TryGetValue(patchId, out var manifest))
        {
            foreach (var dependency in manifest.Requires)
            {
                if (HasCircularDependency(dependency, patches, visited, path))
                {
                    return true;
                }
            }
        }

        path.Remove(patchId);
        return false;
    }

    private static bool VisitPatch(
        string patchId,
        Dictionary<string, PatchManifest> patches,
        HashSet<string> toInstall,
        HashSet<string> visited,
        List<string> ordered)
    {
        if (visited.Contains(patchId))
        {
            return true; // Already processed
        }

        if (!patches.TryGetValue(patchId, out var manifest))
        {
            return false; // Patch not found
        }

        visited.Add(patchId);

        // Visit dependencies first
        foreach (var dependency in manifest.Requires)
        {
            if (toInstall.Contains(dependency))
            {
                if (!VisitPatch(dependency, patches, toInstall, visited, ordered))
                {
                    return false;
                }
            }
        }

        // Add this patch after its dependencies
        ordered.Add(patchId);
        return true;
    }

    /// <summary>
    /// Validates version compatibility between dependent patches
    /// </summary>
    /// <param name="manifest">Patch manifest to check</param>
    /// <param name="installedVersions">Dictionary of installed patch IDs to their versions</param>
    /// <returns>Result indicating if versions are compatible</returns>
    public static PatchResult ValidateVersionCompatibility(
        PatchManifest manifest,
        Dictionary<string, string> installedVersions)
    {
        // For now, this is a simple check that required patches exist
        // In the future, could add version range requirements (e.g., "requires: base-patch >= 1.0")

        var missing = manifest.Requires
            .Where(req => !installedVersions.ContainsKey(req))
            .ToList();

        if (missing.Count > 0)
        {
            return PatchResult.Fail(
                $"Missing required patches: {string.Join(", ", missing)}"
            );
        }

        return PatchResult.Ok("Version compatibility validated");
    }
}
