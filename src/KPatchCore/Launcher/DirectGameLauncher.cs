using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Launch strategy for a host that can run the game executable itself while something other than
/// injection loads the patcher. That is Windows with <see cref="DeploymentMethod.LibraryProxy"/>:
/// the staged KProxy loads the patcher when the game starts, so the manager only has to start it,
/// the same way injection always has.
/// </summary>
internal sealed class DirectGameLauncher : IGameLauncher
{
    public LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return LaunchDispatcher.StartDirectly(
            gameExePath, commandLineArgs, "The game loads the patches at startup.");
    }
}
