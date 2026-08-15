#pragma once

#include <windows.h>

#include <cstdint>

struct ShaderSwapReplacement {
    unsigned int target;
    std::uint64_t originalHash;
    const void* source;
    unsigned int sourceSize;
};

using ShaderSwapRegisterProviderFn = BOOL (__cdecl*)(
    const ShaderSwapReplacement* replacements,
    unsigned int count);
