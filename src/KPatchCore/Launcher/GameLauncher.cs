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

        // Detect the game before deciding anything. The deployment method decides both the
        // patcher module the game loads and how the game is started, and only the first of
        // those depends on patches being installed.
        var versionResult = GameDetector.DetectVersion(gameExePath, allowManagedInstallState: true);
        var gameVersion = versionResult.Data;
        var distribution = gameVersion?.Distribution ?? Distribution.Other;
        var deployment = gameVersion != null
            ? DeploymentPolicy.ForGame(gameVersion)
            : DeploymentPolicy.ForCurrentPlatform();

        if (!File.Exists(patchConfigPath))
        {
            return LaunchVanilla(gameExePath, commandLineArgs, launchConfig);
        }

        var moduleName = DeploymentPolicy.PatcherModuleFileName(deployment);
        var patcherModulePath = Path.Combine(gameDir, moduleName);
        if (!File.Exists(patcherModulePath))
        {
            return LaunchResult.Fail(
                $"Patches are installed (patch_config.toml found) but {moduleName} is missing. " +
                $"Expected location: {patcherModulePath}");
        }

        return Launch(gameExePath, patcherModulePath, distribution, commandLineArgs, launchConfig, deployment);
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
        LaunchConfig? launchConfig = null,
        DeploymentMethod? deployment = null)
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
        return CreateLauncher(launchConfig, deployment).Launch(gameExePath, dllPath, commandLineArgs, distribution);
    }

    /// <summary>
    /// Selects the launch strategy for the configured deployment method.
    /// </summary>
    private static IGameLauncher CreateLauncher(LaunchConfig? launchConfig, DeploymentMethod? deployment)
    {
        // Callers that never detected the game fall back to the host's default.
        var method = deployment ?? DeploymentPolicy.ForCurrentPlatform();

        // Injection has to create the process itself, and it uses the Win32 API to do it.
        if (method == DeploymentMethod.RuntimeInjection)
        {
            return RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? new WindowsGameInjector()
                : new UnsupportedGameInjector();
        }

        // Every other method has the game load the patcher on its own, so starting it is all that
        // is left. How that happens depends on the host, not on the method.
        return DeploymentPolicy.CanStartGameDirectly()
            ? new DirectGameLauncher()
            : new ConfiguredGameLauncher(launchConfig ?? new LaunchConfig());
    }

    /// <summary>
    /// Launches a game without any modification (vanilla launch)
    /// </summary>
    /// <param name="gameExePath">Path to game executable</param>
    /// <param name="commandLineArgs">Optional command line arguments</param>
    /// <param name="launchConfig">How to start the game, when the host cannot run it directly</param>
    /// <returns>Launch result with process information</returns>
    public static LaunchResult LaunchVanilla(
        string gameExePath,
        string? commandLineArgs = null,
        LaunchConfig? launchConfig = null)
    {
        // Validate game path
        if (string.IsNullOrWhiteSpace(gameExePath) || !File.Exists(gameExePath))
        {
            return LaunchResult.Fail($"Game executable not found: {gameExePath}");
        }

        // Whether the game is patched changes what gets loaded into it, not how it is
        // started, so an unpatched launch asks the same question a patched one does.
        return DeploymentPolicy.CanStartGameDirectly()
            ? LaunchDispatcher.StartDirectly(gameExePath, commandLineArgs, "No patches are installed.")
            : LaunchDispatcher.Start(launchConfig ?? new LaunchConfig(), gameExePath, "No patches are installed.");
    }
}
