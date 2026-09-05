#pragma once
#include <cstdint>
#include "wrapper_base.h"

// x86_64 wrapper generator, for the macOS build of the games.
//
// This is not a widened copy of the x86 generator. PUSHAD and POPAD, which that one
// is built around, are invalid opcodes in long mode, and the System V AMD64 calling
// convention passes arguments in registers rather than on the stack. The two share
// only the shape: save the game's state, call the patch function, restore, run the
// stolen bytes, jump back.
//
// KNOWN LIMITATION, and it is the patch author's to respect: stolen bytes are copied
// into the stub and executed there. x86_64 code can address data relative to RIP, and
// such an instruction means something different once it has moved. The engine cannot
// tell, because finding instruction boundaries needs a length disassembler it does not
// have, so a DETOUR hook whose cut bytes contain a RIP-relative reference will read or
// write the wrong address. Cut somewhere else.

namespace KotorPatcher {
    namespace Wrappers {

        class WrapperGenerator_x86_64 : public WrapperGeneratorBase {
        public:
            ~WrapperGenerator_x86_64() override;

            void* GenerateWrapper(const WrapperConfig& config) override;
            void FreeAllWrappers() override;

            const char* GetPlatformName() const override {
                return "x86_64";
            }

        private:
            struct AllocatedWrapper {
                void* address;
                std::size_t size;
            };
            std::vector<AllocatedWrapper> m_allocatedWrappers;

            void* GenerateDetourWrapper(const WrapperConfig& config);
        };

    } // namespace Wrappers
} // namespace KotorPatcher
