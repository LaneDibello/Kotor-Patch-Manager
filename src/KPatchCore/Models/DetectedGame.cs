namespace KPatchCore.Models;

/// <summary>
/// How a game was recognised, in descending order of how much it proves.
/// </summary>
public enum GameIdentity
{
    /// <summary>The executable's hash is one the manager knows.</summary>
    Hash,

    /// <summary>
    /// The manager's own install state says which build this was before it patched it.
    /// </summary>
    ManagedState,

    /// <summary>
    /// The hash matched nothing, but the build identity did. The file has been changed since it
    /// was linked, by another tool or by whoever packaged it.
    /// </summary>
    Inferred,

    /// <summary>Nothing recognised it. No hooks and no address database exist for it.</summary>
    Unknown
}

/// <summary>
/// A game the manager has looked at, and what the answer rests on.
/// </summary>
public sealed record DetectedGame(GameVersion Version, GameIdentity Identity);
