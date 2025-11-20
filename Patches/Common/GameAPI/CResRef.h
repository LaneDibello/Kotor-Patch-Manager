#pragma once
#include "../Common.h"

class CResRef {
public:
    explicit CResRef(void* ptr);

    char* GetCStr();

    void* GetPtr() const { return ptr; }

private:
    void* ptr;
    bool shouldFree;

//TODO: Fill this out
};

#pragma pack(push, 4)
struct CResRef_struct {
    char str[16];
};
#pragma pack(pop)