using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using KPatchCore.Models;

namespace KPatchLauncher;

/// <summary>
/// Handles DLL injection into game processes using Windows API
/// </summary>
public static class ProcessInjector
{
    #region Windows API Constants

    private const uint PROCESS_CREATE_THREAD = 0x0002;
    private const uint PROCESS_QUERY_INFORMATION = 0x0400;
    private const uint PROCESS_VM_OPERATION = 0x0008;
    private const uint PROCESS_VM_WRITE = 0x0020;
    private const uint PROCESS_VM_READ = 0x0010;

    private const uint MEM_COMMIT = 0x1000;
    private const uint MEM_RESERVE = 0x2000;
    private const uint PAGE_READWRITE = 0x04;

    private const uint CREATE_SUSPENDED = 0x00000004;

    #endregion

    #region Windows API P/Invoke

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(
        uint dwDesiredAccess,
        bool bInheritHandle,
        int dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr VirtualAllocEx(
        IntPtr hProcess,
        IntPtr lpAddress,
        uint dwSize,
        uint flAllocationType,
        uint flProtect);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteProcessMemory(
        IntPtr hProcess,
        IntPtr lpBaseAddress,
        byte[] lpBuffer,
        uint nSize,
        out UIntPtr lpNumberOfBytesWritten);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool ReadProcessMemory(
        IntPtr hProcess,
        IntPtr lpBaseAddress,
        [Out] byte[] lpBuffer,
        int dwSize,
        out IntPtr lpNumberOfBytesRead);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateRemoteThread(
        IntPtr hProcess,
        IntPtr lpThreadAttributes,
        uint dwStackSize,
        IntPtr lpStartAddress,
        IntPtr lpParameter,
        uint dwCreationFlags,
        out IntPtr lpThreadId);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool CreateProcess(
        string? lpApplicationName,
        string? lpCommandLine,
        IntPtr lpProcessAttributes,
        IntPtr lpThreadAttributes,
        bool bInheritHandles,
        uint dwCreationFlags,
        IntPtr lpEnvironment,
        string? lpCurrentDirectory,
        ref STARTUPINFO lpStartupInfo,
        out PROCESS_INFORMATION lpProcessInformation);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint ResumeThread(IntPtr hThread);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct STARTUPINFO
    {
        public int cb;
        public string? lpReserved;
        public string? lpDesktop;
        public string? lpTitle;
        public int dwX;
        public int dwY;
        public int dwXSize;
        public int dwYSize;
        public int dwXCountChars;
        public int dwYCountChars;
        public int dwFillAttribute;
        public int dwFlags;
        public short wShowWindow;
        public short cbReserved2;
        public IntPtr lpReserved2;
        public IntPtr hStdInput;
        public IntPtr hStdOutput;
        public IntPtr hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct PROCESS_INFORMATION
    {
        public IntPtr hProcess;
        public IntPtr hThread;
        public int dwProcessId;
        public int dwThreadId;
    }

    #endregion

    /// <summary>
    /// Launches a game executable with DLL injection
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="dllPath">Path to the DLL to inject</param>
    /// <param name="commandLineArgs">Optional command line arguments for the game</param>
    /// <param name="distribution">Game distribution (GOG, Steam, etc.) to determine injection method</param>
    /// <returns>Result containing Process object or error</returns>
    public static PatchResult<Process> LaunchWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs = null,
        Distribution distribution = Distribution.GOG)
    {
        if (!File.Exists(gameExePath))
        {
            return PatchResult<Process>.Fail($"Game executable not found: {gameExePath}");
        }

        if (!File.Exists(dllPath))
        {
            return PatchResult<Process>.Fail($"DLL not found: {dllPath}");
        }

        // Route to appropriate launcher based on distribution
        if (distribution == Distribution.Steam)
        {
            Console.WriteLine("[KPatchLauncher] Detected Steam distribution, using delayed injection method");
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
    private static PatchResult<Process> LaunchDirectWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs)
    {
        try
        {
            // Get absolute paths
            var absGamePath = Path.GetFullPath(gameExePath);
            var absDllPath = Path.GetFullPath(dllPath);

            // Prepare startup info
            var si = new STARTUPINFO
            {
                cb = Marshal.SizeOf(typeof(STARTUPINFO))
            };

            var pi = new PROCESS_INFORMATION();

            // Build command line
            var commandLine = $"\"{absGamePath}\"";
            if (!string.IsNullOrWhiteSpace(commandLineArgs))
            {
                commandLine += $" {commandLineArgs}";
            }

            // Create the process suspended
            var success = CreateProcess(
                absGamePath,
                commandLine,
                IntPtr.Zero,
                IntPtr.Zero,
                false,
                CREATE_SUSPENDED,
                IntPtr.Zero,
                Path.GetDirectoryName(absGamePath),
                ref si,
                out pi);

            if (!success)
            {
                var error = Marshal.GetLastWin32Error();
                return PatchResult<Process>.Fail(
                    $"Failed to create process (error {error}): {gameExePath}");
            }

            try
            {
                // Inject the DLL into the suspended process
                var injectResult = InjectDllIntoProcess(pi.hProcess, absDllPath);

                if (!injectResult.Success)
                {
                    // Injection failed - terminate the process
                    Process.GetProcessById(pi.dwProcessId).Kill();
                    return PatchResult<Process>.Fail(
                        $"DLL injection failed: {injectResult.Error}");
                }

                // Resume the main thread
                var resumeResult = ResumeThread(pi.hThread);
                if (resumeResult == unchecked((uint)-1))
                {
                    var error = Marshal.GetLastWin32Error();
                    Process.GetProcessById(pi.dwProcessId).Kill();
                    return PatchResult<Process>.Fail(
                        $"Failed to resume thread (error {error})");
                }

                // Get the Process object
                var process = Process.GetProcessById(pi.dwProcessId);

                return PatchResult<Process>.Ok(
                    process,
                    $"Successfully launched {Path.GetFileName(gameExePath)} with DLL injection");
            }
            finally
            {
                // Clean up handles
                if (pi.hProcess != IntPtr.Zero) CloseHandle(pi.hProcess);
                if (pi.hThread != IntPtr.Zero) CloseHandle(pi.hThread);
            }
        }
        catch (Exception ex)
        {
            return PatchResult<Process>.Fail($"Launch failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Injects a DLL into a running process
    /// </summary>
    /// <param name="processId">Target process ID</param>
    /// <param name="dllPath">Path to the DLL to inject</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult InjectIntoRunningProcess(int processId, string dllPath)
    {
        if (!File.Exists(dllPath))
        {
            return PatchResult.Fail($"DLL not found: {dllPath}");
        }

        try
        {
            // Open the target process
            var hProcess = OpenProcess(
                PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
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
                CloseHandle(hProcess);
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

            // Get LoadLibraryA address from kernel32.dll
            var hKernel32 = GetModuleHandle("kernel32.dll");
            if (hKernel32 == IntPtr.Zero)
            {
                var msg = "Failed to get kernel32.dll module handle";
                Console.WriteLine($"[Injector] ERROR: {msg}");
                return PatchResult.Fail(msg);
            }
            Console.WriteLine($"[Injector] kernel32.dll handle: 0x{hKernel32:X}");

            var pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
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

            var pDllPath = VirtualAllocEx(
                hProcess,
                IntPtr.Zero,
                (uint)dllPathBytes.Length,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE);

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
            var writeSuccess = WriteProcessMemory(
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
            var hThread = CreateRemoteThread(
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
            const uint INFINITE = 0xFFFFFFFF;
            var waitResult = WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

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
    private static PatchResult<Process> LaunchSteamWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs)
    {
        try
        {
            var absGamePath = Path.GetFullPath(gameExePath);
            var absDllPath = Path.GetFullPath(dllPath);

            Console.WriteLine($"[KPatchLauncher] Launching Steam game: {Path.GetFileName(absGamePath)}");

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
            Console.WriteLine("[KPatchLauncher] Waiting for game process (detecting and skipping Steam bootstrap)...");
            var gameProcess = FindGameProcess(absGamePath, TimeSpan.FromSeconds(30));

            if (gameProcess == null)
            {
                return PatchResult<Process>.Fail(
                    "Could not find valid game process after Steam launch. " +
                    "Ensure Steam is running and the game launches correctly. " +
                    "Check console output for validation details.");
            }

            Console.WriteLine($"[KPatchLauncher] Validated game process found (PID: {gameProcess.Id})");

            // Step 3: Wait for game initialization (window creation)
            Console.WriteLine("[KPatchLauncher] Waiting for game window initialization...");
            if (!WaitForProcessInitialization(gameProcess, TimeSpan.FromSeconds(30)))
            {
                return PatchResult<Process>.Fail(
                    "Timeout waiting for game initialization. " +
                    "The game may have failed to start or Steam decryption took too long.");
            }

            Console.WriteLine("[KPatchLauncher] Game initialized, injecting DLL...");

            // Step 4: Inject DLL into the running process
            var injectResult = InjectIntoRunningProcess(gameProcess.Id, absDllPath);

            if (!injectResult.Success)
            {
                return PatchResult<Process>.Fail(
                    $"Failed to inject DLL into running Steam process: {injectResult.Error}");
            }

            Console.WriteLine("[KPatchLauncher] DLL injected successfully into Steam game");

            return PatchResult<Process>.Ok(
                gameProcess,
                $"Successfully launched {Path.GetFileName(gameExePath)} with delayed injection (Steam)");
        }
        catch (Exception ex)
        {
            return PatchResult<Process>.Fail($"Steam launch failed: {ex.Message}");
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
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        var checkedPids = new HashSet<int>();  // Track PIDs we've already validated

        Console.WriteLine($"[KPatchLauncher] Searching for process: {executableName}");

        while (stopwatch.Elapsed < timeout)
        {
            try
            {
                var processes = Process.GetProcessesByName(executableName);

                foreach (var process in processes)
                {
                    // Skip processes we've already checked
                    if (checkedPids.Contains(process.Id))
                        continue;

                    checkedPids.Add(process.Id);
                    Console.WriteLine($"[KPatchLauncher] Found process candidate: PID {process.Id}, validating...");

                    // Validate this is a real executable, not a bootstrap
                    if (IsValidGameProcess(process))
                    {
                        // Extra stability check: wait 500ms to ensure it doesn't exit immediately
                        Thread.Sleep(500);

                        try
                        {
                            process.Refresh();

                            if (!process.HasExited)
                            {
                                Console.WriteLine($"[KPatchLauncher] Validated and stable process found: PID {process.Id}");
                                return process;
                            }
                            else
                            {
                                Console.WriteLine($"[KPatchLauncher] Process {process.Id} exited after validation, continuing search...");
                            }
                        }
                        catch
                        {
                            Console.WriteLine($"[KPatchLauncher] Process {process.Id} became inaccessible, continuing search...");
                        }
                    }
                    // IsValidGameProcess already logs why validation failed
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[KPatchLauncher] Error during process search: {ex.Message}");
            }

            Thread.Sleep(100);  // Poll every 100ms
        }

        Console.WriteLine($"[KPatchLauncher] Timeout: No valid process found after {timeout.TotalSeconds}s");
        return null;
    }

    /// <summary>
    /// Waits for a process to complete initialization
    /// Detects initialization by checking for main window handle creation
    /// </summary>
    private static bool WaitForProcessInitialization(Process process, TimeSpan timeout)
    {
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();

        while (stopwatch.Elapsed < timeout)
        {
            try
            {
                process.Refresh();

                // Check if process has exited (failed to initialize)
                if (process.HasExited)
                {
                    Console.WriteLine("[KPatchLauncher] Process exited before initialization completed");
                    return false;
                }

                // Window handle creation indicates the game has initialized
                if (process.MainWindowHandle != IntPtr.Zero)
                {
                    Console.WriteLine("[KPatchLauncher] Main window detected, game initialized");
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
            var hProcess = OpenProcess(
                PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                false,
                process.Id);

            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine($"[KPatchLauncher] Could not open process {process.Id} for validation");
                return false;
            }

            try
            {
                // Read DOS header from process base address (0x400000 is typical for Windows executables)
                byte[] dosHeader = new byte[64];
                bool readSuccess = ReadProcessMemory(
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
                        Console.WriteLine($"[KPatchLauncher] Process {process.Id}: Valid PE header detected (MZ signature)");
                        return true;
                    }
                    else
                    {
                        Console.WriteLine($"[KPatchLauncher] Process {process.Id}: Invalid PE header " +
                            $"(got 0x{dosHeader[0]:X2} 0x{dosHeader[1]:X2}, expected 0x4D 0x5A) - likely bootstrap");
                        return false;
                    }
                }
                else
                {
                    Console.WriteLine($"[KPatchLauncher] Process {process.Id}: Could not read memory for validation");

                    // Fallback: Check if process stays alive (bootstrap exits quickly)
                    Thread.Sleep(200);
                    process.Refresh();
                    bool isAlive = !process.HasExited;

                    if (isAlive)
                    {
                        Console.WriteLine($"[KPatchLauncher] Process {process.Id}: Fallback stability check passed");
                    }
                    else
                    {
                        Console.WriteLine($"[KPatchLauncher] Process {process.Id}: Exited during validation");
                    }

                    return isAlive;
                }
            }
            finally
            {
                CloseHandle(hProcess);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[KPatchLauncher] Validation error for process {process.Id}: {ex.Message}");
            return false;  // If we can't validate, assume invalid
        }
    }
}
