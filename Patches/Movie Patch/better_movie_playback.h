#pragma once

#include <windows.h>

namespace BetterMoviePlayback {

constexpr int BaseWidth = 800;
constexpr int BaseHeight = 600;
constexpr int MaximumMovieDimension = 16384;
constexpr int MaximumTextureUnitsToReset = 32;

constexpr SIZE_T MovieBinkOffset = 0x48;
constexpr SIZE_T MovieBufferOffset = 0x4C;
constexpr SIZE_T MovieWindowOffset = 0x50;
constexpr SIZE_T MovieOffsetXOffset = 0x84;
constexpr SIZE_T MovieOffsetYOffset = 0x88;
constexpr DWORD BinkFrameBufferOffset = 0x34;

constexpr DWORD BinkBufferBlitIatAddress = 0x0073D460;
constexpr DWORD BinkCopyToBufferIatAddress = 0x0073D46C;
constexpr DWORD BinkBufferSetOffsetIatAddress = 0x0073D480;
constexpr DWORD BinkBufferSetScaleIatAddress = 0x0073D484;
constexpr DWORD ScreenWidthAddress = 0x0078D1D4;
constexpr DWORD ScreenHeightAddress = 0x0078D1D8;

struct MovieFilterAddresses {
    DWORD binkBufferBlitIatAddress;
    DWORD binkCopyToBufferIatAddress;
    DWORD screenWidthAddress;
    DWORD screenHeightAddress;
};

constexpr MovieFilterAddresses Kotor1MovieFilterAddresses = {
    BinkBufferBlitIatAddress,
    BinkCopyToBufferIatAddress,
    ScreenWidthAddress,
    ScreenHeightAddress
};

constexpr MovieFilterAddresses Kotor2GogMovieFilterAddresses = {
    0x009854BC,
    0x009854C8,
    0x009F22A4,
    0x009F22A8
};

constexpr SIZE_T Kotor2MoviePointerFrameOffset = 0xE0;

extern volatile int* const ScreenWidth;
extern volatile int* const ScreenHeight;

int screenWidth();
int screenHeight();
bool safeReadDword(const void* address, DWORD& value);
bool prepareMovieContext(void* movie);
void releaseMovieContext();
bool blitMovieFilteredImpl(
    void* movie,
    unsigned int rectCount,
    const MovieFilterAddresses& addresses);
void blitMovieOriginalImpl(
    void* movie,
    unsigned int rectCount,
    const MovieFilterAddresses& addresses);

}
