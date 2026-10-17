using System.Diagnostics;
using Avalonia;
using KPatchCore.Applicators;
using KPatchCore.Common;
using KPatchCore.Detectors;
using KPatchCore.Launcher;
using KPatchCore.Managers;
using KPatchCore.Models;
using KPatchCore.Validators;
using KPatchLauncher.Models;

namespace KPatchLauncher;

/// <summary>
/// KPatch Launcher - Dual-mode: CLI for game launching, GUI for patch management
/// </summary>
class Program
{
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
            // Machine configuration, and the CLI installs into and launches the same game the
            // window does. Without reading it here a CLI install would quietly put a proxy
            // deployment back on injection, a CLI launch would inject into a game whose proxy
            // already loads the patcher, and a game the window had been told to accept would be
            // refused.
            var settings = AppSettings.Load();
            DeploymentPolicy.PreferLibraryProxy = settings.PreferLibraryProxy;
            GameDetector.IdentifyUnrecognisedBuilds = settings.IdentifyUnrecognisedBuilds;

            if (!TryTakeDeploymentOption(ref args, out var deploymentError))
            {
                Console.WriteLine($"ERROR: {deploymentError}");
                return 1;
            }

            TakeHashBypassOption(ref args);

            if (!TryTakeOptionValues(ref args, out var optionError))
            {
                Console.WriteLine($"ERROR: {optionError}");
                return 1;
            }

