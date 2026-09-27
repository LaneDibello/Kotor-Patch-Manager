#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
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

        // Reads decimal digits, or hex digits after "0x", as an unsigned value. Every number a
        // parameter source carries goes through this, so a constant and an offset are written
        // the same way.
        //
        // The base is chosen here rather than left to strtoull's prefix detection, which would
        // read a leading zero as octal and make "010" mean eight. strtoull also skips leading
        // whitespace, takes a sign and wraps a negative, and in base 16 takes a second "0x", so
        // every character is checked before it sees the text. All it can still refuse is a
        // value too large to hold.
        inline bool ParseDigits(std::string text, uint64_t& outValue) {
            int base = 10;
            if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
                text = text.substr(2);
                base = 16;
            }

            const bool digitsOnly = !text.empty() &&
                std::all_of(text.begin(), text.end(), [base](char c) {
                    const auto u = static_cast<unsigned char>(c);
                    return base == 16 ? std::isxdigit(u) != 0 : std::isdigit(u) != 0;
                });
            if (!digitsOnly) {
                return false;
            }

            try {
                outValue = std::stoull(text, nullptr, base);
                return true;
            } catch (const std::out_of_range&) {
                return false;
            }
        }

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

            return ParseDigits(source.substr(prefixLength), outValue)
                ? ConstantSource::Parsed
                : ConstantSource::Malformed;
        }

        // The largest offset either way. Arbitrary, but the generators add the distance from
        // their anchor to the game's stack, under 0x1000 bytes, so it has to leave that much
        // room below the int limit or the sum wraps into a slot on the other side. No real
        // frame comes near it.
        constexpr uint64_t kMaxOffsetMagnitude = 0x7FFF0000;

        // Reads the offset in "esp+8" or "[esi-0x10]": a sign, then digits as a constant's are
        // written, no further than kMaxOffsetMagnitude. The sign is required because every
        // caller splits the text at it, so text without one is not an offset.
        inline bool ParseSignedOffset(const std::string& text, int& outOffset) {
            if (text.empty() || (text[0] != '+' && text[0] != '-')) {
                return false;
            }

            uint64_t magnitude = 0;
            if (!ParseDigits(text.substr(1), magnitude) || magnitude > kMaxOffsetMagnitude) {
                return false;
            }

            const int value = static_cast<int>(magnitude);
            outOffset = text[0] == '-' ? -value : value;
            return true;
        }

        // Splits "esi+0x10" into esi and 16, and "eax" into eax and zero. False when the name
        // is missing or the offset does not parse. The name is not checked against any
        // architecture's table, that being the generator's business.
        inline bool SplitRegisterOffset(const std::string& text, std::string& outName,
                                        int& outOffset) {
            const std::size_t sign = text.find_first_of("+-");
            outName = text.substr(0, sign);
            outOffset = 0;
            if (outName.empty()) {
                return false;
            }

            // Hex lets a hook write an offset the way the disassembly shows it, "esi+0x10" as
            // readily as "esi+16".
            return sign == std::string::npos || ParseSignedOffset(text.substr(sign), outOffset);
        }

        // The outcomes a constant has, for the same reason: a source with no brackets is some
        // other form, where brackets the caller could not read is an error rather than an
        // invitation to keep looking.
        enum class Dereference { None, Malformed, Parsed };

        // Reads "[esi+0x10]" as esi and 16, "[eax]" as eax and zero.
        inline Dereference ParseDereference(const std::string& source, std::string& outName,
                                            int& outOffset) {
            // "[]" has nothing between the brackets.
            if (source.size() <= 2 || source.front() != '[' || source.back() != ']') {
                return Dereference::None;
            }

            return SplitRegisterOffset(source.substr(1, source.size() - 2), outName, outOffset)
                ? Dereference::Parsed
                : Dereference::Malformed;
        }

        // The same outcomes again. A source with no sign is some other form, a bare register
        // name among them.
        enum class RegisterAddress { None, Malformed, Parsed };

        // Reads "esi+0x10" as esi and 16: the address that far from the register's value,
        // which is how a hook asks for a field's address rather than what the field holds.
        // "esp-8" is the same form, the stack pointer being a register like the rest. Called
        // once the constant and bracketed forms are ruled out, since both can carry a sign.
        inline RegisterAddress ParseRegisterAddress(const std::string& source,
                                                    std::string& outName, int& outOffset) {
            if (source.find_first_of("+-") == std::string::npos) {
                return RegisterAddress::None;
            }

            return SplitRegisterOffset(source, outName, outOffset)
                ? RegisterAddress::Parsed
                : RegisterAddress::Malformed;
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
