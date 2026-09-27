// Stops fullscreen stretching the frame to the desktop's aspect ratio.
//
// Aspyr's GL renderer ends every frame by blitting its render target onto the window with
// glBlitFramebuffer. Windowed, the destination rect is the render target's own size and the blit is 1:1.
// Fullscreen substitutes the desktop size with no aspect-ratio check, so a render mode whose aspect
// differs from the desktop's is resampled to fit, and the GL_LINEAR filter makes that a smooth stretch.
// Both being 16:9 hides it. A 2560x1440 mode on a 5120x1440 desktop comes out twice as wide as it should.
//
// This rewrites the destination to the largest centred rect of the source's aspect ratio that fits, and
// clears the surface so the margins do not hold whatever the driver left in the buffer. Stretched=1
// restores the stock behaviour.
//
// glBlitFramebuffer takes explicit rects and consults neither glViewport nor the projection, so nothing
// upstream needs to know this happened. Movies are unaffected either way: the Bink player drives its own
// swap and never comes through this blit.
//
// Input does need to know, and the shim is already built for it. HWND_Mac::ScreenToClient maps a screen
// coordinate into engine space as (screen - offset) * scale, and that offset pair is a letterbox origin
// the stock game never sets. Both come from the same rect as the blit, so the cursor cannot land
// somewhere the picture is not.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include "WindowingShim.h"

#include <cstddef>

#include <dlfcn.h>

namespace {

    // glBlitFramebuffer's arguments as the caller stages them on the stack, by slot. The hook sits on the
    // last store before the call, so slot 0 is still unwritten when this runs and is never read.
    const std::size_t kSrcX1 = 2;
    const std::size_t kSrcY1 = 3;
    const std::size_t kDstX0 = 4;
    const std::size_t kDstY0 = 5;
    const std::size_t kDstX1 = 6;
    const std::size_t kDstY1 = 7;

    // HWND_Mac fields the input path reads.
    const std::size_t kScaleX = 0x04;
    const std::size_t kScaleY = 0x0C;
    const std::size_t kLetterboxOffsetX = 0x1C;
    const std::size_t kLetterboxOffsetY = 0x20;

    const unsigned kColourBufferBit = 0x4000;   // GL_COLOR_BUFFER_BIT
    const unsigned kColourClearValue = 0x0C22;  // GL_COLOR_CLEAR_VALUE

    // Black out the whole surface so the letterbox margins are not left holding an old frame. The draw
    // framebuffer and draw buffer the blit targets are already bound by this point, so this lands on the
    // right surface without touching that state. The clear colour is put back: the engine sets it per
    // pass and does not expect it to move underneath.
    //
    // Resolved through dlsym rather than a link-time address because these come from the driver, not from
    // the game.
    void ClearSurface()
    {
        typedef void (*ClearFn)(unsigned);
        typedef void (*ClearColourFn)(float, float, float, float);
        typedef void (*GetFloatFn)(unsigned, float*);

        static ClearFn glClear = reinterpret_cast<ClearFn>(dlsym(RTLD_DEFAULT, "glClear"));
        static ClearColourFn glClearColor =
            reinterpret_cast<ClearColourFn>(dlsym(RTLD_DEFAULT, "glClearColor"));
        static GetFloatFn glGetFloatv = reinterpret_cast<GetFloatFn>(dlsym(RTLD_DEFAULT, "glGetFloatv"));
        if (glClear == 0 || glClearColor == 0 || glGetFloatv == 0) {
            return;
        }

        float previous[4];
        glGetFloatv(kColourClearValue, previous);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(kColourBufferBit);
        glClearColor(previous[0], previous[1], previous[2], previous[3]);
    }

}

// args is the staged argument list of CAurInternalGL::PresentFrame's glBlitFramebuffer call, window the
// HWND_Mac being presented to.
extern "C" void __cdecl FitPresentToSourceAspect(int* args, unsigned char* window)
{
    const int srcW = args[kSrcX1];
    const int srcH = args[kSrcY1];
    const int dstW = args[kDstX1];
    const int dstH = args[kDstY1];

    // Equal aspect ratios scale without distortion, so those keep filling the surface.
    if (!Windowing::NeedsScaling(srcW, srcH, dstW, dstH) || Windowing::StretchWanted()) {
        return;
    }

    // Scale by whichever axis runs out first, then centre the remainder.
    int fitW = dstW;
    int fitH = srcH * dstW / srcW;
    if (fitH > dstH) {
        fitH = dstH;
        fitW = srcW * dstH / srcH;
    }

    const int originX = (dstW - fitW) / 2;
    const int originY = (dstH - fitH) / 2;
    args[kDstX0] = originX;
    args[kDstY0] = originY;
    args[kDstX1] = originX + fitW;
    args[kDstY1] = originY + fitH;

    // Point the cursor at where the image is. Rewritten every frame rather than once, because
    // HWND_Mac::RecomputeScale zeroes the offset on every size or fullscreen change.
    *reinterpret_cast<double*>(window + kScaleX) = static_cast<double>(srcW) / fitW;
    *reinterpret_cast<double*>(window + kScaleY) = static_cast<double>(srcH) / fitH;
    *reinterpret_cast<int*>(window + kLetterboxOffsetX) = originX;
    *reinterpret_cast<int*>(window + kLetterboxOffsetY) = originY;

    ClearSurface();
}
