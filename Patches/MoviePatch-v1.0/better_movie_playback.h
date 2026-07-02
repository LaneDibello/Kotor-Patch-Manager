#pragma once

#include <windows.h>

namespace BetterMoviePlayback {

constexpr int BaseWidth = 800;
constexpr int BaseHeight = 600;

extern volatile int* const ScreenWidth;
extern volatile int* const ScreenHeight;

int screenWidth();
int screenHeight();
bool safeReadDword(const void* address, DWORD& value);
void writeInt(int address, int value);
void patchMovieResolutionConstants();

}
