#pragma once
#include <cstddef>
#include <cstdint>

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
//
// The unprotect/write/reprotect and instruction-cache handling live behind
// Platform::WriteCode, so this module is pure x86 byte layout.
//
// Addresses are uintptr_t because the engine dereferences them: 32 bits on the
// i686/i386 targets, 64 on the macOS x86_64 dylib.

namespace KotorPatcher {
    namespace Trampoline {
        // Displacement a 5-byte E9/E8 at `instructionAddress` needs to reach `target`.
        // Returns false when the target is too far for a signed 32-bit displacement,
        // which can only happen on a 64-bit target.
        bool ComputeRel32(uintptr_t instructionAddress, uintptr_t target, int32_t& outRel);

        // Write a 5-byte relative JMP at `address` targeting `target`.
        bool WriteJump(uintptr_t address, void* target);

        // Write a 5-byte relative CALL at `address` targeting `target`.
        bool WriteCall(uintptr_t address, void* target);

        // Compare `length` bytes at `address` against `expected`. Used as the
        // wrong-game-version guard before any patch is applied.
        bool VerifyBytes(uintptr_t address, const uint8_t* expected, std::size_t length);

        // Overwrite `length` bytes at `startAddress` with NOPs (0x90).
        bool WriteNoOps(uintptr_t startAddress, std::size_t length);
    }
}
