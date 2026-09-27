// Connects Alt+Enter to the fullscreen toggle the engine already has, and makes the result persist.
//
// HandleAltReturn (0x0819C63A) asks the renderer whether it is currently fullscreen and posts the opposite
// as a mode-change request, which WinMain's loop picks up and applies. WinMain turns the value straight
// into the fullscreen selector, `cmp eax,2; sete al` at 0x08199808.
//
// Nothing calls it. Its address appears nowhere in the binary and there is no direct call, so it is
// stranded the same way SpawnBlackScreen is. Aspyr replaced the keyboard path with SDL translation, and
// ASL__TranslateSdlEventToWin32Message never produces a WM_SYSKEYDOWN, so whatever window-proc case used
// to reach it is gone. Every Win32 system key combination is unreachable on this port, not just this one.
//
// The translator does notice Alt+Enter on key-up and sets bit 3 of the window's dwFlags, but that path is
// unusable: HWND_Mac's constructor zeroes dwFlags and the only bit anything sets is bit 1, the unicode
// flag, so the gate on bit 2 is never armed, and nothing reads bit 3 either. Using it would mean arming a
// bit, adding a consumer, and toggling on key release.
//
// So the keystroke is taken from the shim's event pump instead, and the decision about which way to go
// stays with HandleAltReturn.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include "WindowingShim.h"

#include <cstddef>

namespace {

    // Asked to toggle, it queries the renderer for the current state and posts the opposite, so the caller
    // does not have to track which way round things are.
    typedef void (*AltReturnFn)(int, int);
    const AltReturnFn HandleAltReturn = reinterpret_cast<AltReturnFn>(0x0819C63A);

}

// The pump's SDL_Event, at the point SDL_PeepEvents has returned one and before it is dispatched.
extern "C" void __cdecl ToggleFullscreenOnAltEnter(unsigned char* event)
{
    // SDL_KeyboardEvent, whose keysym starts at +0x10.
    const std::size_t kType = 0x00;
    const std::size_t kRepeat = 0x0D;
    const std::size_t kSym = 0x14;
    const std::size_t kMod = 0x18;

    const unsigned kSdlKeyDown = 0x300;
    const unsigned kSdlFirstEvent = 0;
    const int kSdlkReturn = 13;
    const int kSdlkKeypadEnter = 0x40000058;   // SDL_SCANCODE_KP_ENTER | SDLK_SCANCODE_MASK
    const unsigned short kModAlt = 0x0300;     // KMOD_LALT | KMOD_RALT

    if (*reinterpret_cast<unsigned*>(event + kType) != kSdlKeyDown) {
        return;
    }
    // Auto-repeat would otherwise toggle once per repeat for as long as the key is held.
    if (event[kRepeat] != 0) {
        return;
    }
    if ((*reinterpret_cast<unsigned short*>(event + kMod) & kModAlt) == 0) {
        return;
    }

    const int sym = *reinterpret_cast<int*>(event + kSym);
    if (sym != kSdlkReturn && sym != kSdlkKeypadEnter) {
        return;
    }

    // Anything other than 1 for the first argument means "work out which way to go".
    HandleAltReturn(0, 0);

    // Persist what it decided. CClientExoAppInternal::StartServices is the only thing that writes
    // FullScreen, once at startup from the value it just read, so without this the toggle lasts exactly as
    // long as the session. The request it posted says which way it went.
    const int pending = *Windowing::gVideoModeChangePending;
    if (pending == 1 || pending == 2) {
        Windowing::WriteGraphicsOption("FullScreen", pending == 2 ? 1 : 0);
    }

    // Consume the event so it does not also reach the dispatcher as a plain Return and activate whatever
    // the UI has focused. Win32 would not have delivered a system key combination to the game either.
    *reinterpret_cast<unsigned*>(event + kType) = kSdlFirstEvent;
}
