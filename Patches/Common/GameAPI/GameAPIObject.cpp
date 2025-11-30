#include "GameAPIObject.h"

GameAPIObject::GameAPIObject(void* objectPtr, bool shouldFree)
    : objectPtr(objectPtr), shouldFree(shouldFree) {
}

GameAPIObject::~GameAPIObject() {
    // Base destructor doesn't free memory - derived classes handle
    // calling game destructors if needed before this destructor runs
    objectPtr = nullptr;
}

void* GameAPIObject::GetPtr() const {
    return objectPtr;
}

bool GameAPIObject::IsValid() const {
    return objectPtr != nullptr;
}
