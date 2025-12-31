using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// Internal implementation of DLL injection for game processes using Windows API
/// </summary>
internal static class ProcessInjector
{
    /// <summary>
    /// Launches a game executable with DLL injection
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="dllPath">Path to the DLL to inject</param>
    /// <param name="commandLineArgs">Optional command line arguments for the game</param>
    /// <param name="distribution">Game distribution (GOG, Steam, etc.) to determine injection method</param>
    /// <returns>Launch result containing Process object or error</returns>
    internal static LaunchResult LaunchWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs,
        Distribution distribution)
    {
        if (!File.Exists(gameExePath))
        {
            return LaunchResult.Fail($"Game executable not found: {gameExePath}");
        }

        if (!File.Exists(dllPath))
        {
            return LaunchResult.Fail($"DLL not found: {dllPath}");
        }

        if (distribution == Distribution.Steam)
        {
            Console.WriteLine("[KPatchCore] Detected Steam distribution, using delayed injection method");
            return LaunchSteamWithInjection(gameExePath, dllPath, commandLineArgs);
        }
        else
        {
            return LaunchDirectWithInjection(gameExePath, dllPath, commandLineArgs);
        }
    }

    /// <summary>
    /// Launches a game executable with direct DLL injection (GOG/Physical/Other distributions)
    /// Uses CREATE_SUSPENDED to inject before the game starts
    /// </summary>
    private static LaunchResult LaunchDirectWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs)
    {
        try
        {
            var absGamePath = Path.GetFullPath(gameExePath);
            var absDllPath = Path.GetFullPath(dllPath);

            var si = new Win32.STARTUPINFO
            {
                cb = Marshal.SizeOf(typeof(Win32.STARTUPINFO))
            };

            var pi = new Win32.PROCESS_INFORMATION();

            var commandLine = $"\"{absGamePath}\"";
            if (!string.IsNullOrWhiteSpace(commandLineArgs))
            {
                commandLine += $" {commandLineArgs}";
            }

            // Create the process suspended
            var success = Win32.CreateProcess(
                absGamePath,
                commandLine,
                IntPtr.Zero,
                IntPtr.Zero,
                false,
                Win32.CREATE_SUSPENDED,
                IntPtr.Zero,
                Path.GetDirectoryName(absGamePath),
                ref si,
                out pi);

            if (!success)
            {
                var error = Marshal.GetLastWin32Error();
                return LaunchResult.Fail(
                    $"Failed to create process (error {error}): {gameExePath}");
            }

            try
            {
                // Inject the DLL into the suspended process
                var injectResult = InjectDllIntoProcess(pi.hProcess, absDllPath);

                if (!injectResult.Success)
                {
                    Process.GetProcessById(pi.dwProcessId).Kill();
                    return LaunchResult.Fail(
                        $"DLL injection failed: {injectResult.Error}");
                }

                // Debug mode: Set to 'true' if you want to hook a debugger to the process
                if (false)
                {
                    Console.WriteLine("========================================");
                    Console.WriteLine("DEBUG MODE ENABLED");
                    Console.WriteLine($"Game process created (PID: {pi.dwProcessId})");
                    Console.WriteLine("Process is SUSPENDED - DLL has been injected");
                    Console.WriteLine("");
                    Console.WriteLine("You can now:");
                    Console.WriteLine("  1. Attach your debugger (Cheat Engine, x32dbg, etc.)");
                    Console.WriteLine("  2. Set breakpoints in KotorPatcher.dll or game code");
                    Console.WriteLine("  3. Press ENTER to resume the game");
                    Console.WriteLine("========================================");
                    Console.ReadLine();
                    Console.WriteLine("[DEBUG] Resuming game process...");
                }

                // Resume the main thread
                var resumeResult = Win32.ResumeThread(pi.hThread);
                if (resumeResult == unchecked((uint)-1))
                {
                    var error = Marshal.GetLastWin32Error();
                    Process.GetProcessById(pi.dwProcessId).Kill();
                    return LaunchResult.Fail(
                        $"Failed to resume thread (error {error})");
                }

                var process = Process.GetProcessById(pi.dwProcessId);

                return LaunchResult.Ok(
                    process,
                    injectionPerformed: true,
                    $"Successfully launched {Path.GetFileName(gameExePath)} with DLL injection");
            }
            finally
            {
                if (pi.hProcess != IntPtr.Zero) Win32.CloseHandle(pi.hProcess);
                if (pi.hThread != IntPtr.Zero) Win32.CloseHandle(pi.hThread);
            }
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Launch failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Injects a DLL into a running process
    /// </summary>
    /// <param name="processId">Target process ID</param>
    /// <param name="dllPath">Path to the DLL to inject</param>
    /// <returns>Result indicating success or failure</returns>
    private static PatchResult InjectIntoRunningProcess(int processId, string dllPath)
    {
        if (!File.Exists(dllPath))
        {
            return PatchResult.Fail($"DLL not found: {dllPath}");
        }

        try
        {
            // Open the target process
            var hProcess = Win32.OpenProcess(
                Win32.PROCESS_CREATE_THREAD | Win32.PROCESS_QUERY_INFORMATION |
                Win32.PROCESS_VM_OPERATION | Win32.PROCESS_VM_WRITE | Win32.PROCESS_VM_READ,
                false,
                processId);

            if (hProcess == IntPtr.Zero)
            {
                var error = Marshal.GetLastWin32Error();
                return PatchResult.Fail($"Failed to open process {processId} (error {error})");
            }

            try
            {
                return InjectDllIntoProcess(hProcess, Path.GetFullPath(dllPath));
            }
            finally
            {
                Win32.CloseHandle(hProcess);
            }
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Injection failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Internal method to inject DLL into an open process handle
    /// </summary>
    private static PatchResult InjectDllIntoProcess(IntPtr hProcess, string dllPath)
    {
        try
        {
            Console.WriteLine($"[Injector] Injecting: {dllPath}");

            var hKernel32 = Win32.GetModuleHandle("kernel32.dll");
            if (hKernel32 == IntPtr.Zero)
            {
                var msg = "Failed to get kernel32.dll module handle";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] kernel32.dll handle: 0x{hKernel32:X}");

            var pLoadLibraryA = Win32.GetProcAddress(hKernel32, "LoadLibraryA");
            if (pLoadLibraryA == IntPtr.Zero)
            {
                var msg = "Failed to get LoadLibraryA address";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] LoadLibraryA address: 0x{pLoadLibraryA:X}");

            // Allocate memory in the target process for the DLL path
            var dllPathBytes = Encoding.ASCII.GetBytes(dllPath + '\0');
            Console.WriteLine($"[Injector] Allocating {dllPathBytes.Length} bytes in target process...");

            var pDllPath = Win32.VirtualAllocEx(
                hProcess,
                IntPtr.Zero,
                (uint)dllPathBytes.Length,
                Win32.MEM_COMMIT | Win32.MEM_RESERVE,
                Win32.PAGE_READWRITE);

            if (pDllPath == IntPtr.Zero)
            {
                var error = Marshal.GetLastWin32Error();
                var msg = $"Failed to allocate memory in target process (error {error})";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] Allocated memory at: 0x{pDllPath:X}");

            // Write the DLL path to the allocated memory
            Console.WriteLine($"[Injector] Writing DLL path to target process memory...");
            var writeSuccess = Win32.WriteProcessMemory(
                hProcess,
                pDllPath,
                dllPathBytes,
                (uint)dllPathBytes.Length,
                out var bytesWritten);

            if (!writeSuccess || bytesWritten.ToUInt32() != dllPathBytes.Length)
            {
                var error = Marshal.GetLastWin32Error();
                var msg = $"Failed to write DLL path to target process (error {error})";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] Wrote {bytesWritten} bytes successfully");

            // Create a remote thread that calls LoadLibraryA with the DLL path
            Console.WriteLine($"[Injector] Creating remote thread to call LoadLibraryA...");
            var hThread = Win32.CreateRemoteThread(
                hProcess,
                IntPtr.Zero,
                0,
                pLoadLibraryA,
                pDllPath,
                0,
                out var threadId);

            if (hThread == IntPtr.Zero)
            {
                var error = Marshal.GetLastWin32Error();
                var msg = $"Failed to create remote thread (error {error})";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] Remote thread created with ID: {threadId}");

            // Wait for the thread to complete (LoadLibrary to finish)
            Console.WriteLine($"[Injector] Waiting for remote thread to complete...");
            var waitResult = Win32.WaitForSingleObject(hThread, Win32.INFINITE);
            Win32.CloseHandle(hThread);

            if (waitResult != 0) // 0 = WAIT_OBJECT_0
            {
                var msg = $"Remote thread did not complete successfully (wait result: {waitResult})";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }

            Console.WriteLine($"[Injector] SUCCESS: DLL injected successfully!");
            return PatchResult.Ok($"Successfully injected {Path.GetFileName(dllPath)}");
        }
        catch (Exception ex)
        {
            var msg = $"DLL injection failed: {ex.Message}";
            Console.WriteLine($"[Injector] EXCEPTION: {msg}");
            return PatchResult.Fail(msg);
        }
    }

    /// <summary>
    /// Launches a game executable with delayed DLL injection (Steam distribution)
    /// Launches normally, waits for Steam to decrypt, then injects into the running process
    /// </summary>
    private static LaunchResult LaunchSteamWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs)
    {
        try
        {
            var absGamePath = Path.GetFullPath(gameExePath);
            var absDllPath = Path.GetFullPath(dllPath);

            Console.WriteLine($"[KPatchCore] Launching Steam game: {Path.GetFileName(absGamePath)}");

            // Step 1: Launch the game normally (let Steam handle DRM decryption)
            var startInfo = new ProcessStartInfo
            {
                FileName = absGamePath,
                Arguments = commandLineArgs ?? string.Empty,
                UseShellExecute = true,  // Let Steam intercept
                WorkingDirectory = Path.GetDirectoryName(absGamePath)
            };

            Process.Start(startInfo);

            // Step 2: Wait for the game process to appear (with validation to skip bootstrap)
            Console.WriteLine("[KPatchCore] Waiting for game process (detecting and skipping Steam bootstrap)...");
            var gameProcess = FindGameProcess(absGamePath, TimeSpan.FromSeconds(30));

            if (gameProcess == null)
            {
                return LaunchResult.Fail(
                    "Could not find valid game process after Steam launch. " +
                    "Ensure Steam is running and the game launches correctly. " +
                    "Check console output for validation details.");
            }

            Console.WriteLine($"[KPatchCore] Validated game process found (PID: {gameProcess.Id})");

            // Step 3: Wait for game initialization (window creation)
            Console.WriteLine("[KPatchCore] Waiting for game window initialization...");
            if (!WaitForProcessInitialization(gameProcess, TimeSpan.FromSeconds(30)))
            {
                return LaunchResult.Fail(
                    "Timeout waiting for game initialization. " +
                    "The game may have failed to start or Steam decryption took too long.");
            }

            Console.WriteLine("[KPatchCore] Game initialized, injecting DLL...");

            // Step 4: Inject DLL into the running process
            var injectResult = InjectIntoRunningProcess(gameProcess.Id, absDllPath);

            if (!injectResult.Success)
            {
                return LaunchResult.Fail(
                    $"Failed to inject DLL into running Steam process: {injectResult.Error}");
            }

            Console.WriteLine("[KPatchCore] DLL injected successfully into Steam game");

            return LaunchResult.Ok(
                gameProcess,
                injectionPerformed: true,
                $"Successfully launched {Path.GetFileName(gameExePath)} with delayed injection (Steam)");
        }
        catch (Exception ex)
        {
            return LaunchResult.Fail($"Steam launch failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Finds a game process by executable path with validation to avoid Steam bootstrap processes
    /// Polls for processes matching the executable name and validates each one
    /// Tracks checked PIDs to avoid re-validating the same process
    /// </summary>
    private static Process? FindGameProcess(string exePath, TimeSpan timeout)
    {
        var executableName = Path.GetFileNameWithoutExtension(exePath).ToLower();
        var stopwatch = Stopwatch.StartNew();
        var checkedPids = new HashSet<int>();  // Track PIDs we've already validated

        Console.WriteLine($"[KPatchCore] Searching for process: {executableName}");

        while (stopwatch.Elapsed < timeout)
        {
            try
            {
                var processes = Process.GetProcessesByName(executableName);

                foreach (var process in processes)
                {
                    if (checkedPids.Contains(process.Id))
                        continue;

                    checkedPids.Add(process.Id);
                    Console.WriteLine($"[KPatchCore] Found process candidate: PID {process.Id}, validating...");

                    if (IsValidGameProcess(process))
                    {
                        // Extra stability check: wait 500ms to ensure it doesn't exit immediately
                        Thread.Sleep(500);

                        try
                        {
                            process.Refresh();

                            if (!process.HasExited)
                            {
                                Console.WriteLine($"[KPatchCore] Validated and stable process found: PID {process.Id}");
                                return process;
                            }
                            else
                            {
                                Console.WriteLine($"[KPatchCore] Process {process.Id} exited after validation, continuing search...");
                            }
                        }
                        catch
                        {
                            Console.WriteLine($"[KPatchCore] Process {process.Id} became inaccessible, continuing search...");
                        }
                    }
                    // IsValidGameProcess already logs why validation failed
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[KPatchCore] Error during process search: {ex.Message}");
            }

            Thread.Sleep(100);  // Poll every 100ms
        }

        Console.WriteLine($"[KPatchCore] Timeout: No valid process found after {timeout.TotalSeconds}s");
        return null;
    }

    /// <summary>
    /// Waits for a process to complete initialization
    /// Detects initialization by checking for main window handle creation
    /// </summary>
    private static bool WaitForProcessInitialization(Process process, TimeSpan timeout)
    {
        var stopwatch = Stopwatch.StartNew();

        while (stopwatch.Elapsed < timeout)
        {
            try
            {
                process.Refresh();

                // Check if process has exited (failed to initialize)
                if (process.HasExited)
                {
                    Console.WriteLine("[KPatchCore] Process exited before initialization completed");
                    return false;
                }

                // Window handle creation indicates the game has initialized
                if (process.MainWindowHandle != IntPtr.Zero)
                {
                    Console.WriteLine("[KPatchCore] Main window detected, game initialized");
                    return true;
                }
            }
            catch
            {
                // Process may not be accessible - continue waiting
            }

            Thread.Sleep(100);  // Poll every 100ms
        }

        return false;
    }

    /// <summary>
    /// Validates a process to ensure it's the real game executable, not a Steam bootstrap
    /// Uses PE header validation (MZ signature check) to distinguish decrypted game from encrypted stub
    /// </summary>
    private static bool IsValidGameProcess(Process process)
    {
        try
        {
            // Open process with read access to check memory
            var hProcess = Win32.OpenProcess(
                Win32.PROCESS_VM_READ | Win32.PROCESS_QUERY_INFORMATION,
                false,
                process.Id);

            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine($"[KPatchCore] Could not open process {process.Id} for validation");
                return false;
            }

            try
            {
                // Read DOS header from process base address
                byte[] dosHeader = new byte[64];
                bool readSuccess = Win32.ReadProcessMemory(
                    hProcess,
                    new IntPtr(0x400000),  // Base address for Windows executables
                    dosHeader,
                    dosHeader.Length,
                    out _);

                if (readSuccess)
                {
                    // Check for MZ signature (0x4D 0x5A) - indicates valid PE executable
                    if (dosHeader[0] == 0x4D && dosHeader[1] == 0x5A)
                    {
                        Console.WriteLine($"[KPatchCore] Process {process.Id}: Valid PE header detected (MZ signature)");
                        return true;
                    }
                    else
                    {
                        Console.WriteLine($"[KPatchCore] Process {process.Id}: Invalid PE header " +
                            $"(got 0x{dosHeader[0]:X2} 0x{dosHeader[1]:X2}, expected 0x4D 0x5A) - likely bootstrap");
                        return false;
                    }
                }
                else
                {
                    Console.WriteLine($"[KPatchCore] Process {process.Id}: Could not read memory for validation");

                    // Fallback: Check if process stays alive (bootstrap exits quickly)
                    Thread.Sleep(200);
                    process.Refresh();
                    bool isAlive = !process.HasExited;

                    if (isAlive)
                    {
                        Console.WriteLine($"[KPatchCore] Process {process.Id}: Fallback stability check passed");
                    }
                    else
                    {
                        Console.WriteLine($"[KPatchCore] Process {process.Id}: Exited during validation");
                    }

                    return isAlive;
                }
            }
            finally
            {
                Win32.CloseHandle(hProcess);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[KPatchCore] Validation error for process {process.Id}: {ex.Message}");
            return false;  // If we can't validate, assume invalid
        }
    }
}
