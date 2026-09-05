#include "wrapper_x86_64.h"
#include "patcher.h"
#include "platform.h"
#include "trampoline.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

namespace KotorPatcher {
    namespace Wrappers {
        namespace {

            // Register numbers as they appear in ModRM and REX encodings.
            enum Reg : int {
                RAX = 0, RCX = 1, RDX = 2, RBX = 3, RSP = 4, RBP = 5, RSI = 6, RDI = 7,
                R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15
            };

            // Saved in this order, which is what fixes the frame layout below. RSP is
            // absent: it is implicit in the frame, and writing it back from a saved copy
            // would desynchronise the stack. The hardware POPAD skipped ESP for the same
            // reason.
            constexpr Reg kSavedGprs[] = { RAX, RCX, RDX, RBX, RBP, RSI, RDI,
                                           R8, R9, R10, R11, R12, R13, R14, R15 };
            constexpr int kSavedGprCount = static_cast<int>(sizeof(kSavedGprs) / sizeof(kSavedGprs[0]));

            // System V AMD64 argument registers, in order.
            constexpr Reg kIntArgRegs[] = { RDI, RSI, RDX, RCX, R8, R9 };
            constexpr int kMaxIntArgs = static_cast<int>(sizeof(kIntArgRegs) / sizeof(kIntArgRegs[0]));

            // Floating-point arguments go in XMM0..XMM7, a separate sequence from the
            // integer registers: a hook taking (int, float, int) uses RDI, XMM0, RSI.
            constexpr int kMaxSseArgs = 8;

            // The 128 bytes below RSP that a System V leaf function may use as scratch
            // without adjusting RSP. The hooked instruction can be inside such a function,
            // so the wrapper steps over the red zone before touching the stack at all.
            constexpr int kRedZoneSize = 128;

            constexpr int kFlagsSize = 8;
            constexpr int kGprAreaSize = kSavedGprCount * 8;                 // 0x78

            // FXSAVE writes 512 bytes and requires a 16-byte aligned destination.
            // Nothing is known about the game's RSP, so the area carries 15 bytes of
            // slack and the aligned address inside it is computed once there is a
            // scratch register to compute it with.
            constexpr int kFpSaveSize = 512;
            constexpr int kFpAreaOffset = kFlagsSize + kGprAreaSize;         // 0x80
            constexpr int kFpAreaSize = kFpSaveSize + 16;

            // Frame, measured from the anchor the wrapper keeps in RBX:
            //
            //   [RBX + 0x000]            RFLAGS
            //   [RBX + 0x008 .. 0x07F]   the saved GPRs, last pushed nearest the anchor
            //   [RBX + 0x080 .. 0x28F]   the FXSAVE area, plus its alignment slack
            //   [RBX + 0x290 .. 0x30F]   the red zone, stepped over and left alone
            //   [RBX + 0x310]            the game's RSP at the hook site
            constexpr int kFrameSize = kFpAreaOffset + kFpAreaSize + kRedZoneSize;

            // Everything is saved even when the config asks for no restore, so that a
            // parameter sourced from a register always has a value to read. The
            // preserve flags then decide only what is written back.
            constexpr int SavedGprOffset(int index) {
                return kFlagsSize + (kSavedGprCount - 1 - index) * 8;
            }

            struct RegName { Reg reg; const char* wide; const char* narrow; };

            // Both spellings name the same physical register. A macOS hook adapted from a
            // Windows one keeps saying "eax", and silently ignoring that would break the
            // consumed-exit contract, which asks the author to exclude eax from restore.
            constexpr RegName kRegNames[] = {
                { RAX, "rax", "eax" }, { RCX, "rcx", "ecx" }, { RDX, "rdx", "edx" },
                { RBX, "rbx", "ebx" }, { RSP, "rsp", "esp" }, { RBP, "rbp", "ebp" },
                { RSI, "rsi", "esi" }, { RDI, "rdi", "edi" },
                { R8, "r8", "r8d" }, { R9, "r9", "r9d" }, { R10, "r10", "r10d" },
                { R11, "r11", "r11d" }, { R12, "r12", "r12d" }, { R13, "r13", "r13d" },
                { R14, "r14", "r14d" }, { R15, "r15", "r15d" }
            };

