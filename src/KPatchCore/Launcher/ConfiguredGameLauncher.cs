using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Launch strategy for every deployment method the manager cannot start directly. The
/// game loads the patcher itself, through the staged KProxy under Wine or Proton and
/// through DT_NEEDED on the native Linux build, so all this has to do is start the game
/// the way the user configured: through Steam, or a custom command (Lutris, Heroic,
/// plain Wine).
/// </summary>
internal sealed class ConfiguredGameLauncher : IGameLauncher
{
    private readonly LaunchConfig _config;

    public ConfiguredGameLauncher(LaunchConfig config)
    {
        _config = config;
    }

    public LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return LaunchDispatcher.Start(_config, gameExePath, "The game loads the patches at startup.");
    }
}
