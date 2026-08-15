#include "X86InlineDetour.h"

#include <windows.h>

#include <cstdint>
#include <cstring>

namespace {
    static_assert(sizeof(void*) == 4, "Shader Debug inline detours require an x86 build");

    bool DecodeInstructionLength(const std::uint8_t* code, std::size_t& length) {
        std::size_t offset = 0;
        bool operand16 = false;
        bool address16 = false;
        while (offset < 15) {
            const std::uint8_t prefix = code[offset];
            if (prefix == 0x66) {
                operand16 = true;
            } else if (prefix == 0x67) {
                address16 = true;
            } else if (prefix != 0xF0 && prefix != 0xF2 && prefix != 0xF3 &&
                       prefix != 0x26 && prefix != 0x2E && prefix != 0x36 &&
                       prefix != 0x3E && prefix != 0x64 && prefix != 0x65) {
                break;
            }
            ++offset;
        }

        if (offset >= 15) {
            return false;
        }

        const std::uint8_t opcode = code[offset++];
        bool hasModRm = false;
        std::size_t immediateSize = 0;

        if (opcode == 0x0F) {
            if (offset >= 15) {
                return false;
            }
            const std::uint8_t secondOpcode = code[offset++];
            if (secondOpcode >= 0x80 && secondOpcode <= 0x8F) {
                return false;
            }
            if (secondOpcode == 0x38 || secondOpcode == 0x3A) {
                if (offset >= 15) {
                    return false;
                }
                ++offset;
                hasModRm = true;
                immediateSize = secondOpcode == 0x3A ? 1 : 0;
            } else {
                switch (secondOpcode) {
                case 0x05: case 0x06: case 0x07: case 0x08: case 0x09:
                case 0x0B: case 0x30: case 0x31: case 0x32: case 0x33:
                case 0x34: case 0x35: case 0x37: case 0x77:
                case 0xA0: case 0xA1: case 0xA2: case 0xA8: case 0xA9:
                    break;
                default:
                    if (secondOpcode >= 0xC8 && secondOpcode <= 0xCF) {
                        break;
                    }
                    hasModRm = true;
                    if (secondOpcode == 0x70 || secondOpcode == 0x71 ||
                        secondOpcode == 0x72 || secondOpcode == 0x73 ||
                        secondOpcode == 0xA4 || secondOpcode == 0xAC ||
                        secondOpcode == 0xBA || secondOpcode == 0xC2 ||
                        secondOpcode == 0xC4 || secondOpcode == 0xC5 ||
                        secondOpcode == 0xC6) {
                        immediateSize = 1;
                    }
                    break;
                }
            }
        } else {
            if ((opcode >= 0x70 && opcode <= 0x7F) ||
                (opcode >= 0xE0 && opcode <= 0xE9) || opcode == 0xEB) {
                return false;
            }

            if ((opcode & 0xC7) == 0x04) {
                immediateSize = 1;
            } else if ((opcode & 0xC7) == 0x05) {
                immediateSize = operand16 ? 2 : 4;
            } else if (opcode >= 0xB0 && opcode <= 0xB7) {
                immediateSize = 1;
            } else if (opcode >= 0xB8 && opcode <= 0xBF) {
                immediateSize = operand16 ? 2 : 4;
            } else {
                switch (opcode) {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x08: case 0x09: case 0x0A: case 0x0B:
                case 0x10: case 0x11: case 0x12: case 0x13:
                case 0x18: case 0x19: case 0x1A: case 0x1B:
                case 0x20: case 0x21: case 0x22: case 0x23:
                case 0x28: case 0x29: case 0x2A: case 0x2B:
                case 0x30: case 0x31: case 0x32: case 0x33:
                case 0x38: case 0x39: case 0x3A: case 0x3B:
                case 0x62: case 0x63: case 0x84: case 0x85:
                case 0x86: case 0x87: case 0x88: case 0x89:
                case 0x8A: case 0x8B: case 0x8C: case 0x8D:
                case 0x8E: case 0x8F: case 0xC4: case 0xC5:
                case 0xD0: case 0xD1: case 0xD2: case 0xD3:
                case 0xFE: case 0xFF:
                    hasModRm = true;
                    break;
                case 0x69: case 0x81: case 0xC7:
                    hasModRm = true;
                    immediateSize = operand16 ? 2 : 4;
                    break;
                case 0x6B: case 0x80: case 0x82: case 0x83: case 0xC0:
                case 0xC1: case 0xC6:
                    hasModRm = true;
                    immediateSize = 1;
                    break;
                case 0x68:
                    immediateSize = operand16 ? 2 : 4;
                    break;
                case 0x6A: case 0xA8: case 0xCD: case 0xD4: case 0xD5:
                    immediateSize = 1;
                    break;
                case 0xA0: case 0xA1: case 0xA2: case 0xA3:
                    immediateSize = address16 ? 2 : 4;
                    break;
                case 0xA9:
                    immediateSize = operand16 ? 2 : 4;
                    break;
                case 0xC2: case 0xCA:
                    immediateSize = 2;
                    break;
                case 0xC8:
                    immediateSize = 3;
                    break;
                case 0xF6: case 0xF7:
                    hasModRm = true;
                    break;
                case 0x9A: case 0xEA:
                    return false;
                default:
                    if (!((opcode >= 0x40 && opcode <= 0x5F) ||
                          (opcode >= 0x90 && opcode <= 0x9F) ||
                          (opcode >= 0xA4 && opcode <= 0xA7) ||
                          (opcode >= 0xAA && opcode <= 0xAF) ||
                          opcode == 0xC3 ||
                          (opcode >= 0xCC && opcode <= 0xCF) ||
                          (opcode >= 0xD6 && opcode <= 0xD7) ||
                          (opcode >= 0xEC && opcode <= 0xEF) ||
                          (opcode >= 0xF4 && opcode <= 0xFD))) {
                        return false;
                    }
                    break;
                }
            }
        }

        if (hasModRm) {
            if (offset >= 15) {
                return false;
            }
            const std::uint8_t modRm = code[offset++];
            const std::uint8_t mod = modRm >> 6;
            const std::uint8_t reg = (modRm >> 3) & 7;
            const std::uint8_t rm = modRm & 7;

            if (opcode == 0xF6 && reg <= 1) {
                immediateSize = 1;
            } else if (opcode == 0xF7 && reg <= 1) {
                immediateSize = operand16 ? 2 : 4;
            }

            if (mod != 3) {
                if (address16) {
                    if (mod == 0 && rm == 6) {
                        offset += 2;
                    } else if (mod == 1) {
                        offset += 1;
                    } else if (mod == 2) {
                        offset += 2;
                    }
                } else {
                    if (rm == 4) {
                        if (offset >= 15) {
                            return false;
                        }
                        const std::uint8_t sib = code[offset++];
                        if (mod == 0 && (sib & 7) == 5) {
                            offset += 4;
                        }
                    }
                    if (mod == 0 && rm == 5) {
                        offset += 4;
                    } else if (mod == 1) {
                        offset += 1;
                    } else if (mod == 2) {
                        offset += 4;
                    }
                }
            }
        }

        offset += immediateSize;
        if (offset == 0 || offset > 15) {
            return false;
        }
        length = offset;
        return true;
    }

