#include "better_movie_playback.h"

using namespace BetterMoviePlayback;

extern "C" void __cdecl postMovieCleanup() {
    __try {
        typedef void(__cdecl *ResumeGameWindowFn)();
        ResumeGameWindowFn resumeGameWindow = reinterpret_cast<ResumeGameWindowFn>(0x00401E00);
        resumeGameWindow();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl applyMovieAspectScale(void* movie) {
    if (!movie) {
        return;
    }

    __try {
        DWORD bink = 0;
        DWORD buffer = 0;
        if (!safeReadDword(static_cast<char*>(movie) + 0x48, bink) ||
            !safeReadDword(static_cast<char*>(movie) + 0x4C, buffer) ||
            bink == 0 ||
            buffer == 0) {
            return;
        }

        DWORD movieWidthValue = 0;
        DWORD movieHeightValue = 0;
        if (!safeReadDword(reinterpret_cast<void*>(bink), movieWidthValue) ||
            !safeReadDword(reinterpret_cast<void*>(bink + 4), movieHeightValue)) {
            return;
        }

        const int movieWidth = static_cast<int>(movieWidthValue);
        const int movieHeight = static_cast<int>(movieHeightValue);
        const int targetWidth = screenWidth();
        const int targetHeight = screenHeight();
        if (movieWidth <= 0 || movieHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
            return;
        }

        int scaledWidth = targetWidth;
        int scaledHeight = static_cast<int>((static_cast<long long>(targetWidth) * movieHeight) / movieWidth);
        if (scaledHeight > targetHeight) {
            scaledHeight = targetHeight;
            scaledWidth = static_cast<int>((static_cast<long long>(targetHeight) * movieWidth) / movieHeight);
        }

        const int offsetX = (targetWidth - scaledWidth) / 2;
        const int offsetY = (targetHeight - scaledHeight) / 2;

        *reinterpret_cast<int*>(static_cast<char*>(movie) + 0x84) = offsetX;
        *reinterpret_cast<int*>(static_cast<char*>(movie) + 0x88) = offsetY;

        typedef void(__stdcall *BinkBufferSetScaleFn)(DWORD, int, int);
        typedef void(__stdcall *BinkBufferSetOffsetFn)(DWORD, int, int);
        BinkBufferSetScaleFn setScale = *reinterpret_cast<BinkBufferSetScaleFn*>(0x0073D484);
        BinkBufferSetOffsetFn setOffset = *reinterpret_cast<BinkBufferSetOffsetFn*>(0x0073D480);
        if (setScale && setOffset) {
            setScale(buffer, scaledWidth, scaledHeight);
            setOffset(buffer, offsetX, offsetY);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_ATTACH) {
        __try {
            patchMovieResolutionConstants();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return TRUE;
}
