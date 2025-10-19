using System.Diagnostics;
using Avalonia;

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
            // CLI mode - launch game with patches
            return RunCli(args);
        }
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

    /// <summary>
    /// Run CLI mode (game launcher)
    /// </summary>
    private static int RunCli(string[] args)
    {
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
                Console.WriteLine("  KPatchLauncher.exe [game_executable.exe]");
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

            Console.WriteLine();
            Console.WriteLine($"Launching with patches...");
            Console.WriteLine($"Injecting: {PatcherDllName}");
            Console.WriteLine();

            // Launch with DLL injection
            var result = ProcessInjector.LaunchWithInjection(gameExePath, patcherDllPath);

            if (!result.Success)
            {
                Console.WriteLine($"ERROR: {result.Error}");
                Console.WriteLine();
                Console.WriteLine("Failed to inject patches. Try launching the game directly");
                Console.WriteLine("to run without patches, or reinstall patches.");
                return 1;
            }

            var process = result.Data!;
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
            if (File.Exists(specifiedPath) && Path.GetExtension(specifiedPath).ToLower() == ".exe")
            {
                return Path.GetFullPath(specifiedPath);
            }
        }

        // Look for common KOTOR executable names in launcher directory
        var commonNames = new[] { "swkotor.exe", "swkotor2.exe", "KOTOR.exe", "KOTOR2.exe" };

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
