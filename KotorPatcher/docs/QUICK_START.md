# Quick Start Guide - Testing the Wrapper System

## Prerequisites

- Visual Studio 2022
- Windows SDK
- [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) (for monitoring output)

## Step 1: Build the Patcher

1. Open Visual Studio 2022
2. Open `KotorPatcher.sln` (or just the KotorPatcher project)
3. **IMPORTANT**: Select **Win32** platform (not x64)
4. Configuration: Debug or Release
5. Right-click `KotorPatcher` project → Build

**Expected Output**:
```
Build succeeded
Output: Debug/kotor_patcher.dll (or Release/kotor_patcher.dll)
```

**If Build Fails**:
- Check all new files are in the project
- Verify C++ language standard is C++17
- Check include paths include `external/` directory

## Step 2: Create a Test Patch DLL

Create a simple test patch to verify the system works.

### 2.1 Create New DLL Project

1. File → New → Project
2. "Dynamic-Link Library (DLL)"
3. Name: `TestPatch`
4. **Platform**: Win32 (x86)

### 2.2 Add Test Patch Code

`test_patch.cpp`:
```cpp
#include <windows.h>

// Copy PatchContext definition from wrapper_context.h
// (or include it if building against patcher headers)
struct PatchContext {
    DWORD edi, esi, ebp, esp_at_pushad;
    DWORD ebx, edx, ecx, eax;
    DWORD eflags;
    DWORD original_esp;
    DWORD return_address;
    void* original_function;

    DWORD GetParameter(int index) const {
        const DWORD* stack = reinterpret_cast<const DWORD*>(original_esp);
        return stack[index + 1];
    }

    void SetReturnValue(DWORD value) {
        eax = value;
    }
};

// Test function 1: Simple logging
extern "C" __declspec(dllexport)
void TestLogHook(PatchContext* ctx) {
    char msg[256];
    sprintf_s(msg, "[TestPatch] Hook called! EAX=0x%08X, ECX=0x%08X, EDX=0x%08X\n",
        ctx->eax, ctx->ecx, ctx->edx);
    OutputDebugStringA(msg);
}

// Test function 2: Modify return value
extern "C" __declspec(dllexport)
void TestModifyReturn(PatchContext* ctx) {
    DWORD oldValue = ctx->eax;
    ctx->SetReturnValue(0x12345678);

    char msg[128];
    sprintf_s(msg, "[TestPatch] Changed return: 0x%08X -> 0x12345678\n", oldValue);
    OutputDebugStringA(msg);
}

// Test function 3: REPLACE type (no context)
extern "C" __declspec(dllexport) __declspec(naked)
void TestReplaceHook() {
    __asm {
        // Just log and return
        push offset msg
        call OutputDebugStringA
        xor eax, eax  // Return 0
        ret

    msg:
        db "[TestPatch] REPLACE hook called", 0
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        OutputDebugStringA("[TestPatch] DLL Loaded\n");
    }
    return TRUE;
}
```

### 2.3 Build Test Patch

Build as **Win32 DLL** → `TestPatch.dll`

## Step 3: Create Test Environment

Instead of testing on real KOTOR, create a dummy test executable:

### 3.1 Create Test EXE

`test_app.cpp`:
```cpp
#include <windows.h>
#include <iostream>

// Dummy functions to be hooked
__declspec(noinline)
int TestFunction1(int a, int b) {
    return a + b;
}

__declspec(noinline)
int TestFunction2(int x) {
    return x * 2;
}

__declspec(noinline)
void TestFunction3() {
    std::cout << "Original function 3" << std::endl;
}

int main() {
    std::cout << "Test Application Starting..." << std::endl;

    // Call functions that will be hooked
    int result1 = TestFunction1(10, 20);
    std::cout << "TestFunction1(10, 20) = " << result1 << std::endl;

    int result2 = TestFunction2(42);
    std::cout << "TestFunction2(42) = " << result2 << std::endl;

    TestFunction3();

    std::cout << "Test Application Done. Press Enter..." << std::endl;
    std::cin.get();

    return 0;
}
```

Build as **Win32 Console Application** → `test_app.exe`

### 3.2 Find Hook Addresses

Use a disassembler (like x64dbg or IDA) to find addresses of:
- `TestFunction1`
- `TestFunction2`
- `TestFunction3`

Or, use a debugger:
1. Run `test_app.exe` in debugger
2. Break at function start
3. Note the address (e.g., `0x00401020`)

### 3.3 Get Original Bytes

Use debugger or hex editor to read first 6 bytes at each function address.

Example:
```
0x00401020: 55 8B EC 83 EC 10  (push ebp; mov ebp,esp; sub esp,0x10)
```

## Step 4: Create Test Configuration

`patch_config.toml`:
```toml
# Test Configuration for Wrapper System

[[patches]]
id = "test-patch"
dll = "TestPatch.dll"  # Place in same dir as test_app.exe

  # Test 1: INLINE hook with logging only
  [[patches.hooks]]
  address = 0x00401020  # Replace with actual TestFunction1 address
  function = "TestLogHook"
  original_bytes = [0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x10]  # Replace with actual bytes
  type = "inline"

  # Test 2: INLINE hook that modifies return value
  [[patches.hooks]]
  address = 0x00401040  # Replace with actual TestFunction2 address
  function = "TestModifyReturn"
  original_bytes = [0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08]  # Replace with actual bytes
  type = "inline"
  exclude_from_restore = ["eax"]  # Allow return value modification

  # Test 3: REPLACE hook (legacy mode)
  [[patches.hooks]]
  address = 0x00401060  # Replace with actual TestFunction3 address
  function = "TestReplaceHook"
  original_bytes = [0x55, 0x8B, 0xEC]  # Replace with actual bytes
  type = "replace"
```

