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

void writeInt(int address, int value) {
    DWORD oldProtect = 0;
    void* target = reinterpret_cast<void*>(address);
    if (VirtualProtect(target, sizeof(value), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        *reinterpret_cast<int*>(target) = value;
        VirtualProtect(target, sizeof(value), oldProtect, &oldProtect);
    }
}

void patchMovieResolutionConstants() {
    int width = GetPrivateProfileIntA("Graphics Options", "Width", screenWidth(), ".\\swkotor.ini");
    int height = GetPrivateProfileIntA("Graphics Options", "Height", screenHeight(), ".\\swkotor.ini");
    if (width < BaseWidth || height < BaseHeight) {
        width = screenWidth();
        height = screenHeight();
    }

    writeInt(0x00403D6C, width);
    writeInt(0x00403D78, height);
    writeInt(0x005F5B3B, width);
    writeInt(0x005F5B43, height);
}

}
