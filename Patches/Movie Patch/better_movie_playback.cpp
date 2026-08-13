#include "better_movie_playback.h"

namespace BetterMoviePlayback {

volatile int* const ScreenWidth = reinterpret_cast<volatile int*>(0x0078D1D4);
volatile int* const ScreenHeight = reinterpret_cast<volatile int*>(0x0078D1D8);

int screenWidth() {
    const int width = *ScreenWidth;
    return width > 0 ? width : BaseWidth;
}

int screenHeight() {
    const int height = *ScreenHeight;
    return height > 0 ? height : BaseHeight;
}

bool safeReadDword(const void* address, DWORD& value) {
    __try {
        value = *reinterpret_cast<const DWORD*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        value = 0;
        return false;
    }
}

}
