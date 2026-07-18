using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Gets a patched game running with KotorPatcher.dll loaded, the DLL that applies
/// the runtime patches. Implementations differ by how that DLL gets loaded: Windows
/// injects it into the process, the proxy method lets the staged KProxy load it at
/// start, and unsupported platforms report back instead.
/// </summary>
internal interface IGameLauncher
{
    /// <summary>
    /// Launches the game with the patcher DLL loaded, or returns why it could not.
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="dllPath">Path to KotorPatcher.dll</param>
    /// <param name="commandLineArgs">Optional command line arguments for the game</param>
    /// <param name="distribution">Game distribution (GOG, Steam, etc.)</param>
    /// <returns>Launch result with process information or an error</returns>
    LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution);
}
