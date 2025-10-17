namespace KPatchCore.Models;

/// <summary>
/// Supported platforms for KOTOR
/// </summary>
public enum Platform
{
    Windows,
    macOS,
    Linux
}

/// <summary>
/// Game distribution channel
/// </summary>
public enum Distribution
{
    GOG,
    Steam,
    Physical,
    Other
}

/// <summary>
/// CPU architecture
/// </summary>
public enum Architecture
{
    x86,      // 32-bit
    x86_64,   // 64-bit x86
    ARM64     // Apple Silicon, etc.
}

/// <summary>
/// Represents a specific version of KOTOR
/// </summary>
public sealed class GameVersion
{
    /// <summary>
    /// Operating system platform
    /// </summary>
    public required Platform Platform { get; init; }

    /// <summary>
    /// Distribution/storefront
    /// </summary>
    public required Distribution Distribution { get; init; }

    /// <summary>
    /// Game version number (e.g., "1.03")
    /// </summary>
    public required string Version { get; init; }

    /// <summary>
    /// CPU architecture
    /// </summary>
    public required Architecture Architecture { get; init; }

    /// <summary>
    /// Expected file size in bytes (for verification)
    /// </summary>
    public required long FileSize { get; init; }

    /// <summary>
    /// SHA256 hash of the executable (for definitive identification)
    /// </summary>
    public required string Hash { get; init; }

    /// <summary>
    /// Human-readable display name
    /// </summary>
    public string DisplayName =>
        $"KOTOR {Version} ({Distribution}, {Platform}, {Architecture})";

    /// <summary>
    /// Unique identifier for this version
    /// </summary>
    public string Id =>
        $"kotor_{Version}_{Platform}_{Distribution}_{Architecture}".ToLowerInvariant();

    public override string ToString() => DisplayName;
}
