// Skips the one renderer rebuild startup performs for a video mode already in use.
//
// ApplyVideoMode (0x0819A205) applies a mode by rebuilding everything underneath it: the renderer is shut
// down, which destroys the render window, a new window is created, half a second is slept, and the renderer
// is initialised again. On Win32 a window's pixel format is fixed once SetPixelFormat has touched its DC,
// so changing colour depth or multisampling requires a new window; the engine is built around that.
//
// It is also not a one-shot. g_nVideoModeChangePending is a request flag, set by
// CServerExoAppInternal::LoadModule, HandleAltReturn, the SetResolution console command and
// WinMessageHandler, and the apply clears it. So a module load rebuilding the renderer is the design
// working, and suppressing WinMain's call hangs the loop: the flag never clears, the request is reissued
// every iteration, and rendering stops on the last frame drawn.
//
// Which leaves one candidate. Startup reaches the apply three times with identical arguments:
//
//     SetVideoMode    0x081AEB81   first, and the apply that initialises the renderer
//     StartServices   0x081AC661   unconditional, duplicating a mode already applied
//     WinMain         0x0819982C   request-driven, and the only thing that drains the flag
//
// Only StartServices' is an unconditional duplicate, so only that one is skipped. Measured: applies go from
// three to two and windows from four to three. WinMain's still fires, because WinMessageHandler requests a
// re-apply for a message about the render window whether or not StartServices built one.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include <cstring>

namespace {

    // The window InitOpenGLWindow last created. Null before the first one exists, and between a teardown
    // and its replacement, so a null here means an apply is needed whatever the arguments say.
    void** const gRenderWindowHwnd = reinterpret_cast<void**>(0x0890AFF8);

    // Four arguments: width, height, bits per pixel and a fullscreen selector. StartServices and
    // SetVideoMode both stage a fifth slot holding 1, but the body never reads [esp+0xa0], so four is what
    // it consumes and four is what is forwarded.
    typedef void (*ApplyVideoModeFn)(int, int, int, int);
    const ApplyVideoModeFn ApplyVideoMode = reinterpret_cast<ApplyVideoModeFn>(0x0819A205);

    const std::size_t kArgumentCount = 4;

    // What the apply that last ran was asked for. No real mode matches this until one has.
    int gApplied[kArgumentCount] = {-1, -1, -1, -1};

    bool SameAsLastApply(const int* args)
    {
        return std::memcmp(gApplied, args, sizeof(gApplied)) == 0;
    }

    void RunAndRecord(const int* args)
    {
        ApplyVideoMode(args[0], args[1], args[2], args[3]);
        std::memcpy(gApplied, args, sizeof(gApplied));
    }

}

// Replaces the call in CClientExoAppInternal::StartServices. args is the argument list the caller staged, so
// the four values are read and forwarded exactly as assembled.
extern "C" void __cdecl SkipDuplicateVideoModeApply(int* args)
{
    if (*gRenderWindowHwnd != 0 && SameAsLastApply(args)) {
        return;
    }
    RunAndRecord(args);
}

// The first apply of a session comes through CClientExoAppInternal::SetVideoMode and must run, so it is
// reissued here and its arguments recorded, which is what lets the hook above recognise the StartServices
// call as a duplicate.
//
// Reissued rather than left in place because the patcher copies stolen bytes into its stub verbatim, with no
// displacement fixup. A relative call relocated that way resolves against the stub's address and lands
// nowhere, so skip_original_bytes has to be set and the call made from here.
extern "C" void __cdecl RunAndRecordVideoModeApply(int* args)
{
    RunAndRecord(args);
}