            bool LookUpRegister(const std::string& name, Reg& outReg) {
                for (const auto& entry : kRegNames) {
                    if (name == entry.wide || name == entry.narrow) {
                        outReg = entry.reg;
                        return true;
                    }
                }
                return false;
            }

            // Emits into a fixed buffer and refuses to run past the end, so a bad size
            // estimate shows up as a failed hook rather than a corrupted heap.
            class Emitter {
            public:
                Emitter(uint8_t* buffer, std::size_t capacity)
                    : m_begin(buffer), m_cursor(buffer), m_end(buffer + capacity) {}

                void Byte(uint8_t value) {
                    if (m_cursor >= m_end) { m_overflowed = true; return; }
                    *m_cursor++ = value;
                }

                void Bytes(const uint8_t* bytes, std::size_t count) {
                    for (std::size_t i = 0; i < count; ++i) Byte(bytes[i]);
                }

                void Dword(uint32_t value) {
                    for (int i = 0; i < 4; ++i) Byte(static_cast<uint8_t>(value >> (i * 8)));
                }

                bool Overflowed() const { return m_overflowed; }
                uint8_t* Cursor() const { return m_cursor; }
                std::size_t Written() const { return static_cast<std::size_t>(m_cursor - m_begin); }

            private:
                uint8_t* m_begin;
                uint8_t* m_cursor;
                uint8_t* m_end;
                bool m_overflowed = false;
            };

            // REX prefix, omitted when it would carry no information. W selects a 64-bit
            // operand; R and B extend the ModRM reg and rm fields to reach R8..R15.
            void Rex(Emitter& e, bool wide, int regField, int rmField) {
                uint8_t rex = 0x40;
                if (wide) rex |= 0x08;
                if (regField >= 8) rex |= 0x04;
                if (rmField >= 8) rex |= 0x01;
                if (rex != 0x40) e.Byte(rex);
            }

            void PushReg(Emitter& e, int reg) {
                Rex(e, false, 0, reg);
                e.Byte(static_cast<uint8_t>(0x50 | (reg & 7)));
            }

            void PopReg(Emitter& e, int reg) {
                Rex(e, false, 0, reg);
                e.Byte(static_cast<uint8_t>(0x58 | (reg & 7)));
            }

            // `op` reg64, [base + disp32]. Only valid for a base that needs no SIB byte,
            // which the callers satisfy by always using RBX.
            void MemOp(Emitter& e, uint8_t op, int reg, int base, int32_t disp) {
                Rex(e, true, reg, base);
                e.Byte(op);
                e.Byte(static_cast<uint8_t>(0x80 | ((reg & 7) << 3) | (base & 7)));
                e.Dword(static_cast<uint32_t>(disp));
            }

            void MovFromMem(Emitter& e, int dst, int base, int32_t disp) { MemOp(e, 0x8B, dst, base, disp); }
            void LeaFromMem(Emitter& e, int dst, int base, int32_t disp) { MemOp(e, 0x8D, dst, base, disp); }

            // LEA RSP, [RSP + disp32]. Deliberately LEA and not ADD/SUB: it leaves the
            // flags alone, which matters everywhere this is used outside the saved region.
            void AdjustRsp(Emitter& e, int32_t disp) {
                e.Byte(0x48); e.Byte(0x8D); e.Byte(0xA4); e.Byte(0x24);
                e.Dword(static_cast<uint32_t>(disp));
            }

            // Point RAX at the aligned address inside the FP area, then FXSAVE64 or
            // FXRSTOR64 through it. ADD and AND write flags, so both callers run this
            // while the flags are already saved.
            //
            // RAX is borrowed and given back. On the way out it still holds the
            // handler's return value, which the consumed-exit test is about to read and
            // which the caller was told to exclude from restore precisely so it would
            // survive this far.
            void FpSaveArea(Emitter& e, uint8_t modrmReg) {
                e.Byte(0x50);                                             // PUSH RAX
                LeaFromMem(e, RAX, RBX, kFpAreaOffset);
                e.Byte(0x48); e.Byte(0x83); e.Byte(0xC0); e.Byte(0x0F);  // ADD RAX, 15
                e.Byte(0x48); e.Byte(0x83); e.Byte(0xE0); e.Byte(0xF0);  // AND RAX, -16
                e.Byte(0x48); e.Byte(0x0F); e.Byte(0xAE);                // REX.W 0F AE /r
                e.Byte(modrmReg);                                         // [RAX]
                e.Byte(0x58);                                             // POP RAX
            }

