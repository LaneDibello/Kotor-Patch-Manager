using System.Diagnostics;
using System.Runtime.InteropServices;
using KPatchCore.Detectors;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Provides game launching functionality with automatic patch detection and DLL injection
/// </summary>
public static class GameLauncher
{
    /// <summary>
    /// Launches a game executable with automatic patch detection
    /// Detects if patches are installed and injects KotorPatcher.dll if needed
    /// Falls back to vanilla launch if no patches detected
    /// </summary>
    /// <param name="gameExePath">Path to game executable</param>
    /// <param name="commandLineArgs">Optional command line arguments</param>
    /// <returns>Launch result with process information</returns>
    public static LaunchResult LaunchGame(string gameExePath, string? commandLineArgs = null, LaunchConfig? launchConfig = null)
    {
        // Validate game path
        if (string.IsNullOrWhiteSpace(gameExePath) || !File.Exists(gameExePath))
        {
            return LaunchResult.Fail($"Game executable not found: {gameExePath}");
        }

        var gameDir = Path.GetDirectoryName(gameExePath);
        if (string.IsNullOrWhiteSpace(gameDir))
        {
            return LaunchResult.Fail($"Could not determine game directory from path: {gameExePath}");
        }

        var patchConfigPath = Path.Combine(gameDir, "patch_config.toml");

        // Check if patches are installed
        if (!File.Exists(patchConfigPath))
        {
            return LaunchVanilla(gameExePath, commandLineArgs);
        }

        // Patches installed - launch with injection
        var patcherDllPath = Path.Combine(gameDir, "KotorPatcher.dll");

        if (!File.Exists(patcherDllPath))
        {
            return LaunchResult.Fail(
                $"Patches are installed (patch_config.toml found) but KotorPatcher.dll is missing. " +
                $"Expected location: {patcherDllPath}");
        }

        // Detect game version to determine distribution
        var versionResult = GameDetector.DetectVersion(gameExePath, allowManagedInstallState: true);
        var distribution = versionResult.Data?.Distribution ?? Distribution.Other;

        return LaunchWithInjection(gameExePath, patcherDllPath, distribution, commandLineArgs, launchConfig);
    }

    /// <summary>
    /// Launches a game with explicit DLL injection (for patched games)
    /// </summary>
    /// <param name="gameExePath">Path to game executable</param>
    /// <param name="dllPath">Path to DLL to inject</param>
    /// <param name="distribution">Game distribution (for injection strategy)</param>
    /// <param name="commandLineArgs">Optional command line arguments</param>
    /// <returns>Launch result with process information</returns>
    public static LaunchResult LaunchWithInjection(
        string gameExePath,
        string dllPath,
        Distribution distribution,
        string? commandLineArgs = null,
        LaunchConfig? launchConfig = null)
    {
        // Validate inputs
        if (string.IsNullOrWhiteSpace(gameExePath) || !File.Exists(gameExePath))
        {
            return LaunchResult.Fail($"Game executable not found: {gameExePath}");
        }

        if (string.IsNullOrWhiteSpace(dllPath) || !File.Exists(dllPath))
        {
            return LaunchResult.Fail($"DLL not found: {dllPath}");
        }

        // Delegate to the platform-specific injector
        return CreateInjector(launchConfig).LaunchWithInjection(gameExePath, dllPath, commandLineArgs, distribution);
    }

    /// <summary>
    /// Selects the launch strategy for the configured deployment method.
    /// </summary>
    private static IGameInjector CreateInjector(LaunchConfig? launchConfig)
    {
        // Proxy method: the game loads the patcher itself via the staged proxy,
        // so we just start it (Steam or a custom command).
        if (DeploymentPolicy.ForCurrentPlatform() == DeploymentMethod.Proxy)
        {
            return new ProxyGameLauncher(launchConfig ?? new LaunchConfig());
        }

        // Injection only works on Windows (it uses the Win32 API).
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return new WindowsGameInjector();
        }

        return new UnsupportedGameInjector();
    }

    /// <summary>
    /// Launches a game without any modification (vanilla launch)
    /// </summary>
    /// <param name="gameExePath">Path to game executable</param>
    /// <param name="commandLineArgs">Optional command line arguments</param>
    /// <returns>Launch result with process information</returns>
    public static LaunchResult LaunchVanilla(string gameExePath, string? commandLineArgs = null)
    {
        // Validate game path
        if (string.IsNullOrWhiteSpace(gameExePath) || !File.Exists(gameExePath))
        {
            return LaunchResult.Fail($"Game executable not found: {gameExePath}");
        }

        try
        {
            var gameDir = Path.GetDirectoryName(gameExePath);

            var startInfo = new ProcessStartInfo
            {
                FileName = gameExePath,
                Arguments = commandLineArgs ?? string.Empty,
                UseShellExecute = true,
                WorkingDirectory = gameDir
            };

            var process = Process.Start(startInfo);

            if (process == null)
            {
                return LaunchResult.Fail("Process.Start returned null - game may have failed to launch");
            }

            return LaunchResult.Ok(
                process,
                injectionPerformed: false,
                $"Launched {Path.GetFileName(gameExePath)} in vanilla mode (no patches)");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Vanilla launch failed: {ex.Message}");
        }
    }
}
