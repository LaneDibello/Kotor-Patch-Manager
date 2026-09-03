using System.Diagnostics;
using Avalonia;
using KPatchCore.Applicators;
using KPatchCore.Detectors;
using KPatchCore.Launcher;
using KPatchCore.Managers;
using KPatchCore.Models;
using KPatchLauncher.Models;

namespace KPatchLauncher;

/// <summary>
/// KPatch Launcher - Dual-mode: CLI for game launching, GUI for patch management
/// </summary>
class Program
{
    private const string PatcherDllName = "KotorPatcher.dll";
    private const string PatchConfigName = "patch_config.toml";

    [STAThread]
    static int Main(string[] args)
    {
        // Determine mode: GUI if no arguments, CLI if arguments provided
        if (args.Length == 0)
        {
            // GUI mode - launch Avalonia application
            return RunGui();
        }
        else
        {
            // The deployment preference is machine configuration, and the CLI installs into and
            // launches the same game the window does. Without reading it here a CLI install would
            // quietly put a proxy deployment back on injection, and a CLI launch would inject into
            // a game whose proxy already loads the patcher.
            DeploymentPolicy.PreferLibraryProxy = AppSettings.Load().PreferLibraryProxy;

            if (!TryTakeDeploymentOption(ref args, out var deploymentError))
            {
                Console.WriteLine($"ERROR: {deploymentError}");
                return 1;
            }

            // CLI mode - launch game with patches
            return RunCli(args);
        }
    }

