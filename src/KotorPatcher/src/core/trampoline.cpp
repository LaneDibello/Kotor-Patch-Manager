#include "trampoline.h"
#include "platform.h"

#include <cstring>
#include <vector>

// SIMPLE 5-byte relative JMP trampolines that replace the instructions at the
// hook point. The unprotect/write/reprotect and instruction-cache handling live
// in Platform::WriteCode, so this file is pure x86 byte layout. Both targets are
// 32-bit, so a code address fits a uint32_t and reinterpret_cast to/from it
// loses nothing.

namespace KotorPatcher {
    namespace Trampoline {

        bool VerifyBytes(uint32_t address, const uint8_t* expected, std::size_t length) {
            if (!expected || length == 0) {
                return false;
            }
            const uint8_t* actual = reinterpret_cast<const uint8_t*>(address);
            return std::memcmp(actual, expected, length) == 0;
        }

        bool WriteNoOps(uint32_t startAddress, std::size_t length) {
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

        bool WriteJump(uint32_t address, void* target) {
            // E9 <rel32>, where rel32 = target - (address + 5). The +5 is the size
            // of the instruction the CPU has already consumed when it reads rel32.
            uint32_t offset = reinterpret_cast<uint32_t>(target) - (address + 5);
            uint8_t jmp[5];
            jmp[0] = 0xE9;
            std::memcpy(&jmp[1], &offset, 4);
            if (!Platform::WriteCode(reinterpret_cast<void*>(address), jmp, sizeof(jmp))) {
                Platform::Log("[Trampoline] Failed to write JMP\n");
                return false;
            }
            return true;
        }

        bool WriteCall(uint32_t address, void* target) {
            // E8 <rel32>: like WriteJump but pushes a return address first.
            uint32_t offset = reinterpret_cast<uint32_t>(target) - (address + 5);
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
