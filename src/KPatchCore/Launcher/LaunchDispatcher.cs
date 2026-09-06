using System.Diagnostics;
using System.Runtime.InteropServices;

namespace KPatchCore.Launcher;

/// <summary>
/// Starts the game the way the user configured it: through Steam, or a custom command
/// (Lutris, Heroic, plain Wine). This is the single place that turns a
/// <see cref="LaunchConfig"/> into a running game, shared by the patched and unpatched
/// paths so a launch behaves the same either way.
/// </summary>
internal static class LaunchDispatcher
{
    /// <summary>
    /// Starts the game per <paramref name="config"/>. Returns why it could not rather
    /// than throwing.
    /// </summary>
    public static LaunchResult Start(LaunchConfig config, string gameExePath, string context)
    {
        return config.Method == LaunchMethod.Custom
            ? Custom(config, gameExePath, context)
            : Steam(gameExePath, context);
    }

    private static LaunchResult Steam(string gameExePath, string context)
    {
        if (!SteamLauncher.TryResolveAppId(gameExePath, out var appId))
        {
            return LaunchResult.Fail(
                $"No known Steam app id for {Path.GetFileName(gameExePath)}. " +
                "Use a custom launch command instead.");
        }

        try
        {
            SteamLauncher.Launch(appId);
            return LaunchResult.Launched($"Launched through Steam (app {appId}). {context}");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Failed to launch through Steam: {ex.Message}");
        }
    }

    private static LaunchResult Custom(LaunchConfig config, string gameExePath, string context)
    {
        if (string.IsNullOrWhiteSpace(config.CustomCommand))
        {
            return LaunchResult.Fail("No custom launch command is set.");
        }

        var command = config.CustomCommand.Replace("{exe}", gameExePath);

        // KOTOR resolves chitin.key and the override directory relative to the working
        // directory, so it has to start in the game folder. A wrapper like "wine {exe}"
        // would otherwise inherit the manager's own directory and the game exits early.
        var gameDir = Path.GetDirectoryName(gameExePath);

        // A user-typed command line (wrappers, extra args, quoting), so hand it to the
        // platform's command processor. .NET has no native command-line runner, and
        // UseShellExecute only launches a file by association, not a parsed command.
        ProcessStartInfo startInfo;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            // ComSpec is the OS-provided processor, so we don't hardcode cmd.exe.
            var comSpec = Environment.GetEnvironmentVariable("ComSpec") ?? "cmd.exe";
            startInfo = new ProcessStartInfo { FileName = comSpec, ArgumentList = { "/c", command } };
        }
        else
        {
            // /bin/sh is the POSIX system shell, guaranteed present on Linux and macOS
            // alike. Deliberately not $SHELL, which is the user's interactive login shell,
            // not a command runner.
            startInfo = new ProcessStartInfo { FileName = "/bin/sh", ArgumentList = { "-c", command } };
        }

        if (!string.IsNullOrWhiteSpace(gameDir))
        {
            startInfo.WorkingDirectory = gameDir;
        }

        try
        {
            Process.Start(startInfo);
            return LaunchResult.Launched($"Launched with the custom command. {context}");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Custom launch failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Starts the game executable itself, for hosts that can run it. KOTOR resolves chitin.key
    /// and the override directory relative to the working directory, so it starts in the game
    /// folder. Returns why it could not rather than throwing.
    /// </summary>
    /// <param name="patched">
    /// Whether the game has patches installed. Nothing is injected either way here, so this cannot
    /// be inferred from the launch itself: with the library proxy the game loads the patcher on its
    /// own and the launch looks exactly like an unpatched one.
    /// </param>
    public static LaunchResult StartDirectly(
        string gameExePath,
        string? commandLineArgs,
        string context,
        bool patched)
    {
        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = gameExePath,
                Arguments = commandLineArgs ?? string.Empty,
                UseShellExecute = true,
                WorkingDirectory = Path.GetDirectoryName(gameExePath),
            };

            var process = Process.Start(startInfo);
            if (process == null)
            {
                return LaunchResult.Fail("Process.Start returned null - game may have failed to launch");
            }

            var message = $"Launched {Path.GetFileName(gameExePath)}. {context}";
            return patched
                ? LaunchResult.Patched(process, injectionPerformed: false, message)
                : LaunchResult.Vanilla(process, message);
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Launch failed: {ex.Message}");
        }
    }
}
