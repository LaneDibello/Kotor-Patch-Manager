#pragma once
#include "../Common.h"

class CResRef {
public:
    explicit CResRef(void* ptr);

    char* GetCStr();

    void* GetPtr() const { return stringPtr; }

private:
    void* ptr;
    bool shouldFree;

//TODO: Fill this out
};