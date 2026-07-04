using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Gets a patched game running with KotorPatcher.dll loaded, which is the DLL that
/// applies the runtime patches. Windows injects it into the game process; a
/// platform that can't inject from the native launcher reports back instead.
/// </summary>
internal interface IGameInjector
{
    /// <summary>
    /// Launches the game with the patcher DLL loaded, or returns why it could not.
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="dllPath">Path to KotorPatcher.dll</param>
    /// <param name="commandLineArgs">Optional command line arguments for the game</param>
    /// <param name="distribution">Game distribution (GOG, Steam, etc.)</param>
    /// <returns>Launch result with process information or an error</returns>
    LaunchResult LaunchWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution);
}
