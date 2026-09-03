#include "trampoline.h"
#include "platform.h"

#include <cstring>
#include <vector>

// SIMPLE 5-byte relative JMP trampolines that replace the instructions at the
// hook point. The unprotect/write/reprotect and instruction-cache handling live
// in Platform::WriteCode, so this file is pure x86 byte layout.

namespace KotorPatcher {
    namespace Trampoline {

        bool VerifyBytes(uintptr_t address, const uint8_t* expected, std::size_t length) {
            if (!expected || length == 0) {
                return false;
            }
            const uint8_t* actual = reinterpret_cast<const uint8_t*>(address);
            return std::memcmp(actual, expected, length) == 0;
        }

        bool WriteNoOps(uintptr_t startAddress, std::size_t length) {
            if (length == 0) {
                // Nothing to do; the hook fit exactly in 5 bytes.
                return true;
            }
            std::vector<uint8_t> nops(length, 0x90);
            if (!Platform::WriteCode(reinterpret_cast<void*>(startAddress), nops.data(), length)) {
                Platform::Log("[Trampoline] WriteNoOps: failed to write NOPs\n");
                return false;
            }
            return true;
        }

        bool ComputeRel32(uintptr_t instructionAddress, uintptr_t target, int32_t& outRel) {
            // The +5 is the size of the instruction the CPU has already consumed when
            // it reads rel32. Deliberately subtracted in uintptr_t so the arithmetic
            // wraps in the target's pointer width, as the CPU's does.
            uintptr_t delta = target - (instructionAddress + 5);
            int32_t rel = static_cast<int32_t>(delta);

            // The CPU sign-extends rel32 before adding it, so the jump lands on `target`
            // only when that sign extension gives the delta back. Always true at 32 bits.
            if (static_cast<uintptr_t>(static_cast<intptr_t>(rel)) != delta) {
                return false;
            }

            outRel = rel;
            return true;
        }

        bool WriteJump(uintptr_t address, void* target) {
            int32_t offset = 0;
            if (!ComputeRel32(address, reinterpret_cast<uintptr_t>(target), offset)) {
                Platform::Log("[Trampoline] JMP target is out of rel32 range\n");
                return false;
            }
            uint8_t jmp[5];
            jmp[0] = 0xE9;
            std::memcpy(&jmp[1], &offset, 4);
            if (!Platform::WriteCode(reinterpret_cast<void*>(address), jmp, sizeof(jmp))) {
                Platform::Log("[Trampoline] Failed to write JMP\n");
                return false;
            }
            return true;
        }

        bool WriteCall(uintptr_t address, void* target) {
            // E8 <rel32>: like WriteJump but pushes a return address first.
            int32_t offset = 0;
            if (!ComputeRel32(address, reinterpret_cast<uintptr_t>(target), offset)) {
                Platform::Log("[Trampoline] CALL target is out of rel32 range\n");
                return false;
            }
            uint8_t call[5];
            call[0] = 0xE8;
            std::memcpy(&call[1], &offset, 4);
            if (!Platform::WriteCode(reinterpret_cast<void*>(address), call, sizeof(call))) {
                Platform::Log("[Trampoline] Failed to write CALL\n");
                return false;
            }
            return true;
        }

    } // namespace Trampoline
} // namespace KotorPatcher
