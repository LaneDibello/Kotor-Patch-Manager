#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "platform.h"
#include "wrapper_context.h"

// Abstract interface for platform-specific wrapper generators
// Allows supporting multiple platforms/architectures

namespace KotorPatcher {
    // Forward declare ParameterInfo to avoid circular dependency
    struct ParameterInfo;

    namespace Wrappers {

        struct WrapperConfig {

            void* patchFunction;
            uintptr_t hookAddress;

            // Original bytes that were overwritten by the hook (for DETOUR type)
            // These will be executed in the wrapper before returning to original code
            // Must be >= 5 bytes and align with instruction boundaries
            std::vector<uint8_t> originalBytes;

            // Hook type determines wrapper behavior
            enum class HookType {
                DETOUR      // Save state, call patch, restore state, execute stolen bytes, continue original
            };
            HookType type = HookType::DETOUR;

            // State preservation options
            bool preserveRegisters = true;
            bool preserveFlags = true;

            // Registers to exclude from restoration
            // Allows patch to modify specific registers
            std::vector<std::string> excludeFromRestore;

            // Parameters to extract and pass to hook function (for DETOUR hooks)
            std::vector<ParameterInfo> parameters;

            // Skip executing original bytes after patch function returns
            // Set to true when you want to fully replace the original behavior
            // instead of augmenting it
            bool skipOriginalBytes = false;

            // When non-zero, the wrapper emits a conditional jump after the
            // register-restore epilogue: if the handler returned non-zero in
            // EAX, control transfers to this address; otherwise the wrapper
            // falls through to the existing original-bytes / skipOriginalBytes
            // path. Caller must add "eax" to excludeFromRestore so the
            // handler's return value reaches the test. Default 0 = disabled.
            uintptr_t consumedExitAddress = 0;

            // Original function pointer (future use)
            void* originalFunction = nullptr;

            // Helper: Check if a register should be restored
            bool ShouldRestoreRegister(const std::string& regName) const {
                if (!preserveRegisters) return false;

                for (const auto& excluded : excludeFromRestore) {
                    if (StrICmp(excluded.c_str(), regName.c_str()) == 0) {
                        return false;
                    }
                }
                return true;
            }
        };

        // What a parameter source of the form "const:<value>" turned out to be.
        enum class ConstantSource {
            // Not a constant, so the caller goes on to the register and stack forms.
            None,
            // "const:" followed by something that is not a number. Distinct from None
            // because it must not fall through to be looked up as a register name.
            Malformed,
            Parsed
        };

        // Reads the literal out of a "const:<value>" parameter source. Unsigned, in decimal
        // or with a 0x prefix.
        //
        // A constant lets one exported patch function serve several game builds that differ
        // only by an offset or a count, instead of an exported function per build.
        //
        // The value is not range-checked here, because what fits depends on the target as
        // well as the declared type. Each caller checks against its own table.
        inline ConstantSource ParseConstantSource(const std::string& source, uint64_t& outValue) {
            static const char kPrefix[] = "const:";
            const std::size_t prefixLength = sizeof(kPrefix) - 1;

            // Only a source too short to hold the prefix can be something else entirely.
            // "const:" with nothing after it is a constant missing its value, and saying so
            // beats going on to look for a register by that name.
            if (source.size() < prefixLength ||
                source.compare(0, prefixLength, kPrefix) != 0) {
                return ConstantSource::None;
            }

            std::string text = source.substr(prefixLength);

            // stoull accepts a sign and wraps a negative into a huge unsigned value, and it
            // skips leading whitespace, so both are turned away before it sees them.
            if (text.empty() || text[0] < '0' || text[0] > '9') {
                return ConstantSource::Malformed;
            }

            // Base is chosen rather than left to strtoull's prefix detection, which would
            // read a leading zero as octal and make "const:010" mean eight.
            int base = 10;
            if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
                base = 16;
                text = text.substr(2);
            }

            try {
                std::size_t consumed = 0;
                outValue = std::stoull(text, &consumed, base);
                if (consumed != text.size()) {
                    return ConstantSource::Malformed;
                }
            } catch (...) {
                return ConstantSource::Malformed;
            }

            return ConstantSource::Parsed;
        }

        // The outcomes a constant has, for the same reason: a source with no brackets is some
        // other form, where brackets the caller could not read is an error rather than an
        // invitation to keep looking.
        enum class Dereference { None, Malformed, Parsed };

        // Reads "[esi+0x10]" as esi and 16, "[eax]" as eax and zero. The register name is not
        // checked against any architecture's table, that being the generator's business.
        inline Dereference ParseDereference(const std::string& source, std::string& outName,
                                            int& outOffset) {
            // "[]" has nothing between the brackets.
            if (source.size() <= 2 || source.front() != '[' || source.back() != ']') {
                return Dereference::None;
            }

            const std::string inner = source.substr(1, source.size() - 2);
            const std::size_t sign = inner.find_first_of("+-");
            outOffset = 0;

            if (sign == std::string::npos) {
                outName = inner;
                return Dereference::Parsed;
            }

            outName = inner.substr(0, sign);
            if (outName.empty()) {
                return Dereference::Malformed;
            }

            // stoi reads the sign, so the offset keeps it. Base 0 lets a hook write a field
            // offset the way the disassembly shows it, "[esi+0x10]" as readily as "[esi+16]".
            const std::string offset = inner.substr(sign);
            try {
                std::size_t consumed = 0;
                outOffset = std::stoi(offset, &consumed, 0);
                if (consumed != offset.size()) {
                    return Dereference::Malformed;
                }
            } catch (...) {
                return Dereference::Malformed;
            }

            return Dereference::Parsed;
        }

        // Abstract base class for wrapper generators
        class WrapperGeneratorBase {
        public:
            virtual ~WrapperGeneratorBase() = default;

            // Generate a wrapper stub and return its address
            // Returns nullptr on failure
            virtual void* GenerateWrapper(const WrapperConfig& config) = 0;

            // Free all allocated wrappers
            virtual void FreeAllWrappers() = 0;

            // Get platform name for debugging
            virtual const char* GetPlatformName() const = 0;
        };

        // Factory function to get the appropriate wrapper generator for current platform
        WrapperGeneratorBase* GetWrapperGenerator();

    } // namespace Wrappers
} // namespace KotorPatcher
