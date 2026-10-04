#include "GameMemory.h"
#include "GameVersion.h"
#include "../Common.h"
#include <cstdlib>

bool GameMemory::initialized = false;
GameMemory::OperatorNewFn GameMemory::operatorNew = nullptr;
GameMemory::FreeFn GameMemory::gameFree = nullptr;

void GameMemory::Initialize() {
    initialized = true;

    // Resolve both or neither, so a block is never allocated on one heap and freed on the other
    OperatorNewFn newFn = nullptr;
    FreeFn freeFn = nullptr;
    if (GameVersion::ResolveFunction(newFn, "Global", "operator_new") &&
        GameVersion::ResolveFunction(freeFn, "Global", "free")) {
        operatorNew = newFn;
        gameFree = freeFn;
    }
    else {
        debugLog("[GameMemory] WARNING: game allocator not recorded for this version; using the CRT heap");
    }
}

void* GameMemory::Alloc(size_t size) {
    if (!initialized) {
        Initialize();
    }

    return operatorNew ? operatorNew(size) : malloc(size);
}

void GameMemory::Free(void* block) {
    if (!block) {
        return;
    }

    if (!initialized) {
        Initialize();
    }

    if (gameFree) {
        gameFree(block);
    }
    else {
        free(block);
    }
}
