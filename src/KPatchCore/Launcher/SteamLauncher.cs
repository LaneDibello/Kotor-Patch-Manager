using System.Diagnostics;

namespace KPatchCore.Launcher;

/// <summary>
/// Resolves the Steam app id for a KOTOR game and launches it through Steam, so a
/// patched game starts under Proton with the full Steam runtime environment.
/// </summary>
internal static class SteamLauncher
{
    // KOTOR's Steam app ids are stable constants, keyed by executable name.
    private static readonly Dictionary<string, string> AppIds = new(StringComparer.OrdinalIgnoreCase)
    {
        ["swkotor.exe"] = "32370",    // Knights of the Old Republic
        ["swkotor2.exe"] = "208580",  // Knights of the Old Republic II
        ["KOTOR2"] = "208580",        // KOTOR II native Linux ELF (same Steam app, Linux depot)

        // The macOS builds, which live inside an .app bundle. Same Steam apps again, macOS
        // depots. These matter more than the others: a macOS host cannot start the game
        // itself, so Steam is the only way in.
        ["KOTOR_Exe"] = "32370",      // KOTOR I, Knights of the Old Republic.app
        ["KOTOR2sub"] = "208580",     // KOTOR II, KOTOR2.app
    };

    /// <summary>
    /// Resolves the Steam app id for a KOTOR executable by name.
    /// </summary>
    /// <param name="gameExePath">Path to the game executable.</param>
    /// <param name="appId">The resolved app id, if the executable is recognised.</param>
    /// <returns>True if an app id was found.</returns>
    internal static bool TryResolveAppId(string gameExePath, out string appId)
    {
        if (AppIds.TryGetValue(Path.GetFileName(gameExePath), out var id))
        {
            appId = id;
            return true;
        }

        appId = string.Empty;
        return false;
    }

    /// <summary>
    /// Asks Steam to launch a game by app id via the steam:// URL handler.
    /// </summary>
    /// <param name="appId">Steam app id to launch.</param>
    internal static void Launch(string appId)
    {
        Process.Start(new ProcessStartInfo
        {
            FileName = $"steam://rungameid/{appId}",
            UseShellExecute = true
        });
    }
}
