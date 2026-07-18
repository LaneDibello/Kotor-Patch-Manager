using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Anything that isn't Windows or Linux. No launch strategy here, so say so
/// plainly rather than let a Win32 call blow up. Applying patches still works.
/// </summary>
internal sealed class UnsupportedGameInjector : IGameLauncher
{
    public LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return LaunchResult.Fail(
            "Launching with injection is only supported on Windows. You can apply " +
            "patches here and launch the game yourself (e.g. through Steam, Proton, " +
            "or Wine), but without injection only executable (static) patches take " +
            "effect.");
    }
}
