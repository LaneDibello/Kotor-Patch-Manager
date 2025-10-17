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
    /// <returns>Result containing Process object or error</returns>
    public static PatchResult<Process> LaunchWithInjection(
        string gameExePath,
        string dllPath,
        string? commandLineArgs = null)
    {
        if (!File.Exists(gameExePath))
        {
            return PatchResult<Process>.Fail($"Game executable not found: {gameExePath}");
        }

        if (!File.Exists(dllPath))
        {
            return PatchResult<Process>.Fail($"DLL not found: {dllPath}");
        }

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
            // Get LoadLibraryA address from kernel32.dll
            var hKernel32 = GetModuleHandle("kernel32.dll");
            if (hKernel32 == IntPtr.Zero)
            {
                return PatchResult.Fail("Failed to get kernel32.dll module handle");
            }

            var pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
            if (pLoadLibraryA == IntPtr.Zero)
            {
                return PatchResult.Fail("Failed to get LoadLibraryA address");
            }

            // Allocate memory in the target process for the DLL path
            var dllPathBytes = Encoding.ASCII.GetBytes(dllPath + '\0');
            var pDllPath = VirtualAllocEx(
                hProcess,
                IntPtr.Zero,
                (uint)dllPathBytes.Length,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE);

            if (pDllPath == IntPtr.Zero)
            {
                var error = Marshal.GetLastWin32Error();
                return PatchResult.Fail($"Failed to allocate memory in target process (error {error})");
            }

            // Write the DLL path to the allocated memory
            var writeSuccess = WriteProcessMemory(
                hProcess,
                pDllPath,
                dllPathBytes,
                (uint)dllPathBytes.Length,
                out var bytesWritten);

            if (!writeSuccess || bytesWritten.ToUInt32() != dllPathBytes.Length)
            {
                var error = Marshal.GetLastWin32Error();
                return PatchResult.Fail($"Failed to write DLL path to target process (error {error})");
            }

            // Create a remote thread that calls LoadLibraryA with the DLL path
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
                return PatchResult.Fail($"Failed to create remote thread (error {error})");
            }

            // Wait for the thread to complete (LoadLibrary to finish)
            const uint INFINITE = 0xFFFFFFFF;
            var waitResult = WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            if (waitResult != 0) // 0 = WAIT_OBJECT_0
            {
                return PatchResult.Fail("Remote thread did not complete successfully");
            }

            return PatchResult.Ok($"Successfully injected {Path.GetFileName(dllPath)}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"DLL injection failed: {ex.Message}");
        }
    }
}
