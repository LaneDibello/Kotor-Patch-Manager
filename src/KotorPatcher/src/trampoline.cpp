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

        bool WriteNoOps(DWORD startAddress, size_t length) {
            // Sanity checks
            if (length == 0) {
                OutputDebugStringA("[Trampoline] WriteNoOps: length is 0, nothing to write\n");
                return true;
            }
            // Prevent 32-bit wrap/overflow for the DWORD + length calculation
            DWORD endAddr64 = static_cast<DWORD>(startAddress) + static_cast<DWORD>(length);
            if (endAddr64 > 0xFFFFFFFFULL) {
                OutputDebugStringA("[Trampoline] WriteNoOps: requested range overflows 32-bit address space\n");
                return false;
            }

            LPVOID targetPtr = reinterpret_cast<LPVOID>(static_cast<uintptr_t>(startAddress));
            SIZE_T byteCount = static_cast<SIZE_T>(length);

            // Change protection to RWX so we can write instructions
            DWORD oldProtect = 0;
            if (!VirtualProtect(targetPtr, byteCount, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                OutputDebugStringA("[Trampoline] WriteNoOps: VirtualProtect failed to unprotect memory\n");
                return false;
            }

            // Write NOP (0x90) bytes
            // Use volatile pointer to avoid compiler optimizations that might skip the write
            volatile BYTE* p = reinterpret_cast<volatile BYTE*>(targetPtr);
            for (SIZE_T i = 0; i < byteCount; ++i) {
                p[i] = 0x90;
            }

            // Ensure CPU sees the updated instructions
            if (!FlushInstructionCache(GetCurrentProcess(), targetPtr, byteCount)) {
                OutputDebugStringA("[Trampoline] WriteNoOps: FlushInstructionCache failed\n");
                // Not a fatal error here; proceed to attempt to restore protection
            }

            // Restore original memory protection
            DWORD tmp;
            if (!VirtualProtect(targetPtr, byteCount, oldProtect, &tmp)) {
                OutputDebugStringA("[Trampoline] WriteNoOps: Warning: Failed to restore memory protection\n");
                // Not treated as fatal (the NOPs are already written)
            }

            return true;
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