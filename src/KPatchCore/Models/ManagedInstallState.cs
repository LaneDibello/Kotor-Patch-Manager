namespace KPatchCore.Models;

/// <summary>
/// Persisted identity for a KPM-managed game executable.
/// </summary>
public sealed class ManagedInstallState
{
    /// <summary>
    /// Schema version for future migrations.
    /// </summary>
    public int SchemaVersion { get; init; } = 1;

    /// <summary>
    /// Last known absolute path to the game executable. Diagnostic only; users may rename the EXE.
    /// </summary>
    public required string GameExePath { get; init; }

    /// <summary>
    /// Last known executable filename. Diagnostic only; renaming the EXE does not invalidate state.
    /// </summary>
    public required string GameExeFileName { get; init; }

    /// <summary>
    /// SHA-256 hash of the original clean executable. This is the stable game identity.
    /// </summary>
    public required string OriginalHash { get; init; }

    /// <summary>
    /// File size of the original clean executable. Diagnostic only; future KPM patches may change the executable size.
    /// </summary>
    public required long OriginalFileSize { get; init; }

    /// <summary>
    /// Game version detected from the original clean executable.
    /// </summary>
    public required GameVersion OriginalVersion { get; init; }

    /// <summary>
    /// SHA-256 hash of the executable after the latest successful KPM install/update. Diagnostic only.
    /// </summary>
    public string? CurrentHash { get; init; }

    /// <summary>
    /// File size of the executable after the latest successful KPM install/update. Diagnostic only.
    /// </summary>
    public long? CurrentFileSize { get; init; }

    /// <summary>
    /// Patch IDs installed during the latest successful KPM install/update.
    /// </summary>
    public List<string> InstalledPatches { get; init; } = new();

    /// <summary>
    /// Whether the KProxy was staged (the game's binkw32.dll renamed to
    /// binkw32Hooked.dll and the proxy put in its place).
    /// </summary>
    public bool KProxyInstalled { get; init; }

    /// <summary>
    /// When KPM first claimed this install.
    /// </summary>
    public required DateTime CreatedAt { get; init; }

    /// <summary>
    /// When KPM last updated this state.
    /// </summary>
    public required DateTime UpdatedAt { get; init; }
}
