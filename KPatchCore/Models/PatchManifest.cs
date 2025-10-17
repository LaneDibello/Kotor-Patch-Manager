namespace KPatchCore.Models;

/// <summary>
/// Represents the metadata from a patch's manifest.toml file
/// </summary>
public sealed class PatchManifest
{
    /// <summary>
    /// Unique identifier for the patch (e.g., "widescreen-fix")
    /// </summary>
    public required string Id { get; init; }

    /// <summary>
    /// Human-readable patch name (e.g., "Widescreen Resolution Fix")
    /// </summary>
    public required string Name { get; init; }

    /// <summary>
    /// Semantic version (e.g., "1.2.0")
    /// </summary>
    public required string Version { get; init; }

    /// <summary>
    /// Patch author/creator
    /// </summary>
    public required string Author { get; init; }

    /// <summary>
    /// Description of what the patch does
    /// </summary>
    public required string Description { get; init; }

    /// <summary>
    /// List of patch IDs that must be installed before this one
    /// </summary>
    public List<string> Requires { get; init; } = new();

    /// <summary>
    /// List of patch IDs that conflict with this patch
    /// </summary>
    public List<string> Conflicts { get; init; } = new();

    /// <summary>
    /// Game versions this patch supports (key = GameVersion.Id, value = expected hash)
    /// </summary>
    public Dictionary<string, string> SupportedVersions { get; init; } = new();

    /// <summary>
    /// Optional URL to patch homepage/documentation
    /// </summary>
    public string? Url { get; init; }

    /// <summary>
    /// Optional license information
    /// </summary>
    public string? License { get; init; }

    public override string ToString() =>
        $"{Name} v{Version} by {Author}";
}
