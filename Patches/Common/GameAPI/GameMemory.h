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

    // ===== Diagnostics =====

    typedef void (*TraceFn)(const char* message);

    /// <summary>
    /// When set, every Alloc and Free is reported to fn (nullptr turns it off)
    /// </summary>
    static void SetTrace(TraceFn fn);

    /// <summary>
    /// Returns the process heap whose regions hold the block, or nullptr if none does
    /// Walks every heap, so it is slow; diagnostics only
    /// </summary>
    static void* OwnerHeap(void* block);

    /// <summary>
    /// Validates every process heap, reporting each failure to the trace hook
    /// Returns true if all pass
    /// </summary>
    static bool ValidateHeaps(const char* tag);

private:
    typedef void* (__cdecl* OperatorNewFn)(size_t size);
    typedef void(__cdecl* FreeFn)(void* block);

    static void Initialize();
    static void Trace(const char* format, ...);

    static bool initialized;
    static OperatorNewFn operatorNew;
    static FreeFn gameFree;
    static TraceFn trace;
};
