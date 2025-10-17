# PE Import Injection Implementation Notes

## Current Status

The `LoaderInjector.cs` has a basic implementation of PE import table modification, but there are significant challenges with this approach.

## The Problem

Modifying PE import tables is **extremely complex** and error-prone because:

1. **Space Constraints**: The import directory must be contiguous, so adding a new import requires either:
   - Finding a code cave large enough for the entire new import directory
   - Creating a new section (requires fixing ALL pointers in the PE)
   - Relocating the existing import directory to a larger space

2. **Multiple Structures**: Import tables require updating:
   - Import Directory Table (array of IMAGE_IMPORT_DESCRIPTOR)
   - Import Lookup Table (ILT) - list of imported function names
   - Import Address Table (IAT) - runtime function pointers
   - DLL name strings
   - Function name strings

3. **PE Header Updates**: Many fields must be updated:
   - Data Directory entry for imports (size and RVA)
   - Section characteristics and sizes
   - Import table RVAs throughout the PE
   - Bound import information (if present)

4. **Loader Complexity**: Windows PE loader has specific requirements that must be met exactly

## Alternative Approaches

### Approach 1: DLL Hijacking (Safer)
Instead of modifying the import table, use **DLL search order hijacking**:
- Place `version.dll` or another commonly-imported system DLL in game directory
- This proxy DLL loads the real system DLL
- Then loads `kotor_patcher.dll`
- No PE modification required!

**Pros**: No risk of corrupting executable
**Cons**: Requires maintaining a proxy DLL per system DLL

### Approach 2: AppInit_DLLs (Global)
Use Windows registry `AppInit_DLLs` feature:
- Register `kotor_patcher.dll` to load into all processes
- Filter in DllMain to only activate for KOTOR

**Pros**: No PE modification
**Cons**: Global injection (affects all processes), deprecated in Windows 10+

### Approach 3: Launcher Application
Create a launcher that:
- Starts KOTOR suspended
- Injects `kotor_patcher.dll` using `CreateRemoteThread` + `LoadLibrary`
- Resumes execution

**Pros**: Clean, no PE modification, full control
**Cons**: Requires separate launcher executable

### Approach 4: External Tool
Use existing PE modification tools:
- CFF Explorer (GUI)
- LordPE (CLI)
- PE-bear
- IDA Pro scripting

**Pros**: Battle-tested, reliable
**Cons**: External dependency

### Approach 5: Better PE Library
Use a library specifically designed for PE modification:
- **LIEF** (C++ with Python bindings) - excellent for PE manipulation
- **PEFile** (Python) - robust PE parser/writer
- Shell out to Python script from C#

**Pros**: Reliable PE modification
**Cons**: External dependency on Python/C++

## Recommended Approach for MVP

**Use Approach 3: Launcher Application**

Create `KotorPatcherLauncher.exe` that:
1. Locates KOTOR executable
2. Starts it with `CREATE_SUSPENDED`
3. Injects `kotor_patcher.dll` via `CreateRemoteThread`
4. Resumes main thread
5. Optionally: Monitor for crashes and provide error reporting

### Launcher Implementation (C#)

```csharp
public static class ProcessInjector
{
    [DllImport("kernel32.dll")]
    private static extern IntPtr OpenProcess(int dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll")]
    private static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress,
        uint dwSize, uint flAllocationType, uint flProtect);

    [DllImport("kernel32.dll")]
    private static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress,
        byte[] lpBuffer, uint nSize, out UIntPtr lpNumberOfBytesWritten);

    [DllImport("kernel32.dll")]
    private static extern IntPtr CreateRemoteThread(IntPtr hProcess,
        IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress,
        IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

    public static PatchResult InjectDll(int processId, string dllPath)
    {
        // Implementation of DLL injection
        // This is a well-known technique with many examples online
    }
}
```

## Current LoaderInjector.cs Status

The current implementation:
- ✅ Can read import tables
- ✅ Can detect if patcher DLL is already injected
- ✅ Has basic structure for manual injection
- ⚠️ Manual injection has **limitations**:
  - Only works if there's a code cave in the import section
  - Doesn't properly set up IAT/ILT (FirstThunk = 0)
  - Doesn't handle bound imports
  - May not work with all executables

**Recommendation**: Mark `InjectLoader()` as experimental and prefer launcher approach

## Testing Strategy

1. **Test with dummy executable** first
2. **Verify with PE analysis tools** (CFF Explorer, PE-bear)
3. **Test on KOTOR backup** copy
4. **Have multiple backups** before testing

## Implementation Priority

For MVP completion:

1. ✅ Complete Phase 5 API (LoaderInjector.cs exists)
2. ⏭️ Move to Phase 6 (Orchestration)
3. ⏭️ Move to Phase 7 (Console app)
4. 🔄 Return to improve PE injection or implement launcher approach

The current LoaderInjector provides the API surface needed for orchestration layers, even if the actual injection is placeholder/experimental.

## References

- [PE Format Specification](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Import Table Structure](https://0xrick.github.io/win-internals/pe3/)
- [LIEF Library](https://lief.quarkslab.com/)
- [DLL Injection Techniques](https://www.ired.team/offensive-security/code-injection-process-injection/dll-injection)
