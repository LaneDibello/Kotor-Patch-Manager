// Video Playback Fix for KOTOR II (Mac).
//
// In ASL::PlayBinkMovieGL, Aspyr activates an SDL event filter before initializing
// target_perf_counter on the stack. The main thread concurrently pumps SDL events,
// comparing incoming events against uninitialized stack memory. If that memory is
// in the past or zero, any residual or transition event immediately sets skipped = 1,
// prematurely cutting off cutscenes (especially chained sequences like Ebon Hawk
// travel and hyperspace).
//
// K2FixMovieRaceCondition runs right before SDL_SetEventFilter is called. It:
// 1. Flushes any stale mouse-up, key-up, or finger-up events left over from
//    clicking to travel or from the prior movie in a chained sequence.
// 2. Pre-initializes target_perf_counter with a safe lockout duration before
//    the event filter becomes active on the UI thread, eliminating the race condition.

#include <cstddef>
#include <cstdint>
#include <cstring>

#if !defined(_WIN32)
// create-patch finds the exports by this spelling.
#define __cdecl
#endif

namespace {

// SDL functions in KOTOR2sub
constexpr std::uintptr_t kSDL_GetPerformanceCounter   = 0x100014BD0;
constexpr std::uintptr_t kSDL_GetPerformanceFrequency = 0x100014C60;
constexpr std::uintptr_t kSDL_FlushEvent              = 0x100006800;

inline std::uint64_t GetPerformanceCounter()
{
    return reinterpret_cast<std::uint64_t (*)()>(kSDL_GetPerformanceCounter)();
}

inline std::uint64_t GetPerformanceFrequency()
{
    return reinterpret_cast<std::uint64_t (*)()>(kSDL_GetPerformanceFrequency)();
}

inline void FlushEvent(std::uint32_t type)
{
    reinterpret_cast<void (*)(std::uint32_t)>(kSDL_FlushEvent)(type);
}

// In ASL::PlayBinkMovieGL, EventUserData layout:
// offset 0x00: bool skipped (uint8_t)
// offset 0x01: bool quit (uint8_t)
// offset 0x20: uint64_t current_perf_counter
// offset 0x28: uint64_t target_perf_counter
constexpr std::size_t kTargetTimeOffset = 0x28;

// Default fallback lockout duration in milliseconds if none provided
constexpr std::uint32_t kDefaultLockoutMs = 500;

} // namespace

extern "C" void __cdecl K2FixMovieRaceCondition(unsigned char* eventUserData, std::uint32_t lockoutMs)
{
    if (eventUserData == nullptr) {
        return;
    }

    // 1. Flush any stale input events queued before this movie started.
    // In chained movie sequences (takeoff -> hyperspace -> landing), this ensures
    // that mouse or key releases from prior interactions never leak into the new movie.
    FlushEvent(0x301); // SDL_KEYUP
    FlushEvent(0x402); // SDL_MOUSEBUTTONUP
    FlushEvent(0x701); // SDL_FINGERUP

    // 2. Pre-initialize target_perf_counter BEFORE SDL_SetEventFilter is activated.
    // This guarantees that the concurrent UI thread never evaluates uninitialized
    // stack memory during the movie initialization window.
    const std::uint64_t now = GetPerformanceCounter();
    const std::uint64_t freq = GetPerformanceFrequency();

    // Use the lockout duration passed from StartMovie (defaults to 500ms)
    const std::uint32_t effectiveLockout = (lockoutMs == 0) ? kDefaultLockoutMs : lockoutMs;
    const std::uint64_t ticks = (freq / 1000) * effectiveLockout;
    const std::uint64_t targetTime = now + ticks;

    std::memcpy(eventUserData + kTargetTimeOffset, &targetTime, sizeof(targetTime));
}
