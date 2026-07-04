using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Linux: the game runs as a Windows executable under Wine/Proton, and the native
/// launcher can't inject the DLL into it. So this reports back and leaves
/// launching to the user.
/// </summary>
internal sealed class LinuxGameInjector : IGameInjector
{
    public LaunchResult LaunchWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return LaunchResult.Fail(
            "Launching with injection is not supported on Linux. You can apply " +
            "patches here and launch the game through Steam, Proton, or Wine, but " +
            "without injection only executable (static) patches take effect.");
    }
}
