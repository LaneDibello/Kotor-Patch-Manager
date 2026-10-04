#include "GameMemory.h"
#include "GameVersion.h"
#include "../Common.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

bool GameMemory::initialized = false;
GameMemory::OperatorNewFn GameMemory::operatorNew = nullptr;
GameMemory::FreeFn GameMemory::gameFree = nullptr;
GameMemory::TraceFn GameMemory::trace = nullptr;

void GameMemory::Initialize() {
    initialized = true;

    // Resolve both or neither, so a block is never allocated on one heap and freed on the other
    OperatorNewFn newFn = nullptr;
    FreeFn freeFn = nullptr;
    if (GameVersion::ResolveFunction(newFn, "Global", "operator_new") &&
        GameVersion::ResolveFunction(freeFn, "Global", "free")) {
        operatorNew = newFn;
        gameFree = freeFn;
        Trace("[GameMemory] using game allocator: operator_new=%p free=%p", newFn, freeFn);
    }
    else {
        debugLog("[GameMemory] WARNING: game allocator not recorded for this version; using the CRT heap");
        Trace("[GameMemory] WARNING: game allocator not recorded for this version; using the CRT heap");
    }
}

void* GameMemory::Alloc(size_t size) {
    if (!initialized) {
        Initialize();
    }

    void* block = operatorNew ? operatorNew(size) : malloc(size);
    Trace("[GameMemory] Alloc 0x%X -> %p (%s)", (unsigned)size, block, operatorNew ? "game" : "crt");
    return block;
}

void GameMemory::Free(void* block) {
    if (!block) {
        return;
    }

    if (!initialized) {
        Initialize();
    }

    Trace("[GameMemory] Free %p (%s)", block, gameFree ? "game" : "crt");
    if (gameFree) {
        gameFree(block);
    }
    else {
        free(block);
    }
}

void GameMemory::SetTrace(TraceFn fn) {
    trace = fn;
}

void GameMemory::Trace(const char* format, ...) {
    if (!trace) {
        return;
    }

    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);
    trace(buffer);
}

void* GameMemory::OwnerHeap(void* block) {
    if (!block) {
        return nullptr;
    }

    // Region ranges only; HeapValidate on a block another heap owns can fail fast like HeapFree
    HANDLE heaps[64];
    DWORD count = GetProcessHeaps(64, heaps);
    if (count > 64) {
        count = 64;
    }

    const char* address = static_cast<const char*>(block);
    for (DWORD i = 0; i < count; ++i) {
        if (!HeapLock(heaps[i])) {
            continue;
        }

        bool found = false;
        PROCESS_HEAP_ENTRY entry = {};
        while (!found && HeapWalk(heaps[i], &entry)) {
            if (entry.wFlags & PROCESS_HEAP_REGION) {
                const char* first = static_cast<const char*>(entry.Region.lpFirstBlock);
                const char* last = static_cast<const char*>(entry.Region.lpLastBlock);
                found = address >= first && address < last;
            }
            else if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
                // Blocks outside any region (large allocations) are matched exactly
                const char* data = static_cast<const char*>(entry.lpData);
                found = address >= data && address < data + entry.cbData;
            }
        }

        HeapUnlock(heaps[i]);
        if (found) {
            return heaps[i];
        }
    }
    return nullptr;
}

bool GameMemory::ValidateHeaps(const char* tag) {
    HANDLE heaps[64];
    DWORD count = GetProcessHeaps(64, heaps);
    if (count > 64) {
        count = 64;
    }

    // Logged first: if validation itself dies on a corrupt heap, this is the last line written
    Trace("[GameMemory] validating %u heap(s) at %s", (unsigned)count, tag);

    bool allValid = true;
    for (DWORD i = 0; i < count; ++i) {
        if (!HeapValidate(heaps[i], 0, nullptr)) {
            Trace("[GameMemory] HEAP %p FAILED validation at %s", heaps[i], tag);
            allValid = false;
        }
    }

    if (allValid) {
        Trace("[GameMemory] all heaps valid at %s", tag);
    }
    return allValid;
}
