# KotOR II Mac Video Playback Patch: Technical & Non-Technical Walkthrough

This document explains the root causes of the notorious cutscene skipping bug in the Mac port of *Star Wars: Knights of the Old Republic II – The Sith Lords*, and details how the **Video Playback Patch** resolves it.

---

## Table of Contents
1. [The Problem](#part-1-the-problem)
   - [The Symptom](#the-symptom)
   - ["The Why": Non-Technical Explanation](#the-why-non-technical-explanation)
   - ["The Why": Root Causes in Assembly](#the-why-root-causes-in-assembly)
2. [The Solution](#part-2-the-solution)
   - [Non-Technical Explanation of What the Patch Does](#what-the-patch-does-non-technical-overview)
   - [Hook Breakdown & Byte Diffs](#hook-breakdown--byte-diffs)
   - [Summary of Affected Addresses & Files](#summary-of-affected-addresses--files)

---

# Part 1: The Problem

### The Symptom
When playing *KotOR II* on macOS (Steam / Aspyr port), pre-rendered movie cutscenes—most noticeably space-travel cutscenes involving the Ebon Hawk (takeoffs, hyperspace jumps, space battles, and landings)—frequently cut off after a fraction of a second, skipping straight to the loading screen or the next scene without user intention.


---

### The Why: Non-Technical Explanation

In the original Windows version of KotOR II, movies could **only** be skipped by pressing the **Escape key**. Mouse clicks were completely ignored during video playback.

When Aspyr ported the game to macOS, they redesigned the movie player so players could skip cutscenes using mouse clicks, keyboard presses, controller buttons, or trackpad gestures. To avoid having the click that *launched* a cutscene immediately skip it, they added a **250-millisecond (0.25 second) grace period**. 

This patch exists to cover 3 distinct limitations with their approach:

1. **Human Click Duration**: A natural, relaxed mouse click (press down, then release) takes roughly 200 to 350 milliseconds. Because the game only checks for the **release** of the mouse button (`Mouse Up`), if you, say, clicked a planet to travel to on the Galaxy Map and released your mouse at all slowly, you could easily exceed the limited grace period and cause the movie to cut off/skip.
2. **Mac Trackpads**: On Mac laptops and Magic Trackpads, simply lifting your finger off the glass sends a `Finger Up` gesture. Aspyr wired this gesture to the cutscene skip function. Resting your hand on the trackpad and lifting your finger or moving your mouse pointer and lifting your finger ould both cause a video to terminate and skip.
3. **The "Ghost" Race Condition**: Behind the scenes, the game runs on two CPU threads simultaneously. For a brief microsecond when a video begins, the grace period timer was left uninitialized (holding random computer memory). If macOS delivered any input during that tiny sliver of time, the video was skipped immediately before frame 1 even finished loading.

These issues, partiulraly the first two, were most often seen during space travel scenes, relatively unique for how commonly they play **2 to 3 movies back-to-back** (e.g., Planet Takeoff $\rightarrow$ Hyperspace $\rightarrow$ Planet Landing). If you clicked and held too long to skip the takeoff, or if your finger shifted between movies, the leftover release event could bleed directly into the start of the hyperspace movie, instantly killing it.

---

### The Why: Root Causes in Assembly

On Windows, BioWare handled cutscenes inside `WinMessageHandler` (`0x0049f84b`), checking solely for `WM_KEYDOWN` with `VK_ESCAPE` (`0x1b`).

On macOS, Aspyr introduced a custom OpenGL Bink player (`ASL::PlayBinkMovieGL`) and split execution across two threads:
- **Game Thread (`WinMainThread`)**: Executes game scripts (`NWScript`), scene logic, and movie playback (`CExoMoviePlayerInternal::StartMovie`).
- **Main UI Thread**: Continuously runs an event pump loop via `ASL::SDL::ProcessEvents()`, retrieving events from Cocoa and passing them to SDL2.

To handle movie skipping, `PlayBinkMovieGL` installs an SDL event filter:
```cpp
// ASL::EventUserData::EventFilter(void* userdata, SDL_Event* event)
```
The filter checks incoming SDL events and sets `EventUserData.skipped = 1` if an authorized skip event occurs after a target timestamp.

#### 1. The Trackpad Gesture Check (`SDL_FINGERUP` = 0x701)
In the 64-bit binary (`KOTOR2sub`, slice `x86_64`) at address `0x1004822D2`:
```x86asm
1004822d2: 3d 01 07 00 00  cmpl $0x701, %eax    ## SDL_FINGERUP
1004822d7: 75 03           jne  0x1004822dc     ## Jump to return 0
1004822d9: c6 03 01        movb $0x1, (%rbx)    ## data->skipped = 1
1004822dc: 31 c0           xorl %eax, %eax      ## return 0
```
When a user lifts their finger from an Apple trackpad, Cocoa's window listener triggers `-[Cocoa_WindowListener touchesEndedWithEvent:]`, calling `SDL_SendTouch(..., SDL_FINGERUP)`. The filter catches this event and terminates the movie.

#### 2. The 250ms Lockout in `StartMovie`
In `CExoMoviePlayerInternal::StartMovie` at `0x1002CE300`:
```x86asm
1002ce300: ba fa 00 00 00  movl $0xfa, %edx     ## 0xFA = 250 milliseconds
1002ce305: 48 89 c7        movq %rax, %rdi      ## filename
1002ce308: f3 0f 10 45 c8  movss -0x38(%rbp), %xmm0 ## volume
1002ce30d: e8 fc 2f 1b 00  callq ASL::PlayBinkMovieGL
```
Arg 4 (`%edx`) defines the lockout window. After `BinkOpen` opens the media stream and compiles OpenGL shaders, 250 ms is insufficient to protect against:
- Slow or held mouse releases from clicking UI buttons (`SDL_MOUSEBUTTONUP` is the only mouse event checked; `MOUSEBUTTONDOWN` is ignored).
- Chained transitions in `CExoMoviePlayerInternal::PlayMoviesAsync`, where Movie 1 ends and Movie 2 immediately starts.

#### 3. Uninitialized Stack Memory Race Condition
In `ASL::PlayBinkMovieGL` at `0x100481C0A`:
```x86asm
100481c03: leaq 0x648(%rip), %rdi    ## &EventUserData::EventFilter
100481c0a: leaq -0x460(%rbp), %rsi   ## &EventUserData on stack
100481c11: callq _SDL_SetEventFilter ## <-- FILTER ACTIVATES HERE
100481c16: callq _SDL_GetPerformanceCounter
...
100481c29: callq _SDL_GetPerformanceFrequency
...
100481c49: addq  -0x448(%rbp), %rax
100481c50: movq  %rax, -0x438(%rbp)  ## <-- target_perf_counter WRITTEN HERE
```
`_SDL_SetEventFilter` is called at `100481c11`, enabling the filter on the UI thread immediately. However, the stack slot holding `target_perf_counter` (`rbp - 0x438`, or `userdata + 0x28`) is **not written until `100481c50`**.

During this execution window, `EventFilter` evaluates incoming events by comparing `current_time` against uninitialized stack garbage:
```x86asm
100482268: cmpq 0x28(%rbx), %rax     ## Compare now with target_perf_counter
10048226c: jb   0x1004822dc          ## If now < target, ignore
```
If the uninitialized stack slot holds 0 or a past timestamp, `now < target` evaluates to false, immediately triggering a cutscene abort before the first frame even renders.


---

# Part 2: The Solution

### What the Patch Does: Non-technical Overview

This patch fixes the issue by addressing all three mechanical triggers:

| Problem | How the Patch Solves It | Result for the Player |
| :--- | :--- | :--- |
| **Trackpad Finger Lift** | Removes `Finger Up` from the list of skip triggers in the video engine. | Resting or lifting your fingers on a MacBook trackpad will never skip a cutscene. |
| **Lingering Click / Natural Release** | Extends the initial skip grace period from 250 ms to 500 ms (0.5s). | Normal clicks to travel or clicking through dialogue won't bleed into the movie, while intentional skips remain quick and responsive. |
| **Multi-Thread Race Condition** | Detours the video setup into custom C++ code that safely initializes timers and flushes stale clicks. | Videos will never abort due to uninitialized memory or leftover clicks from menus or chained cutscenes. |

---

### Hook Breakdown & Byte Diffs

#### Hook 1: Neutralize `SDL_FINGERUP`
* **Address**: `0x1004822D7`
* **Type**: `simple`
* **Byte Diff**:
  ```diff
  - 1004822d7: 75 03  (jne 0x1004822dc)
  + 1004822d7: eb 03  (jmp 0x1004822dc)
  ```
* **Explanation**: Replaces the conditional jump with an unconditional short jump. Regardless of whether the incoming event is `SDL_FINGERUP` (`0x701`), the code jumps straight to the exit branch, skipping `movb $0x1, (%rbx)`.

#### Hook 2: Extend Lockout Window in `StartMovie`
* **Address**: `0x1002CE300`
* **Type**: `simple`
* **Byte Diff**:
  ```diff
  - 1002ce300: ba fa 00 00 00  (movl $0x000000FA, %edx  ; 250 ms)
  + 1002ce300: ba f4 01 00 00  (movl $0x000001F4, %edx  ; 500 ms)
  ```
* **Explanation**: Changes the lockout duration parameter from 250 ms to 500 ms (`0x1F4`). A relaxed human mouse click takes roughly 200–350 ms from press-down to release, and clicking rapidly through dialogue that unexpectedly launches a cutscene frequently introduces an extra trailing click. Raising the grace window to 500 ms (half a second) ensures that normal clicks to interact with galaxy map buttons or dialogs do not trigger an accidental skip on button release, while intentional skips (clicking or pressing Escape) still feel immediate and responsive.

#### Hook 3: C++ Detour for Race Condition & Event Purging
* **Address**: `0x100481C0A`
* **Type**: `detour`
* **Target Function**: `K2FixMovieRaceCondition`
* **Intercepted Instruction**:
  ```x86asm
  100481c0a: 48 8d b5 a0 fb ff ff   leaq -0x460(%rbp), %rsi
  ```
* **Parameters Passed**:
  ```toml
  [[hooks.parameters]]
  source = "rbp-0x460" # Pointer to EventUserData struct
  type = "pointer"

  [[hooks.parameters]]
  source = "rbx" # Initial lockout duration in milliseconds
  type = "uint"
  ```
* **C++ Implementation ([`VideoPlaybackFix.cpp`](file:///Users/danielfolsom/Downloads/Video%20Bug/K2VideoPatch/VideoPlaybackFix.cpp))**:
  ```cpp
  extern "C" void __cdecl K2FixMovieRaceCondition(unsigned char* eventUserData, std::uint32_t lockoutMs)
  {
      if (eventUserData == nullptr) return;

      // 1. Flush stale events queued prior to movie start
      FlushEvent(0x301); // SDL_KEYUP
      FlushEvent(0x402); // SDL_MOUSEBUTTONUP
      FlushEvent(0x701); // SDL_FINGERUP

      // 2. Pre-initialize target_perf_counter BEFORE SDL_SetEventFilter is called
      const std::uint64_t now = GetPerformanceCounter();
      const std::uint64_t freq = GetPerformanceFrequency();

      // Use the lockout duration passed from StartMovie (defaults to 500ms)
      const std::uint32_t effectiveLockout = (lockoutMs == 0) ? 500 : lockoutMs;
      const std::uint64_t ticks = (freq / 1000) * effectiveLockout;
      const std::uint64_t targetTime = now + ticks;

      // offset 0x28 is target_perf_counter
      std::memcpy(eventUserData + kTargetTimeOffset, &targetTime, sizeof(targetTime));
  }
  ```

---

### Summary of Affected Addresses & Files

| Component | Target Symbol / Function | 64-Bit Address | Hook Type | File |
| :--- | :--- | :--- | :--- | :--- |
| **Trackpad Fix** | `ASL::EventUserData::EventFilter` | `0x1004822D7` | `simple` | [`hooks.toml`] |
| **Lockout Window** | `CExoMoviePlayerInternal::StartMovie` | `0x1002CE300` | `simple` | [`hooks.toml`]|
| **Race / Event Flush**| `ASL::PlayBinkMovieGL` | `0x100481C0A` | `detour` | [`hooks.toml`] / [`VideoPlaybackFix.cpp`]|