            void SaveFpState(Emitter& e)    { FpSaveArea(e, 0x00); }  // FXSAVE64  /0
            void RestoreFpState(Emitter& e) { FpSaveArea(e, 0x08); }  // FXRSTOR64 /1

            // MOVD xmm, r32: moves the raw bits, which is what a float argument in the
            // low half of an SSE register is.
            void MovdToXmm(Emitter& e, int xmm, int gpr) {
                e.Byte(0x66);
                Rex(e, false, xmm, gpr);
                e.Byte(0x0F); e.Byte(0x6E);
                e.Byte(static_cast<uint8_t>(0xC0 | ((xmm & 7) << 3) | (gpr & 7)));
            }

        } // namespace

        namespace {

            int SavedGprIndex(Reg reg) {
                for (int i = 0; i < kSavedGprCount; ++i) {
                    if (kSavedGprs[i] == reg) return i;
                }
                return -1;
            }

            // Excluding either spelling excludes the register.
            bool ShouldRestore(const WrapperConfig& config, Reg reg) {
                for (const auto& entry : kRegNames) {
                    if (entry.reg == reg) {
                        return config.ShouldRestoreRegister(entry.wide) &&
                               config.ShouldRestoreRegister(entry.narrow);
                    }
                }
                return true;
            }

            void EmitRel32(Emitter& e, uint8_t opcode, uintptr_t target, bool& outReachable) {
                e.Byte(opcode);
                int32_t rel = 0;
                outReachable = Trampoline::ComputeRel32(
                    reinterpret_cast<uintptr_t>(e.Cursor()) - 1, target, rel);
                e.Dword(static_cast<uint32_t>(rel));
            }

            // Places one argument in the register the System V convention gives it.
            bool LoadArgument(Emitter& e, const ParameterInfo& param,
                              int& intArgIndex, int& sseArgIndex) {
                std::string source = param.source;
                std::transform(source.begin(), source.end(), source.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                const bool wantsSse = param.type == ParameterType::FLOAT;
                if (wantsSse ? sseArgIndex >= kMaxSseArgs : intArgIndex >= kMaxIntArgs) {
                    Platform::Log("[Wrapper] Too many arguments for the register convention\n");
                    return false;
                }

                Reg sourceReg = RAX;
                if (LookUpRegister(source, sourceReg)) {
                    int index = SavedGprIndex(sourceReg);
                    if (index < 0) {
                        // RSP is the one register with no saved copy. A hook wanting the
                        // game's stack pointer asks for "rsp+0", which is that address.
                        Platform::Log(("[Wrapper] No saved copy of " + source +
                                       "; use rsp+0 for the stack pointer\n").c_str());
                        return false;
                    }
                    if (wantsSse) {
                        MovFromMem(e, RAX, RBX, SavedGprOffset(index));
                        MovdToXmm(e, sseArgIndex++, RAX);
                    } else {
                        MovFromMem(e, kIntArgRegs[intArgIndex++], RBX, SavedGprOffset(index));
                    }
                    return true;
                }

                // "rsp+8", and "esp+8" from a hook adapted off the Windows one. Like the
                // x86 generator, this passes the address of that slot rather than what
                // is in it.
                const bool stackSource = source.size() > 4 &&
                    (source.compare(0, 3, "rsp") == 0 || source.compare(0, 3, "esp") == 0) &&
                    (source[3] == '+' || source[3] == '-');
                if (stackSource) {
                    if (wantsSse) {
                        Platform::Log("[Wrapper] A stack source yields an address, not a float\n");
                        return false;
                    }
                    int userOffset = 0;
                    try {
                        userOffset = std::stoi(source.substr(3));
                    } catch (...) {
                        Platform::Log(("[Wrapper] Invalid stack offset: " + source + "\n").c_str());
                        return false;
                    }
                    LeaFromMem(e, kIntArgRegs[intArgIndex++], RBX, kFrameSize + userOffset);
                    return true;
                }

                Platform::Log(("[Wrapper] Unsupported parameter source: " + source + "\n").c_str());
                return false;
            }

        } // namespace

