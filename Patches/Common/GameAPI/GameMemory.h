#pragma once

#include <cstddef>

/// <summary>
/// Allocates and frees memory on the game's own heap
/// Patch-only memory (wrapper objects, scratch buffers) should keep using the CRT
/// </summary>
class GameMemory {
public:
    /// <summary>
    /// Allocates a block with the game's `new` operator
    /// Falls back to the CRT `malloc` otherwise
    /// </summary>
    static void* Alloc(size_t size);

    /// <summary>
    /// Frees a block with the game's `free`
    /// Falls back to the CRT `free` otherwise
    /// </summary>
    static void Free(void* block);

private:
    typedef void* (__cdecl* OperatorNewFn)(size_t size);
    typedef void(__cdecl* FreeFn)(void* block);

    static void Initialize();

    static bool initialized;
    static OperatorNewFn operatorNew;
    static FreeFn gameFree;
};
