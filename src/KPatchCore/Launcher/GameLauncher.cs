using System.Diagnostics;
using System.Runtime.InteropServices;
using KPatchCore.Detectors;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Provides game launching functionality with automatic patch detection and patcher loading
/// </summary>
public static class GameLauncher
{
    /// <summary>
    /// Launches a game executable with automatic patch detection
    /// Detects if patches are installed and loads KotorPatcher.dll if needed
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

        // Patches are installed. Detect the game to pick the deployment method (which decides the
        // patcher module the game loads) and the distribution (which decides the launch strategy).
        var versionResult = GameDetector.DetectVersion(gameExePath, allowManagedInstallState: true);
        var gameVersion = versionResult.Data;
        var distribution = gameVersion?.Distribution ?? Distribution.Other;
        var deployment = gameVersion != null
            ? DeploymentPolicy.ForGame(gameVersion)
            : DeploymentPolicy.ForCurrentPlatform();

        var moduleName = DeploymentPolicy.PatcherModuleFileName(deployment);
        var patcherModulePath = Path.Combine(gameDir, moduleName);
        if (!File.Exists(patcherModulePath))
        {
            return LaunchResult.Fail(
                $"Patches are installed (patch_config.toml found) but {moduleName} is missing. " +
                $"Expected location: {patcherModulePath}");
        }

        return Launch(gameExePath, patcherModulePath, distribution, commandLineArgs, launchConfig);
    }

    /// <summary>
    /// Launches a patched game with an explicit patcher DLL
    /// </summary>
    /// <param name="gameExePath">Path to game executable</param>
    /// <param name="dllPath">Path to the patcher DLL to load</param>
    /// <param name="distribution">Game distribution (for the launch strategy)</param>
    /// <param name="commandLineArgs">Optional command line arguments</param>
    /// <returns>Launch result with process information</returns>
    public static LaunchResult Launch(
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

        // Delegate to the platform-specific launcher
        return CreateLauncher(launchConfig).Launch(gameExePath, dllPath, commandLineArgs, distribution);
    }

    /// <summary>
    /// Selects the launch strategy for the configured deployment method.
    /// </summary>
    private static IGameLauncher CreateLauncher(LaunchConfig? launchConfig)
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
