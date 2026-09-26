#pragma once
#include <cstdint>
#include "wrapper_base.h"
#include "emitter.h"

// x86 32-bit wrapper generator.
// Emits runtime code to save/restore CPU state and call patch functions. The
// bytes are identical on Windows and Linux; only executable-memory allocation
// and cache flushing differ, and those go through the platform seam.

namespace KotorPatcher {
    namespace Wrappers {

        class WrapperGenerator_x86 : public WrapperGeneratorBase {
        public:
            WrapperGenerator_x86();
            ~WrapperGenerator_x86() override;

            // Generate wrapper stub for given configuration
            void* GenerateWrapper(const WrapperConfig& config) override;

            // Free all allocated wrapper memory
            void FreeAllWrappers() override;

            // Platform identifier
            const char* GetPlatformName() const override {
                return "x86";
            }

        private:
            // Track allocated wrapper stubs for cleanup
            struct AllocatedWrapper {
                void* address;
                size_t size;
            };
            std::vector<AllocatedWrapper> m_allocatedWrappers;

            // Allocate executable memory for wrapper code
            void* AllocateExecutableMemory(size_t size, uintptr_t nearAddress);

            // Generate DETOUR type wrapper (save state, call patch, restore state, execute stolen bytes)
            void* GenerateDetourWrapper(const WrapperConfig& config);

            // Helper: Emit x86 machine code bytes
            void EmitBytes(Emitter& code, const uint8_t* bytes, size_t count);
            void EmitByte(Emitter& code, uint8_t value);
            void EmitDword(Emitter& code, uint32_t value);

            // Helper: Emit FXSAVE or FXRSTOR through the reserved area above the
            // saved state, which the caller reaches from EBX
            void EmitFpStateAccess(Emitter& code, int savedStateSize, bool restore);

            // Helper: Calculate relative offset for JMP/CALL
            uint32_t CalculateRelativeOffset(void* from, void* to);

            // Helper: Extract parameter from source and push onto stack
            // False when the source is one this generator cannot read. The caller has
            // to abandon the wrapper on that: a parameter that is not pushed shifts
            // every later argument down a slot rather than merely going missing.
            bool ExtractAndPushParameter(Emitter& code, const ParameterInfo& param, int savedStateSize);
        };

    } // namespace Wrappers
} // namespace KotorPatcher
