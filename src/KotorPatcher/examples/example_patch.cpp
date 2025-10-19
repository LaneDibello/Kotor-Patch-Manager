// example_patch.cpp
// Example patch DLL demonstrating how to use the KOTOR Patcher wrapper system
//
// Build as a 32-bit DLL and export your patch functions
// The patcher will call your functions with a PatchContext pointer

#include <windows.h>

// If building against the patcher headers, you can include:
// #include "wrapper_context.h"
// and use: KotorPatcher::Wrappers::PatchContext
//
// Otherwise, copy the structure definition:
struct PatchContext {
    // General-purpose registers (in PUSHAD order)
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD esp_at_pushad;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;

    // Flags register
    DWORD eflags;

    // Original stack pointer
    DWORD original_esp;

    // Return address
    DWORD return_address;

    // Pointer to original function (for detour trampolines)
    void* original_function;

    // Helper methods
    DWORD GetParameter(int index) const {
        const DWORD* stack = reinterpret_cast<const DWORD*>(original_esp);
        return stack[index + 1];  // +1 to skip return address
    }

    void SetReturnValue(DWORD value) {
        eax = value;
    }

    DWORD GetReturnValue() const {
        return eax;
    }
};

// ===== Example 1: Simple Inspection Hook =====
// Just logs information, doesn't modify anything
// Config: type = "inline" (default)

extern "C" __declspec(dllexport)
void LogFunctionCall(PatchContext* ctx) {
    char msg[256];
    sprintf_s(msg, "[Patch] Function called! EAX=0x%08X, ECX=0x%08X\n",
        ctx->eax, ctx->ecx);
    OutputDebugStringA(msg);

    // No modifications - all registers will be restored automatically
}

// ===== Example 2: Modify Return Value =====
// Changes the return value (EAX) of a function
// Config: type = "inline", exclude_from_restore = ["eax"]

extern "C" __declspec(dllexport)
void ForceReturnValue(PatchContext* ctx) {
    DWORD originalReturn = ctx->GetReturnValue();

    OutputDebugStringA("[Patch] Intercepted return value\n");

    // Force function to return 42 instead of original value
    ctx->SetReturnValue(42);

    char msg[128];
    sprintf_s(msg, "[Patch] Changed return: 0x%08X -> 0x%08X\n",
        originalReturn, 42);
    OutputDebugStringA(msg);
}

// ===== Example 3: Inspect Function Parameters =====
// Reads parameters passed to the hooked function
// Config: type = "inline"

extern "C" __declspec(dllexport)
void InspectParameters(PatchContext* ctx) {
    // Assuming __stdcall or __cdecl function with 2 int parameters
    DWORD param1 = ctx->GetParameter(0);
    DWORD param2 = ctx->GetParameter(1);

    char msg[256];
    sprintf_s(msg, "[Patch] Function called with params: %d, %d\n",
        param1, param2);
    OutputDebugStringA(msg);
}

// ===== Example 4: Modify Multiple Registers =====
// Sets output parameters via registers (common in x86 calling conventions)
// Config: type = "inline", exclude_from_restore = ["eax", "edx"]

extern "C" __declspec(dllexport)
void SetOutputParameters(PatchContext* ctx) {
    // Simulate a function that returns two values via EAX and EDX
    ctx->eax = 100;   // Primary return value
    ctx->edx = 200;   // Secondary return value

    OutputDebugStringA("[Patch] Set EAX=100, EDX=200\n");
}

// ===== Example 5: Conditional Behavior =====
// Modifies behavior based on input
// Config: type = "inline", exclude_from_restore = ["eax"]

extern "C" __declspec(dllexport)
void ConditionalPatch(PatchContext* ctx) {
    DWORD param = ctx->GetParameter(0);

    if (param > 1000) {
        // Clamp large values
        ctx->SetReturnValue(1000);
        OutputDebugStringA("[Patch] Clamped large value to 1000\n");
    }
    else if (param == 0) {
        // Prevent zero
        ctx->SetReturnValue(1);
        OutputDebugStringA("[Patch] Prevented zero, set to 1\n");
    }
    // Otherwise, let original value pass through (don't modify EAX)
}

// ===== Example 6: Modify Flags =====
// Changes EFLAGS (e.g., force a condition to be true/false)
// Config: type = "inline"

extern "C" __declspec(dllexport)
void ForceZeroFlag(PatchContext* ctx) {
    const DWORD FLAG_ZERO = 0x0040;

    // Force the Zero Flag to be set
    ctx->eflags |= FLAG_ZERO;

    OutputDebugStringA("[Patch] Forced Zero Flag on\n");
}

// ===== Example 7: Call Original Function (Future - Detours) =====
// When detour trampolines are implemented, you can call the original
// Config: type = "wrap" (requires Phase 2 detours)

extern "C" __declspec(dllexport)
void WrapOriginalFunction(PatchContext* ctx) {
    OutputDebugStringA("[Patch] Before original function\n");

    // In Phase 2, you'll be able to do:
    // typedef int (__stdcall *OriginalFunc)(int, int);
    // OriginalFunc original = (OriginalFunc)ctx->original_function;
    // int result = original(ctx->GetParameter(0), ctx->GetParameter(1));
    // ctx->SetReturnValue(result);

    OutputDebugStringA("[Patch] After original function\n");
}

// ===== Example 8: Advanced - Memory Manipulation =====
// Read/write game memory
// Config: type = "inline"

extern "C" __declspec(dllexport)
void MemoryPatch(PatchContext* ctx) {
    // Example: Read a global variable at a known address
    DWORD* globalVar = reinterpret_cast<DWORD*>(0x00600000);

    char msg[128];
    sprintf_s(msg, "[Patch] Global variable value: %d\n", *globalVar);
    OutputDebugStringA(msg);

    // Modify the global variable
    *globalVar = 999;

    OutputDebugStringA("[Patch] Modified global variable\n");
}

// ===== Example 9: Legacy REPLACE Mode =====
// For advanced users who want full control (no wrapper)
// Config: type = "replace"
// NOTE: You must handle stack frame, register preservation, etc.

extern "C" __declspec(dllexport) __declspec(naked)
void LegacyAssemblyPatch() {
    __asm {
        // Save registers manually
        push ebp
        mov ebp, esp
        pushad

        // Your patch logic here
        // (call C functions if needed)

        // Restore registers
        popad
        pop ebp

        // Return to caller
        ret
    }
}

// ===== DLL Entry Point =====

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        OutputDebugStringA("[ExamplePatch] Patch DLL loaded\n");
        break;
    case DLL_PROCESS_DETACH:
        OutputDebugStringA("[ExamplePatch] Patch DLL unloaded\n");
        break;
    }
    return TRUE;
}
