#include "better_movie_playback.h"

using namespace BetterMoviePlayback;

namespace {

void* movieFromKotor2Frame(void* framePointer) {
    DWORD movie = 0;
    if (!framePointer ||
        !safeReadDword(
            static_cast<char*>(framePointer) - Kotor2MoviePointerFrameOffset,
            movie)) {
        return nullptr;
    }
    return reinterpret_cast<void*>(movie);
}

}

extern "C" void __cdecl applyMovieAspectScale(void* movie) {
    if (!movie) {
        return;
    }

    __try {
        DWORD bink = 0;
        DWORD buffer = 0;
        if (!safeReadDword(static_cast<char*>(movie) + MovieBinkOffset, bink) ||
            !safeReadDword(static_cast<char*>(movie) + MovieBufferOffset, buffer) ||
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

        *reinterpret_cast<int*>(static_cast<char*>(movie) + MovieOffsetXOffset) = offsetX;
        *reinterpret_cast<int*>(static_cast<char*>(movie) + MovieOffsetYOffset) = offsetY;

        typedef void(__stdcall *BinkBufferSetScaleFn)(DWORD, int, int);
        typedef void(__stdcall *BinkBufferSetOffsetFn)(DWORD, int, int);
        BinkBufferSetScaleFn setScale =
            *reinterpret_cast<BinkBufferSetScaleFn*>(BinkBufferSetScaleIatAddress);
        BinkBufferSetOffsetFn setOffset =
            *reinterpret_cast<BinkBufferSetOffsetFn*>(BinkBufferSetOffsetIatAddress);
        if (setScale && setOffset) {
            setScale(buffer, scaledWidth, scaledHeight);
            setOffset(buffer, offsetX, offsetY);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl blitMovieMitchellNetravali(void* movie, unsigned int rectCount) {
    __try {
        blitMovieMitchellNetravaliImpl(
            movie, rectCount, Kotor1MovieFilterAddresses);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        blitMovieOriginalImpl(movie, rectCount, Kotor1MovieFilterAddresses);
    }
}

extern "C" void __cdecl blitMovieMitchellNetravaliKotor2(
    void* framePointer,
    unsigned int rectCount) {
    void* movie = movieFromKotor2Frame(framePointer);
    __try {
        if (!prepareKotor2MovieContext(movie)) {
            blitMovieOriginalImpl(movie, rectCount, Kotor2GogMovieFilterAddresses);
            return;
        }
        blitMovieMitchellNetravaliImpl(
            movie, rectCount, Kotor2GogMovieFilterAddresses);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        blitMovieOriginalImpl(movie, rectCount, Kotor2GogMovieFilterAddresses);
    }
}

extern "C" void __cdecl releaseMovieFilterKotor2() {
    releaseKotor2MovieContext();
}
