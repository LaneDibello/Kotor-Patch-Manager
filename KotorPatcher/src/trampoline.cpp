#include "trampoline.h"
#include <cstring>

// NOTE: Current implementation uses SIMPLE trampolines (5-byte relative JMP)
// that completely replace original instructions at the hook point.
//
// LIMITATION: Original instructions are lost and cannot be executed by patches.
// Patches must completely replace the hooked function's behavior.
//
// FUTURE: Implement "stolen bytes" detour trampolines that preserve original
// instructions in an allocated trampoline, allowing patches to call original code.
// See README.md "Future Enhancements" section for details.

namespace KotorPatcher {
    namespace Trampoline {

        bool UnprotectMemory(DWORD address, size_t size, DWORD* oldProtect) {
            // Make the memory region writable
            // Most game code is read-only or read-execute by default
            return VirtualProtect(
                reinterpret_cast<LPVOID>(address),
                size,
                PAGE_EXECUTE_READWRITE,  // Allow read, write, and execute
                oldProtect
            ) != 0;
        }

        bool ProtectMemory(DWORD address, size_t size, DWORD oldProtect) {
            DWORD dummy;
            return VirtualProtect(
                reinterpret_cast<LPVOID>(address),
                size,
                oldProtect,  // Restore original protection
                &dummy
            ) != 0;
        }

        bool VerifyBytes(DWORD address, const BYTE* expected, size_t length) {
            if (!expected || length == 0) {
                return false;
            }

            // Compare the bytes at the address with what we expect
            const BYTE* actual = reinterpret_cast<const BYTE*>(address);

            // Use memcmp for efficient comparison
            return memcmp(actual, expected, length) == 0;
        }

        bool WriteJump(DWORD address, void* target) {
            // We're writing a 5-byte relative JMP instruction:
            // E9 [4-byte relative offset]

            // Calculate the relative offset
            // Formula: offset = target - (address + 5)
            // The +5 is because the CPU reads the offset AFTER the 5-byte instruction
            DWORD offset = reinterpret_cast<DWORD>(target) - (address + 5);

            // Build the instruction bytes
            BYTE jmpInstruction[5];
            jmpInstruction[0] = 0xE9;  // JMP opcode
            memcpy(&jmpInstruction[1], &offset, 4);  // 4-byte offset

            // Unprotect memory
            DWORD oldProtect;
            if (!UnprotectMemory(address, 5, &oldProtect)) {
                OutputDebugStringA("[Trampoline] Failed to unprotect memory for JMP\n");
                return false;
            }

            // Write the instruction
            memcpy(reinterpret_cast<void*>(address), jmpInstruction, 5);

            // Restore memory protection
            if (!ProtectMemory(address, 5, oldProtect)) {
                OutputDebugStringA("[Trampoline] Warning: Failed to restore memory protection\n");
                // Not a critical failure - the jump still works
            }

            // Flush instruction cache to ensure CPU sees the new code
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), 5);

            return true;
        }

        bool WriteCall(DWORD address, void* target) {
            // Similar to WriteJump, but uses CALL instruction (E8 instead of E9)
            // CALL pushes return address on stack before jumping

            DWORD offset = reinterpret_cast<DWORD>(target) - (address + 5);

            BYTE callInstruction[5];
            callInstruction[0] = 0xE8;  // CALL opcode
            memcpy(&callInstruction[1], &offset, 4);

            DWORD oldProtect;
            if (!UnprotectMemory(address, 5, &oldProtect)) {
                OutputDebugStringA("[Trampoline] Failed to unprotect memory for CALL\n");
                return false;
            }

            memcpy(reinterpret_cast<void*>(address), callInstruction, 5);

            if (!ProtectMemory(address, 5, oldProtect)) {
                OutputDebugStringA("[Trampoline] Warning: Failed to restore memory protection\n");
            }

            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), 5);

            return true;
        }

    } // namespace Trampoline
} // namespace KotorPatcher