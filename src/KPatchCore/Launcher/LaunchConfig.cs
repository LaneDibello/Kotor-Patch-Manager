namespace KPatchCore.Launcher;

/// <summary>
/// How the manager starts the game when deploying via proxy: the staged KProxy
/// already loads the patcher, so the manager only needs to launch the game.
/// </summary>
public enum LaunchMethod
{
    /// <summary>Launch through Steam (Proton) by app id.</summary>
    Steam,

    /// <summary>Run a user-supplied command (Lutris, Heroic, plain Wine, etc.).</summary>
    Custom,
}

/// <summary>
/// User-chosen launch configuration: how the manager should start the game.
/// </summary>
public sealed class LaunchConfig
{
    /// <summary>The launch method to use.</summary>
    public LaunchMethod Method { get; init; } = LaunchMethod.Steam;

    /// <summary>
    /// Command to run for <see cref="LaunchMethod.Custom"/>. The token "{exe}" is
    /// replaced with the game executable path.
    /// </summary>
    public string? CustomCommand { get; init; }
}
