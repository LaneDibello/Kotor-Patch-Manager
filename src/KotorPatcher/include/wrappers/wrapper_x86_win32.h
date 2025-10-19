#pragma once
#include "wrapper_base.h"

// x86 32-bit Windows wrapper generator
// Generates runtime code to save/restore CPU state and call patch functions

namespace KotorPatcher {
    namespace Wrappers {

        class WrapperGenerator_x86_Win32 : public WrapperGeneratorBase {
        public:
            WrapperGenerator_x86_Win32();
            ~WrapperGenerator_x86_Win32() override;

            // Generate wrapper stub for given configuration
            void* GenerateWrapper(const WrapperConfig& config) override;

            // Free all allocated wrapper memory
            void FreeAllWrappers() override;

            // Platform identifier
            const char* GetPlatformName() const override {
                return "x86_Win32";
            }

        private:
            // Track allocated wrapper stubs for cleanup
            struct AllocatedWrapper {
                void* address;
                size_t size;
            };
            std::vector<AllocatedWrapper> m_allocatedWrappers;

            // Allocate executable memory for wrapper code
            void* AllocateExecutableMemory(size_t size);

            // Generate DETOUR type wrapper (save state, call patch, restore state, execute stolen bytes)
            void* GenerateDetourWrapper(const WrapperConfig& config);

            // Helper: Emit x86 machine code bytes
            void EmitBytes(BYTE*& code, const BYTE* bytes, size_t count);
            void EmitByte(BYTE*& code, BYTE value);
            void EmitDword(BYTE*& code, DWORD value);

            // Helper: Calculate relative offset for JMP/CALL
            DWORD CalculateRelativeOffset(void* from, void* to);

            // Helper: Extract parameter from source and push onto stack
            void ExtractAndPushParameter(BYTE*& code, const ParameterInfo& param, int savedStateOffset, int pushcCunt);
        };

    } // namespace Wrappers
} // namespace KotorPatcher
