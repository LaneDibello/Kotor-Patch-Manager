// Frame Limiter
//
// The only frame cap KOTOR offers is V-Sync, and V-Sync hands the waiting to the driver.
// Modern drivers spin while they wait, so you end up with a capped frame rate and a
// pinned CPU core at the same time.
//
// There is a better cap sitting in WinMain already: a spin at 0x00404990 that waits out
// the frame, with a Sleep in front of it. Neither has ever run, because the globals that
// switch them on are never written. Switch them on and they work fine.
//
// The catch is the Sleep takes a fixed number of milliseconds, and how long a frame needs
// to wait is not fixed. Too small and the spin burns the rest. Too large and a slow frame
// sleeps clean past its deadline, which the spin cannot take back.
//
// So: switch their limiter on, and hand it the right number every frame.

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/GameVersion.h"

namespace {

// The last millisecond goes to their spin instead of to Sleep. Sleep can come back a
// timer tick late, and this is that tick, so a bigger margin only spins longer.
const LONGLONG kSpinMarginUs = 1000;

// WinMain's limiter, looked up in the version database. The target switches it on, the
// gate lets its Sleep run, the nap is what that Sleep gets, and the frame start is where
// its deadline counts from.
float* g_target = nullptr;
int* g_gate = nullptr;
DWORD* g_napMs = nullptr;
LONGLONG* g_frameStart = nullptr;

// The game's own clock, the one that stamps the frame start. We could work the same
// number out ourselves, but only by assuming which branch the timer takes. Calling it
// cannot be wrong.
typedef LONGLONG (__thiscall *GetHighResolutionTimerFn)(void*);
GetHighResolutionTimerFn g_readTimer = nullptr;
void* g_timers = nullptr;

const int kDefaultFps = 60;

// Below 20 is more likely a typo than a choice, and above 300 the wait is shorter than
// Sleep's own jitter. Anything outside turns the limiter off, so Limit=0 means off.
const int kMinFps = 20;
const int kMaxFps = 300;

// Its own file, not swkotor.ini, which the game rewrites whenever the options screen is
// used. Arrays because the game's string type takes a mutable char*.
char g_iniFile[] = "FrameLimiter.ini";
char g_section[] = "Frame Limiter";
char g_key[] = "Limit";

LONGLONG g_targetUs = 0;   // microseconds per frame; 0 means off
bool g_initialised = false;

// Everything the patch touches in the game, out of the version database. All of it or
// none, because switching the limiter on without being able to feed it would leave the
// spin doing the whole wait.
//
// The timer is safe to call from the hook: WinMain calls it a few instructions later, in
// the very block this feeds.
bool ResolveGame() {
    try {
        g_target = (float*)GameVersion::GetGlobalPointer("FRAME_LIMIT_TARGET_FPS");
        g_gate = (int*)GameVersion::GetGlobalPointer("FRAME_LIMIT_SLEEP_ENABLED");
        g_napMs = (DWORD*)GameVersion::GetGlobalPointer("FRAME_LIMIT_SLEEP_MS");
        g_frameStart = (LONGLONG*)GameVersion::GetGlobalPointer("FRAME_LIMIT_FRAME_START");
        g_readTimer = (GetHighResolutionTimerFn)GameVersion::GetFunctionAddress(
            "CExoTimers", "GetHighResolutionTimer");

        void** exoBase = (void**)GameVersion::GetGlobalPointer("EXO_BASE_PTR");
        int timersOffset = GameVersion::GetOffset("CExoBase", "timers");
        if (exoBase != nullptr && *exoBase != nullptr && timersOffset >= 0) {
            g_timers = *(void**)((char*)*exoBase + timersOffset);
        }
    }
    catch (const GameVersionException&) {
        return false;
    }

    return g_target != nullptr && g_gate != nullptr && g_napMs != nullptr && g_frameStart != nullptr
        && g_readTimer != nullptr && g_timers != nullptr;
}

// The game's ini reader calls fopen, which resolves a bare name against the working
// directory rather than the install, so the settings file gets named absolutely.
bool PathBesideExe(const char* name, char* out, DWORD size) {
    DWORD length = GetModuleFileNameA(nullptr, out, size);
    if (length == 0 || length >= size) {
        return false;
    }
    char* separator = strrchr(out, '\\');
    if (separator == nullptr) {
        return false;
    }
    if ((size_t)(separator + 1 - out) + strlen(name) + 1 > size) {
        return false;
    }
    strcpy(separator + 1, name);
    return true;
}

// Read with the game's own ini parser. Falls back to the default if the key is missing,
// or if the version database never resolved and the read could not happen at all.
int ReadConfiguredFps(bool* found) {
    char path[MAX_PATH];
    if (!PathBesideExe(g_iniFile, path, MAX_PATH)) {
        *found = false;
        return kDefaultFps;
    }

    CExoIni ini;
    CExoString value;
    CExoString filename(path);
    CExoString category(g_section);
    CExoString key(g_key);
    if (ini.ReadIniEntry(&value, &filename, &category, &key) == 0) {
        *found = false;
        return kDefaultFps;
    }

    char* text = value.GetCStr();
    *found = true;
    return text != nullptr ? atoi(text) : kDefaultFps;
}

// Leaves the setting on disk on first run so there is something to edit.
void WriteConfiguredFps(int fps) {
    char path[MAX_PATH];
    if (!PathBesideExe(g_iniFile, path, MAX_PATH)) {
        return;
    }
    char text[16];
    sprintf(text, "%d", fps);

    CExoIni ini;
    CExoString value(text);
    CExoString filename(path);
    CExoString category(g_section);
    CExoString key(g_key);
    ini.WriteIniEntry(&value, &filename, &category, &key);
}

// Without this, Sleep rounds up to the scheduler tick of about 15.6ms, which is most of a
// frame at 60. The game never asks for anything finer, so their Sleep(1) would have been
// a whole frame on its own.
//
// winmm is loaded here rather than linked, since the patch build links a fixed set of
// libraries, and here rather than in DllMain, since LoadLibrary must not run under the
// loader lock. It is left loaded on purpose: the finer resolution only lasts as long as
// the module does.
void RaiseTimerResolution() {
    HMODULE winmm = LoadLibraryA("winmm.dll");
    if (winmm == nullptr) {
        return;
    }
    typedef UINT (WINAPI *TimeBeginPeriodFn)(UINT);
    TimeBeginPeriodFn timeBeginPeriod =
        (TimeBeginPeriodFn)GetProcAddress(winmm, "timeBeginPeriod");
    if (timeBeginPeriod != nullptr) {
        timeBeginPeriod(1);
    }
}

void Initialise() {
    g_initialised = true;

    if (!ResolveGame()) {
        OutputDebugStringA("[FrameLimiter] could not resolve the game's frame limiter\n");
        return;
    }

    bool found = false;
    int fps = ReadConfiguredFps(&found);
    if (!found) {
        WriteConfiguredFps(kDefaultFps);
    }
    if (fps < kMinFps || fps > kMaxFps) {
        return;
    }
    g_targetUs = 1000000 / fps;

    // Arms their limiter, and opens the gate on the Sleep in front of its spin. Both are
    // plain data the retail build never writes, so nothing puts them back.
    *g_target = (float)fps;
    *g_gate = 1;
    RaiseTimerResolution();
}

// How long this frame can afford to sleep, in milliseconds. Zero leaves the whole wait
// to their spin.
DWORD NapFor(LONGLONG elapsedUs) {
    LONGLONG remaining = g_targetUs - elapsedUs - kSpinMarginUs;
    return remaining > 0 ? (DWORD)(remaining / 1000) : 0;
}

}  // namespace

// Called once per frame, after the buffers are swapped and before the game's own limiter
// reads the nap.
extern "C" void __cdecl PaceFrame() {
    if (!g_initialised) {
        Initialise();
    }
    if (g_targetUs == 0) {
        return;
    }

    // Zero until their block has stamped a frame start, which makes the first elapsed the
    // whole uptime. Leaving the nap at zero gives their spin that frame's wait.
    LONGLONG elapsedUs = g_readTimer(g_timers) - *g_frameStart;
    *g_napMs = (elapsedUs >= 0 && elapsedUs <= 1000000) ? NapFor(elapsedUs) : 0;
}

// Not fatal: without the version database the limiter runs on the default rather than
// not at all.
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance;
    (void)reserved;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            OutputDebugStringA("[FrameLimiter] GameVersion::Initialize() failed\n");
        }
        break;
    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    default:
        break;
    }
    return TRUE;
}
