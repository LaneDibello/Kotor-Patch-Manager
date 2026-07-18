using System.Diagnostics;
using System.Runtime.InteropServices;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Launch strategy for the proxy deployment method. The staged KProxy loads the
/// patcher when the game starts, so this just starts the game the way the user
/// configured: through Steam, or a custom command (Lutris, Heroic, plain Wine).
/// Used wherever the deployment method is proxy (Linux today, or Windows if wired
/// onto it via <see cref="DeploymentPolicy"/>).
/// </summary>
internal sealed class ProxyGameLauncher : IGameLauncher
{
    private readonly LaunchConfig _config;

    public ProxyGameLauncher(LaunchConfig config)
    {
        _config = config;
    }

    public LaunchResult Launch(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        return _config.Method == LaunchMethod.Custom
            ? LaunchCustom(gameExePath)
            : LaunchSteam(gameExePath);
    }

    private LaunchResult LaunchSteam(string gameExePath)
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
            return LaunchResult.Launched(
                $"Launched through Steam (app {appId}). Patches load via the KProxy.");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Failed to launch through Steam: {ex.Message}");
        }
    }

    private LaunchResult LaunchCustom(string gameExePath)
    {
        if (string.IsNullOrWhiteSpace(_config.CustomCommand))
        {
            return LaunchResult.Fail("No custom launch command is set.");
        }

        var command = _config.CustomCommand.Replace("{exe}", gameExePath);

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
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            // /bin/sh is the POSIX system shell, guaranteed present. Deliberately not
            // $SHELL, which is the user's interactive login shell, not a command runner.
            startInfo = new ProcessStartInfo { FileName = "/bin/sh", ArgumentList = { "-c", command } };
        }
        else
        {
            return LaunchResult.Fail("Custom launch is only supported on Windows and Linux.");
        }

        try
        {
            Process.Start(startInfo);
            return LaunchResult.Launched("Launched with the custom command. Patches load via the KProxy.");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Custom launch failed: {ex.Message}");
        }
    }
}