    std::uint8_t* FollowEntryJumps(void* function) {
        auto* target = static_cast<std::uint8_t*>(function);
        for (int i = 0; i < 8; ++i) {
            if (target[0] == 0xE9) {
                const auto displacement = *reinterpret_cast<const std::int32_t*>(target + 1);
                target += 5 + displacement;
            } else if (target[0] == 0xEB) {
                const auto displacement = *reinterpret_cast<const std::int8_t*>(target + 1);
                target += 2 + displacement;
            } else if (target[0] == 0xFF && target[1] == 0x25) {
                const auto pointerAddress = *reinterpret_cast<const std::uint32_t*>(target + 2);
                target = *reinterpret_cast<std::uint8_t**>(static_cast<std::uintptr_t>(pointerAddress));
            } else {
                break;
            }
        }
        return target;
    }
}

namespace x86hook {
    bool InlineDetour::Install(void* entryPoint, void* replacement) {
        if (!entryPoint || !replacement || IsInstalled()) {
            return false;
        }

        auto* target = FollowEntryJumps(entryPoint);
        std::size_t stolenSize = 0;
        while (stolenSize < 5) {
            std::size_t instructionSize = 0;
            if (!DecodeInstructionLength(target + stolenSize, instructionSize) ||
                stolenSize + instructionSize > kMaxStolenBytes) {
                return false;
            }
            stolenSize += instructionSize;
        }

        auto* trampoline = static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, stolenSize + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!trampoline) {
            return false;
        }

        std::memcpy(trampoline, target, stolenSize);
        trampoline[stolenSize] = 0xE9;
        *reinterpret_cast<std::int32_t*>(trampoline + stolenSize + 1) =
            static_cast<std::int32_t>(
                reinterpret_cast<std::uintptr_t>(target + stolenSize) -
                reinterpret_cast<std::uintptr_t>(trampoline + stolenSize + 5));

        DWORD oldProtect = 0;
        if (!VirtualProtect(target, stolenSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            VirtualFree(trampoline, 0, MEM_RELEASE);
            return false;
        }

        target[0] = 0xE9;
        *reinterpret_cast<std::int32_t*>(target + 1) =
            static_cast<std::int32_t>(
                reinterpret_cast<std::uintptr_t>(replacement) -
                reinterpret_cast<std::uintptr_t>(target + 5));
        std::memset(target + 5, 0x90, stolenSize - 5);
        FlushInstructionCache(GetCurrentProcess(), target, stolenSize);

        DWORD ignored = 0;
        VirtualProtect(target, stolenSize, oldProtect, &ignored);
        m_target = target;
        m_replacement = replacement;
        m_trampoline = trampoline;
        return true;
    }

    bool InlineDetour::Covers(void* entryPoint) const {
        if (!IsInstalled() || !entryPoint) {
            return false;
        }
        void* followed = FollowEntryJumps(entryPoint);
        return entryPoint == m_target || followed == m_target || followed == m_replacement;
    }
}