## Step 5: Inject Patcher into Test App

### Method 1: Modify Import Table (Proper Way)

Use a PE editor to add `kotor_patcher.dll` to `test_app.exe` imports.

### Method 2: Manual Injection (Quick Test)

Use a DLL injector tool or write a simple loader:

`loader.cpp`:
```cpp
#include <windows.h>
#include <iostream>

int main() {
    // Start test_app.exe suspended
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessA(
        "test_app.exe",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_SUSPENDED,
        nullptr,
        nullptr,
        &si,
        &pi))
    {
        std::cerr << "Failed to create process" << std::endl;
        return 1;
    }

    // Inject kotor_patcher.dll
    LPVOID loadLibAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    const char* dllPath = "C:\\path\\to\\kotor_patcher.dll";

    LPVOID pathAddr = VirtualAllocEx(pi.hProcess, nullptr, strlen(dllPath) + 1,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    WriteProcessMemory(pi.hProcess, pathAddr, dllPath, strlen(dllPath) + 1, nullptr);

    HANDLE hThread = CreateRemoteThread(pi.hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadLibAddr, pathAddr, 0, nullptr);

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // Resume execution
    ResumeThread(pi.hThread);

    // Wait for process
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
```

### Method 3: AppInit_DLLs (Registry - Use Caution)

**Warning**: This affects all processes. Only for isolated test VM.

```reg
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows]
"AppInit_DLLs"="C:\\path\\to\\kotor_patcher.dll"
"LoadAppInit_DLLs"=dword:00000001
```

## Step 6: Run and Monitor

### 6.1 Set Up DebugView

1. Download and run [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview)
2. Capture → Capture Win32
3. Capture → Capture Global Win32

### 6.2 Prepare Files

Place in same directory:
- `test_app.exe`
- `kotor_patcher.dll`
- `TestPatch.dll`
- `patch_config.toml`

### 6.3 Run

Execute `test_app.exe` (or use injection method)

### 6.4 Expected Output in DebugView

```
[KotorPatcher] Using wrapper generator: x86_Win32
[Config] Loaded hook: test-patch -> TestLogHook @ 0x00401020 (6 bytes)
[Config] Loaded hook: test-patch -> TestModifyReturn @ 0x00401040 (6 bytes)
[Config] Loaded hook: test-patch -> TestReplaceHook @ 0x00401060 (3 bytes)
[Config] Successfully loaded 3 patch hook(s)
[Wrapper] Generated INLINE wrapper at 0x12340000 (87 bytes)
[KotorPatcher] Applied INLINE hook at 0x00401020 -> TestLogHook
[Wrapper] Generated INLINE wrapper at 0x12341000 (95 bytes)
[KotorPatcher] Applied INLINE hook at 0x00401040 -> TestModifyReturn
[Wrapper] Generated REPLACE wrapper (direct JMP) at 0x12342000
[KotorPatcher] Applied REPLACE hook at 0x00401060 -> TestReplaceHook
[TestPatch] DLL Loaded
[TestPatch] Hook called! EAX=0x00000000, ECX=0x00000014, EDX=0x0000000A
[TestPatch] Changed return: 0x0000001E -> 0x12345678
[TestPatch] REPLACE hook called
```

## Step 7: Verify Behavior

### Test 1: Logging Hook
- Should see log message with register values
- Original function behavior unchanged

### Test 2: Modified Return
- Function should return `0x12345678` instead of original value
- Test app should show modified result

### Test 3: REPLACE Hook
- Should see log from replace hook
- Original function not called

## Troubleshooting

### Build Errors

**Error**: Cannot open include file 'wrapper_base.h'
- **Fix**: Add `$(ProjectDir)/include` to include directories

**Error**: Unresolved external symbols
- **Fix**: Add `wrapper_x86_win32.cpp` to project

**Error**: Cannot convert HookType
- **Fix**: Ensure using scoped enum `HookType::INLINE` not just `INLINE`

### Runtime Errors

**No debug output**
- Check DebugView is running with Win32 capture enabled
- Verify patcher DLL loaded (check with Process Explorer)

**"Original bytes mismatch"**
- Re-check addresses with debugger
- Verify you copied bytes correctly
- Ensure test app not optimized (use Debug build)

**Crash on hook**
- Check stack alignment
- Verify function signature matches expected calling convention
- Use debugger to see where crash occurs

**Wrapper not generated**
- Check config parsing succeeded
- Verify `type = "inline"` in config
- Look for error messages in DebugView

## Next Steps

Once basic tests work:

1. **Test all hook types** (INLINE, REPLACE)
2. **Test register exclusions** (exclude_from_restore)
3. **Test parameter reading** (GetParameter)
4. **Test flag modifications** (EFLAGS)
5. **Profile performance** (measure wrapper overhead)
6. **Stress test** (many hooks, large patches)

Then move to real KOTOR testing!

## Safety Tips

- **Always backup KOTOR executable** before testing on real game
- **Test on copy of game** first
- **Use isolated test environment** initially
- **Keep DebugView running** to see errors
- **Start simple** (single hook) before complex configs

## Questions?

See:
- `WRAPPER_SYSTEM.md` - Complete implementation guide
- `examples/example_patch.cpp` - More patch examples
- `examples/patch_config.toml` - More config examples
- `README.md` - General patcher documentation

Good luck testing! 🎮🔧
