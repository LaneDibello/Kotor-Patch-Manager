#pragma once
#include <windows.h>

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