// Makes ShowWindow show and minimise, so alt-tab out of fullscreen behaves.
//
// The engine asks for exactly what it should: WinMessageHandler calls ShowWindow(hwnd, SW_MINIMIZE) when
// the window loses the display, and CAurInternalGL::Initialize calls it with SW_SHOWNORMAL once the
// renderer is ready. ShowWindow_Win32 honours neither. It tests nCmdShow against a mask of the commands it
// considers "bring this window forward" and calls SDL_RaiseWindow for them, which only reorders a window
// that is already mapped. SW_MINIMIZE is not in that mask at all, so alt-tabbing away from a fullscreen
// window did nothing.
//
// The mask below is the shim's own, so "show" keeps one meaning. Only the two commands the engine issues
// are handled, and the original body still runs afterwards and does its raise.
//
// Windowed mode is unaffected: Win32 only minimises on deactivation when the window is fullscreen, and
// that is the only case the engine issues SW_MINIMIZE in.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include <cstddef>

namespace {

    // Identified by structure rather than by symbol, because this build is stripped. SDL_ShowWindow
    // early-returns when SDL_WINDOW_SHOWN is already set, so it is safe to call on a window already up.
    typedef void (*WindowActionFn)(void*);
    const WindowActionFn SdlShowWindow = reinterpret_cast<WindowActionFn>(0x0869C6A0);
    const WindowActionFn SdlMinimizeWindow = reinterpret_cast<WindowActionFn>(0x0869C910);

}

extern "C" void __cdecl ShowWindowHonoursCommand(unsigned* args)
{
    const std::size_t kHwnd = 1;     // [esp+4]
    const std::size_t kCommand = 2;  // [esp+8], nCmdShow
    const unsigned kShowCommands = 0x62E;
    const unsigned kCommandLimit = 0xC;
    const unsigned kMinimize = 6;    // SW_MINIMIZE

    unsigned char* const hwnd = reinterpret_cast<unsigned char*>(args[kHwnd]);
    if (hwnd == 0) {
        return;
    }
    void* const sdlWindow = *reinterpret_cast<void**>(hwnd);
    if (sdlWindow == 0) {
        return;
    }

    const unsigned command = args[kCommand];
    if (command == kMinimize) {
        SdlMinimizeWindow(sdlWindow);
    } else if (command < kCommandLimit && ((kShowCommands >> command) & 1) != 0) {
        SdlShowWindow(sdlWindow);
    }
}