    /// <summary>
    /// Consumes "--deployment proxy|injection" from <paramref name="args"/> and applies it for
    /// this run, leaving what remains positional for the callers that index into it.
    /// </summary>
    /// <remarks>
    /// The choice is not written back to the settings file. A scripted install should not decide
    /// what the window does the next time somebody opens it.
    /// </remarks>
    /// <returns>False when the option is present but unusable, with the reason in
    /// <paramref name="error"/>.</returns>
    private static bool TryTakeDeploymentOption(ref string[] args, out string? error)
    {
        error = null;

        var index = Array.FindIndex(args, a => a.Equals("--deployment", StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            return true;
        }

        if (index + 1 >= args.Length)
        {
            error = "--deployment needs a value: proxy or injection.";
            return false;
        }

        var value = args[index + 1];
        DeploymentMethod requested;
        if (value.Equals("proxy", StringComparison.OrdinalIgnoreCase))
        {
            DeploymentPolicy.PreferLibraryProxy = true;
            requested = DeploymentMethod.LibraryProxy;
        }
        else if (value.Equals("injection", StringComparison.OrdinalIgnoreCase))
        {
            DeploymentPolicy.PreferLibraryProxy = false;
            requested = DeploymentMethod.RuntimeInjection;
        }
        else
        {
            error = $"Unknown deployment \"{value}\". Use proxy or injection.";
            return false;
        }

        // Asking for something the host cannot do is not fatal, but it would otherwise be silent:
        // a Linux host has no way to inject and stays on the proxy whatever was asked for.
        var effective = DeploymentPolicy.ForCurrentPlatform();
        if (effective != requested)
        {
            Console.WriteLine($"Note: this host cannot use {value}; deploying via {effective} instead.");
        }

        args = args.Where((_, i) => i != index && i != index + 1).ToArray();
        return true;
    }

    /// <summary>
    /// Run GUI mode (patch management interface)
    /// </summary>
    private static int RunGui()
    {
        try
        {
            var builder = BuildAvaloniaApp();
            return builder.StartWithClassicDesktopLifetime(Array.Empty<string>());
        }
        catch (Exception ex)
        {
            Console.WriteLine($"GUI ERROR: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
            return 1;
        }
    }

    /// <summary>
    /// Avalonia configuration
    /// </summary>
    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .WithInterFont()
            .LogToTrace();

    private static int RunApplyCli(string[] args)
    {
        if (args.Length < 3)
        {
            Console.WriteLine("Usage: KPatchLauncher.exe <game_executable.exe> --patches <patches_directory> [patch_id]...");
            Console.WriteLine("       [--deployment proxy|injection]  how the patcher gets into the game");
            return 1;
        }

        var gameExePath = args[0];
        var orchestrator = new PatchOrchestrator(args[2]);
        var availablePatches = orchestrator.GetAvailablePatches();
        var patchIds = args.Skip(3).ToList();
        if (patchIds.Count == 0)
        {
            patchIds.AddRange(availablePatches.Keys);
        }

        var missingPatchId = patchIds.FirstOrDefault(id => !availablePatches.ContainsKey(id));
        if (patchIds.Count == 0 || missingPatchId != null)
        {
            Console.WriteLine(missingPatchId == null
                ? "ERROR: No .kpatch files were found in the patches directory."
                : $"ERROR: Patch ID not found: {missingPatchId}");
            return 1;
        }

        var removalResult = PatchRemover.RemoveAllPatches(gameExePath, removeManagedState: false);
        if (!removalResult.Success)
        {
            Console.WriteLine($"ERROR: {removalResult.Error}");
            return 1;
        }

        var result = orchestrator.InstallPatches(
            gameExePath,
            patchIds,
            patcherDirectory: AppContext.BaseDirectory);
        if (!result.Success)
        {
            Console.WriteLine($"ERROR: {result.Error}");
            return 1;
        }

        Console.WriteLine($"Applied {result.InstalledPatches.Count} patch(es) successfully.");
        return RunCli(new[] { gameExePath });
    }

    /// <summary>
    /// Run CLI mode (game launcher)
    /// </summary>
    private static int RunCli(string[] args)
    {
        if (args.Length > 1 && args[1].Equals("--patches", StringComparison.OrdinalIgnoreCase))
        {
            return RunApplyCli(args);
        }

        Console.WriteLine("KPatch Launcher v1.0");
        Console.WriteLine("====================");
        Console.WriteLine();

        try
        {
            // Get the directory where the launcher is located
            var launcherDir = AppContext.BaseDirectory;

            // Find the game executable
            var gameExePath = FindGameExecutable(launcherDir, args);
            if (gameExePath == null)
            {
                Console.WriteLine("ERROR: Could not find game executable.");
                Console.WriteLine();
                Console.WriteLine("Usage:");
                Console.WriteLine("  KPatchLauncher.exe [game_executable.exe] [--deployment proxy|injection]");
                Console.WriteLine();
                Console.WriteLine("Place this launcher in the same directory as the game executable,");
                Console.WriteLine("or specify the game executable path as an argument.");
                return 1;
            }

            Console.WriteLine($"Game: {Path.GetFileName(gameExePath)}");
            Console.WriteLine($"Directory: {Path.GetDirectoryName(gameExePath)}");
            Console.WriteLine();

            // Check if patches are installed
            var gameDir = Path.GetDirectoryName(gameExePath)!;
            var patchConfigPath = Path.Combine(gameDir, PatchConfigName);
            var patcherDllPath = Path.Combine(gameDir, PatcherDllName);

            if (!File.Exists(patchConfigPath))
            {
                Console.WriteLine("No patches detected (patch_config.toml not found).");
                Console.WriteLine("Launching vanilla game...");
                Console.WriteLine();

                return LaunchVanilla(gameExePath);
            }

            if (!File.Exists(patcherDllPath))
            {
                Console.WriteLine($"ERROR: {PatcherDllName} not found in game directory.");
                Console.WriteLine("Patches are configured but patcher DLL is missing.");
                Console.WriteLine("Please reinstall patches or run vanilla game directly.");
                return 1;
            }

            // Check patches directory
            var patchesDir = Path.Combine(gameDir, "patches");
            if (!Directory.Exists(patchesDir))
            {
                Console.WriteLine("WARNING: patches/ directory not found.");
                Console.WriteLine("Patches may not work correctly.");
            }
            else
            {
                var patchDlls = Directory.GetFiles(patchesDir, "*.dll");
                Console.WriteLine($"Found {patchDlls.Length} patch DLL(s) in patches/ directory");
            }

            // Detect game version to determine distribution (Steam, GOG, etc.)
            Console.WriteLine("Detecting game version...");
            var versionResult = GameDetector.DetectVersion(gameExePath, allowManagedInstallState: true);
            var distribution = Distribution.Other;  // Default fallback

            if (versionResult.Success && versionResult.Data != null)
            {
                distribution = versionResult.Data.Distribution;
                Console.WriteLine($"Detected: {versionResult.Data.DisplayName}");
            }
            else
            {
                Console.WriteLine($"WARNING: Could not detect game version: {versionResult.Error}");
                Console.WriteLine("Defaulting to direct injection method.");
            }

            Console.WriteLine();
            Console.WriteLine($"Launching with patches...");
            Console.WriteLine($"Injecting: {PatcherDllName}");
            Console.WriteLine();

            // Launch with DLL injection (method depends on distribution)
            var result = GameLauncher.Launch(
                gameExePath,
                patcherDllPath,
                distribution: distribution,
                commandLineArgs: null);

            if (!result.Success)
            {
                Console.WriteLine($"ERROR: {result.Error}");
                Console.WriteLine();
                Console.WriteLine("Failed to inject patches. Try launching the game directly");
                Console.WriteLine("to run without patches, or reinstall patches.");
                return 1;
            }

            var process = result.GameProcess!;
            Console.WriteLine($"✓ Game launched successfully (PID: {process.Id})");
            Console.WriteLine($"✓ {PatcherDllName} injected");
            Console.WriteLine();
            Console.WriteLine("Game is running with patches applied.");
            Console.WriteLine("You can close this window - the game will continue running.");
            Console.WriteLine();

            // Optionally monitor for crashes (disabled by default)
            if (args.Contains("--monitor"))
            {
                Console.WriteLine("Monitoring game process...");
                process.WaitForExit();
                Console.WriteLine($"Game exited with code: {process.ExitCode}");
                return process.ExitCode;
            }

            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"FATAL ERROR: {ex.Message}");
            Console.WriteLine();
            Console.WriteLine(ex.StackTrace);
            return 1;
        }
    }

    /// <summary>
    /// Finds the game executable in the launcher directory or from arguments
    /// </summary>
    private static string? FindGameExecutable(string launcherDir, string[] args)
    {
        // Check if game executable was specified as argument
        if (args.Length > 0)
        {
            var specifiedPath = args[0];
            var ext = Path.GetExtension(specifiedPath).ToLowerInvariant();
            if (File.Exists(specifiedPath) && (ext == ".exe" || (!OperatingSystem.IsWindows() && ext.Length == 0)))
            {
                return Path.GetFullPath(specifiedPath);
            }
        }

        // Look for common KOTOR executable names in launcher directory
        var commonNames = new[] { "swkotor.exe", "swkotor2.exe", "KOTOR.exe", "KOTOR2.exe", "KOTOR2" };

        foreach (var name in commonNames)
        {
            var path = Path.Combine(launcherDir, name);
            if (File.Exists(path))
            {
                return path;
            }
        }

        // Look for any .exe file (excluding the launcher itself)
        var exeFiles = Directory.GetFiles(launcherDir, "*.exe")
            .Where(f => !Path.GetFileName(f).Equals("KPatchLauncher.exe", StringComparison.OrdinalIgnoreCase))
            .ToArray();

        if (exeFiles.Length == 1)
        {
            return exeFiles[0];
        }

        return null;
    }

    /// <summary>
    /// Launches the game without patches (vanilla)
    /// </summary>
    private static int LaunchVanilla(string gameExePath)
    {
        try
        {
            var process = Process.Start(new ProcessStartInfo
            {
                FileName = gameExePath,
                WorkingDirectory = Path.GetDirectoryName(gameExePath),
                UseShellExecute = true
            });

            if (process == null)
            {
                Console.WriteLine("ERROR: Failed to start game process");
                return 1;
            }

            Console.WriteLine($"✓ Game launched (PID: {process.Id})");
            Console.WriteLine("You can close this window - the game will continue running.");
            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR: Failed to launch game: {ex.Message}");
            return 1;
        }
    }
}
