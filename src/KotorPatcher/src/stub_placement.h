#pragma once
#include <cstddef>
#include <cstdint>

// Where to put a generated stub so the game can jump to it.
//
// A hook site is redirected with a 5-byte relative jump, which reaches a signed
// 32-bit displacement. On the 32-bit targets that is the whole address space, so a
// stub can go anywhere. On the macOS x86_64 target the game image sits at
// 0x100000000 while the allocator is free to answer from anywhere in a 47-bit
// space, so a stub has to be placed deliberately.
//
// This is policy shared by both platform backends, which differ only in how they
// ask the OS for a specific address. It is a conservative approximation:
// Trampoline::ComputeRel32 remains the authority on whether a jump reaches.

namespace KotorPatcher {
namespace StubPlacement {

    // Search a little inside the true rel32 limit so the far end of the block is
    // reachable too, and so the jump back out of it still fits.
    constexpr std::uintptr_t kRel32Reach = 0x7FF00000;

    // Every 32-bit address is within a rel32 of every other, because the
    // displacement wraps in the same width the CPU computes in. Searching there
    // could only fail where a plain allocation would have succeeded.
    constexpr bool kEveryAddressReaches = sizeof(void*) == 4;

    // Candidates are walked outward from the hint in both directions. A megabyte
    // covers the whole window in a bounded number of tries, and the space just past
    // a game image is normally free, so the first or second try lands.
    constexpr std::uintptr_t kSearchStep = 0x100000;
    constexpr std::size_t kSearchAttempts = 2 * (kRel32Reach / kSearchStep);

    // Whether a block of `size` bytes at `candidate` is close enough to `nearAddress`
    // for a relative jump in either direction.
    inline bool NearEnough(std::uintptr_t nearAddress, std::uintptr_t candidate,
                           std::size_t size) {
        std::uintptr_t lo = candidate < nearAddress ? candidate : nearAddress;
        std::uintptr_t hi = candidate < nearAddress ? nearAddress : candidate + size;
        return hi - lo <= kRel32Reach;
    }

    // Address to try on `attempt`, alternating above and below `nearAddress`.
    // Returns false once the walk would leave the window or run off the bottom of
    // the address space, which ends the search.
    inline bool NextCandidate(std::uintptr_t nearAddress, std::size_t attempt,
                              std::uintptr_t& outHint) {
        std::uintptr_t offset = (attempt / 2 + 1) * kSearchStep;
        if (offset > kRel32Reach) return false;

        if (attempt % 2 == 0) {
            outHint = nearAddress + offset;
        } else {
            if (offset > nearAddress) return false;
            outHint = nearAddress - offset;
        }
        return true;
    }

} // namespace StubPlacement
} // namespace KotorPatcher
