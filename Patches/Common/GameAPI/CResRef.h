#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class CResRef : public GameAPIObject {
public:
    explicit CResRef(void* ptr);

    char* GetCStr();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    static bool functionsInitialized;
    static bool offsetsInitialized;

//TODO: Fill this out
};

#pragma pack(push, 4)
struct CResRef_struct {
    char str[16];
};
#pragma pack(pop)