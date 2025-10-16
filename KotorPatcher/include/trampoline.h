#pragma once
#include <windows.h>

// Trampoline Module: Memory patching for runtime code injection
//
// CURRENT IMPLEMENTATION: Simple 5-byte JMP trampolines
//   - Overwrites original instructions at hook point
//   - Patch functions cannot call original code
//   - Best for function entry point hooks
//
// FUTURE ENHANCEMENT: Detour trampolines with stolen bytes
//   - Preserve original instructions in allocated trampoline
//   - Allow patches to call original code before/after their logic
//   - Support mid-function hooks safely
//   - See README.md for implementation plan

namespace KotorPatcher {
    namespace Trampoline {
        // Write a 5-byte relative JMP instruction
        bool WriteJump(DWORD address, void* target);

        // Write a 5-byte CALL instruction
        bool WriteCall(DWORD address, void* target);

        // Verify original bytes at address before patching
        bool VerifyBytes(DWORD address, const BYTE* expected, size_t length);

        // Make memory region writable (and store old protection)
        bool UnprotectMemory(DWORD address, size_t size, DWORD* oldProtect);

        // Restore original memory protection
        bool ProtectMemory(DWORD address, size_t size, DWORD oldProtect);
    }
}