        WrapperGenerator_x86_64::~WrapperGenerator_x86_64() {
            FreeAllWrappers();
        }

        void WrapperGenerator_x86_64::FreeAllWrappers() {
            for (const auto& wrapper : m_allocatedWrappers) {
                Platform::FreeExec(wrapper.address, wrapper.size);
            }
            m_allocatedWrappers.clear();
        }

        void* WrapperGenerator_x86_64::GenerateWrapper(const WrapperConfig& config) {
            return GenerateDetourWrapper(config);
        }

        void* WrapperGenerator_x86_64::GenerateDetourWrapper(const WrapperConfig& config) {
            // An exact upper bound on what the emitter below can produce, term by term,
            // so the emitter's own bounds check is a backstop and not the design.
            const std::size_t capacity =
                8 + 8                                 // step over the red zone, open the FP area
                + kSavedGprCount * 2                  // push the GPRs
                + 1 + 3                               // PUSHFQ, MOV RBX, RSP
                + 21                                  // address the FP area and FXSAVE64
                + config.parameters.size() * 12       // worst case for one argument
                + 2 + 4 + 5                           // MOV AL, AND RSP, CALL
                + 3 + 8                               // MOV RSP, RBX and the flags slot
                + 21                                  // address the FP area and FXRSTOR64
                + kSavedGprCount * 8                  // pop or step past each GPR
                + 8                                   // close the frame
                + config.originalBytes.size()
                + 36                                  // the consumed-exit branch
                + 5;                                  // the jump back

            // The hook site jumps here and the wrapper jumps back, so the stub has to
            // land within a relative jump of the code it is hooking.
            auto* mem = static_cast<uint8_t*>(Platform::AllocExec(capacity, config.hookAddress));
            if (!mem) {
                Platform::Log("[Wrapper] Failed to allocate wrapper memory\n");
                return nullptr;
            }
            m_allocatedWrappers.push_back({ mem, capacity });

            Emitter e(mem, capacity);

            // ===== PROLOGUE =====
            // The red zone first: the hooked instruction can sit inside a leaf function
            // that is using the 128 bytes below RSP, and every push from here down would
            // land on top of it.
            AdjustRsp(e, -kRedZoneSize);
            AdjustRsp(e, -kFpAreaSize);
            for (int i = 0; i < kSavedGprCount; ++i) {
                PushReg(e, kSavedGprs[i]);
            }
            e.Byte(0x9C);                                       // PUSHFQ
            e.Byte(0x48); e.Byte(0x89); e.Byte(0xE3);           // MOV RBX, RSP

            // The floating-point state goes next, now that RAX is free to address it.
            // Everything FXSAVE covers is caller-saved, so the patch function may
            // destroy all of it: XMM0..XMM15 hold every float the game is working with,
            // and MXCSR, the x87 stack and the x87 control word travel with them.
            SaveFpState(e);

            // ===== ARGUMENTS =====
            int intArgIndex = 0;
            int sseArgIndex = 0;
            for (const auto& param : config.parameters) {
                if (!LoadArgument(e, param, intArgIndex, sseArgIndex)) {
                    return nullptr;
                }
            }
            // AL holds the number of vector registers used, which a variadic callee reads
            // and a normal one ignores.
            e.Byte(0xB0); e.Byte(static_cast<uint8_t>(sseArgIndex));

            // ===== CALL =====
            // The convention wants RSP 16-byte aligned at the call itself, and nothing
            // says what it was at an arbitrary hook site. AND writes flags, which is
            // harmless here because they are already saved.
            e.Byte(0x48); e.Byte(0x83); e.Byte(0xE4); e.Byte(0xF0);   // AND RSP, -16

            bool reachable = false;
            EmitRel32(e, 0xE8, reinterpret_cast<uintptr_t>(config.patchFunction), reachable);
            if (!reachable) {
                Platform::Log("[Wrapper] Patch function is out of rel32 range of the wrapper\n");
                return nullptr;
            }

            // ===== EPILOGUE =====
            e.Byte(0x48); e.Byte(0x89); e.Byte(0xDC);           // MOV RSP, RBX

            // Ahead of the flags, because addressing the area writes them, and ahead of
            // the pops, because it needs RAX.
            if (config.preserveRegisters) {
                RestoreFpState(e);
            }

            if (config.preserveFlags) {
                e.Byte(0x9D);                                   // POPFQ
            } else {
                AdjustRsp(e, kFlagsSize);
            }

            // RBX is always restored: the wrapper took it as its anchor, so the patch
            // function never had a say in its value, and skipping it would hand the
            // game one of our stack addresses instead.
            for (int i = kSavedGprCount - 1; i >= 0; --i) {
                if (kSavedGprs[i] == RBX || ShouldRestore(config, kSavedGprs[i])) {
                    PopReg(e, kSavedGprs[i]);
                } else {
                    // LEA rather than ADD, so stepping over a register the patch wanted
                    // to keep does not disturb the flags just restored above.
                    AdjustRsp(e, 8);
                }
            }

            AdjustRsp(e, kFpAreaSize + kRedZoneSize);

            // ===== STOLEN BYTES =====
            // Run before the consumed-exit test so both paths leave the same state, as
            // if the cut had executed in place. See the header on RIP-relative operands.
            if (!config.skipOriginalBytes) {
                if (config.originalBytes.empty()) {
                    Platform::Log("[Wrapper] ERROR: No original bytes provided for DETOUR hook\n");
                    return nullptr;
                }
                e.Bytes(config.originalBytes.data(), config.originalBytes.size());
            }

            // ===== CONSUMED-EVENT EXIT =====
            // TEST overwrites the flags the stolen bytes just set, which a Jcc at the
            // resume point may be about to read, so the test is wrapped in a save and
            // restore of its own. That save steps over the red zone again, because by
            // now RSP is the game's once more.
            if (config.consumedExitAddress != 0) {
                constexpr uint8_t kConsumedBranchSize = 1 + 8 + 5;  // POPFQ, LEA, JMP rel32
                AdjustRsp(e, -kRedZoneSize);
                e.Byte(0x9C);                                   // PUSHFQ
                e.Byte(0x85); e.Byte(0xC0);                     // TEST EAX, EAX
                e.Byte(0x74); e.Byte(kConsumedBranchSize);      // JZ to the fall-through
                e.Byte(0x9D);                                   // POPFQ
                AdjustRsp(e, kRedZoneSize);
                EmitRel32(e, 0xE9, config.consumedExitAddress, reachable);
                if (!reachable) {
                    Platform::Log("[Wrapper] consumed_exit_address is out of rel32 range\n");
                    return nullptr;
                }
                e.Byte(0x9D);                                   // POPFQ
                AdjustRsp(e, kRedZoneSize);
            }

            // ===== RESUME =====
            const uintptr_t resumeAddress = config.hookAddress + config.originalBytes.size();
            EmitRel32(e, 0xE9, resumeAddress, reachable);
            if (!reachable) {
                Platform::Log("[Wrapper] Resume point is out of rel32 range of the wrapper\n");
                return nullptr;
            }

            if (e.Overflowed()) {
                Platform::Log("[Wrapper] Wrapper exceeded its allocation\n");
                return nullptr;
            }

            if (!Platform::ProtectExec(mem, capacity)) {
                Platform::Log("[Wrapper] Failed to make the wrapper executable\n");
                return nullptr;
            }

            char debugMsg[192];
            snprintf(debugMsg, sizeof(debugMsg),
                "[Wrapper] Generated DETOUR wrapper at 0x%08" PRIXPTR " (%zu bytes)\n",
                reinterpret_cast<uintptr_t>(mem), e.Written());
            Platform::Log(debugMsg);

            return mem;
        }

        // ===== Factory Function =====

        // One generator is linked per build, the same way one platform backend is, so
        // this definition and the x86 one are never present together.
        // Constructed on first call, not at load time. The entry point runs from a
        // module initializer, and a generator defined at file scope is only built by
        // an initializer of its own: whichever the linker ordered first wins, and
        // calling through the object before its constructor has run reads a null
        // vtable pointer. The Linux build survived that by the order its objects
        // happened to be listed in.
        WrapperGeneratorBase* GetWrapperGenerator() {
            static WrapperGenerator_x86_64 generator;
            return &generator;
        }

    } // namespace Wrappers
} // namespace KotorPatcher
