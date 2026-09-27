// Creates each window at the size and with the border it is going to keep.
//
// Every window the game asks for passes CW_USEDEFAULT for its width and height, which in Win32 means "pick
// something sensible". CreateWindowEx_Shared answers it with ASL__Display__GetLogicalMode(0), the logical
// mode of the display, which with no fullscreen window on it is the bare desktop resolution. The window is
// then resized to whatever it should be.
//
// That resize is a visible event whenever the two sizes differ, and it happens once per window: the
// renderer is torn down and rebuilt several times while the game starts, and each replacement repeats it.
// Each teardown also leaves nothing on screen until its replacement appears, so the replacement arrives
// already correct instead of resizing into place.
//
// Correct depends on where the window is going. Fullscreen windows become the desktop, so the logical mode
// the stock code picks is already right and is left alone. Windowed ones become the configured mode, so
// they adopt that instead.
//
// Size is only half of matching. A window bound for fullscreen is still created windowed and decorated,
// because the shim hands SDL a bare SDL_WINDOW_OPENGL and never consults dwStyle, and it cannot be
// created fullscreen either: the HWND_Mac constructor applies fullscreen=0 the moment it exists, undoing
// it. Borderless closes the gap, since desktop-sized and undecorated matches the fullscreen surface that
// replaces it.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include "WindowingShim.h"

#include <cstddef>

namespace {

    // The mode the engine renders at. Holds the engine's own 800x600 default until
    // CClientExoAppInternal::ReadVideoModeSettings has run.
    const int* const gGuiResolutionWidth = reinterpret_cast<const int*>(0x088CEF94);
    const int* const gGuiResolutionHeight = reinterpret_cast<const int*>(0x088CEF98);

}

// Replaces the four instructions that copy the logical mode into CreateWindowEx_Shared's width and height
// locals. args is the frame those locals live in, so the slots are addressed by their offsets from it.
extern "C" void __cdecl DefaultSizeMatchesTheFinalWindow(int* args)
{
    const std::size_t kWidth = 0x28 / 4;      // the width handed to ASL__SDL__CreateWindow
    const std::size_t kHeight = 0x2C / 4;     // and the height
    const std::size_t kLogicalW = 0x140 / 4;  // the SDL_Rect GetLogicalMode just filled
    const std::size_t kLogicalH = 0x144 / 4;

    int width = args[kLogicalW];
    int height = args[kLogicalH];

    if (!Windowing::FullscreenWanted()) {
        // From the ini rather than the globals, which still hold the engine's 800x600 default when the
        // first window is created. Both agree once the engine has read the ini, so the file only makes the
        // answer available sooner. The globals are the second choice and the logical mode the last.
        const int configuredWidth = Windowing::ReadGraphicsOption("Width");
        const int configuredHeight = Windowing::ReadGraphicsOption("Height");
        if (configuredWidth > 0 && configuredHeight > 0) {
            width = configuredWidth;
            height = configuredHeight;
        } else if (*gGuiResolutionWidth > 0 && *gGuiResolutionHeight > 0) {
            width = *gGuiResolutionWidth;
            height = *gGuiResolutionHeight;
        }
    }

    args[kWidth] = width;
    args[kHeight] = height;
}

// The SDL flags staged for ASL__SDL__CreateWindow, replacing the store that hardcoded SDL_WINDOW_OPENGL.
// Windowed keeps its decorations, which are correct there.
extern "C" void __cdecl CreationFlagsMatchTheFinalWindow(unsigned* args)
{
    const std::size_t kSdlFlags = 0x14 / 4;
    const unsigned kSdlWindowOpenGl = 0x0002;    // the value the shim always passed
    const unsigned kSdlWindowBorderless = 0x0010;

    unsigned flags = kSdlWindowOpenGl;
    if (Windowing::FullscreenWanted()) {
        flags |= kSdlWindowBorderless;
    }
    args[kSdlFlags] = flags;
}