            if (CliOptionValues.Count > 0 && !args.Contains("--patches", StringComparer.OrdinalIgnoreCase))
            {
                Console.WriteLine("ERROR: --option sets what --patches installs; it does nothing on a launch alone.");
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

        // Asked for explicitly, so it outranks the method a game was installed with too.
        DeploymentPolicy.RequestedDeployment = requested;

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
    /// Patch option values given on the command line, by patch id then option id.
    /// </summary>
    private static readonly Dictionary<string, Dictionary<string, string>> CliOptionValues = new();

    /// <summary>
    /// Consumes every "--option &lt;patch-id&gt;.&lt;option-id&gt;=&lt;value&gt;" from
    /// <paramref name="args"/>, leaving the positional arguments where the rest of the CLI
    /// expects them.
    /// </summary>
    /// <remarks>
    /// Only the shape is checked here. Whether the patch has that option, and whether the
    /// option can hold that value, is the installer's to say: it has the manifest.
    /// </remarks>
    /// <returns>False when an --option is present but unusable, with the reason in
    /// <paramref name="error"/>.</returns>
    private static bool TryTakeOptionValues(ref string[] args, out string? error)
    {
        error = null;
        var kept = new List<string>();

        for (int i = 0; i < args.Length; i++)
        {
            if (!args[i].Equals("--option", StringComparison.OrdinalIgnoreCase))
            {
                kept.Add(args[i]);
                continue;
            }

            if (i + 1 >= args.Length)
            {
                error = "--option needs a value: <patch-id>.<option-id>=<value>.";
                return false;
            }

            var text = args[++i];
            var dot = text.IndexOf('.');
            var equals = text.IndexOf('=');
            if (dot <= 0 || equals <= dot + 1 || equals == text.Length - 1)
            {
                error = $"Cannot read --option \"{text}\". Use <patch-id>.<option-id>=<value>.";
                return false;
            }

            var patchId = text[..dot];
            if (!CliOptionValues.TryGetValue(patchId, out var values))
            {
                CliOptionValues[patchId] = values = new Dictionary<string, string>();
            }
            values[text[(dot + 1)..equals]] = text[(equals + 1)..];
        }

        args = kept.ToArray();
        return true;
    }

    /// <summary>
    /// Consumes "--bypass-hash" from <paramref name="args"/> and applies it for this run, leaving
    /// what remains positional for the callers that index into it.
    /// </summary>
    /// <remarks>
    /// Not written back to the settings file, for the reason --deployment is not. It only switches
    /// the bypass on: a run that wants the hash enforced simply leaves the flag off, and says so by
    /// not passing it.
    /// </remarks>
    private static void TakeHashBypassOption(ref string[] args)
    {
        var index = Array.FindIndex(args, a => a.Equals("--bypass-hash", StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            return;
        }

        GameDetector.IdentifyUnrecognisedBuilds = true;
        args = args.Where((_, i) => i != index).ToArray();
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
            Console.WriteLine("       [--bypass-hash]                 patch a game whose hash is not recognised");
            Console.WriteLine("       [--option <patch_id>.<option_id>=<value>]...  set a patch option (default otherwise)");
            return 1;
        }

        // Resolved the same way the window resolves it, so a macOS bundle or a game
        // folder works from a script too.
        var gameExePath = PathHelpers.ResolveGameExecutable(args[0]);
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

        // Checked before anything is removed, like the patch ids above: a mistyped option
        // must not cost the install that is already there.
        var strayOptions = CliOptionValues.Keys.FirstOrDefault(id => !patchIds.Contains(id));
        if (strayOptions != null)
        {
            Console.WriteLine($"ERROR: Options were given for {strayOptions}, which is not being installed");
            return 1;
        }

        foreach (var patchId in patchIds)
        {
            var values = OptionValidator.ResolveValues(
                availablePatches[patchId].Manifest,
                CliOptionValues.GetValueOrDefault(patchId));
            if (!values.Success)
            {
                Console.WriteLine($"ERROR: {values.Error}");
                return 1;
            }
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
            patcherDirectory: AppContext.BaseDirectory,
            optionValues: CliOptionValues);
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
                Console.WriteLine("  KPatchLauncher.exe [game_executable.exe] [--deployment proxy|injection] [--bypass-hash]");
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
            if (!File.Exists(patchConfigPath))
            {
                Console.WriteLine("No patches detected (patch_config.toml not found).");
                Console.WriteLine("Launching vanilla game...");
                Console.WriteLine();

                return LaunchVanilla(gameExePath);
            }

            // Which module the game loads follows the game, not the host: the Windows builds
            // take the DLL, the native Linux one a shared object, the macOS ones a dylib.
            var detectedForModule = GameDetector.DetectVersion(gameExePath, allowManagedInstallState: true).Data;
            var patcherModuleName = DeploymentPolicy.PatcherModuleFileName(detectedForModule);
            var patcherModulePath = Path.Combine(gameDir, patcherModuleName);

            if (!File.Exists(patcherModulePath))
            {
                Console.WriteLine($"ERROR: {patcherModuleName} not found in game directory.");
                Console.WriteLine("Patches are configured but the patcher module is missing.");
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
            Console.WriteLine($"Patcher module: {patcherModuleName}");
            Console.WriteLine();

            // Launch the way the game was installed, as the window does: a proxy install is only
            // started, an injection install is injected into (DeploymentPolicy.ForLaunch).
            var result = GameLauncher.Launch(
                gameExePath,
                patcherModulePath,
                distribution: distribution,
                commandLineArgs: null,
                deployment: versionResult.Success && versionResult.Data != null
                    ? DeploymentPolicy.ForLaunch(versionResult.Data, gameExePath)
                    : null);

            if (!result.Success)
            {
                Console.WriteLine($"ERROR: {result.Error}");
                Console.WriteLine();
                Console.WriteLine("Failed to inject patches. Try launching the game directly");
                Console.WriteLine("to run without patches, or reinstall patches.");
                return 1;
            }

            // Only a launch the manager performed itself has a process to report. Handing off to
            // Steam or to a custom command starts the game through something else, so there is
            // nothing here to watch, and reading one crashed the moment Steam became reachable.
            var process = result.GameProcess;
            var pidInfo = process is null ? string.Empty : $" (PID: {process.Id})";
            Console.WriteLine($"✓ Game launched successfully{pidInfo}");
            Console.WriteLine($"✓ {patcherModuleName} in place");
            Console.WriteLine();
            Console.WriteLine("Game is running with patches applied.");
            Console.WriteLine("You can close this window - the game will continue running.");
            Console.WriteLine();

            // Optionally monitor for crashes (disabled by default)
            if (args.Contains("--monitor") && process is not null)
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
