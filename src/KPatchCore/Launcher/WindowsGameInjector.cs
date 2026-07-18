using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Windows: inject the DLL into the game process via the Win32 API. The real
/// work lives in <see cref="ProcessInjector"/> (suspended-create for GOG, delayed
/// injection for Steam).
/// </summary>
internal sealed class WindowsGameInjector : IGameLauncher
{
    public LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return ProcessInjector.LaunchWithInjection(
            gameExePath, dllPath, commandLineArgs, distribution);
    }
}
