#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class CResRef : public GameAPIObject {
public:
    explicit CResRef(void* ptr);

    // Builds a new, locally-owned 16-byte CResRef from a null-terminated string.
    // The buffer is zero-padded; at most 16 characters are copied (a 16-char
    // resref is not null-terminated within the field). This wrapper owns and
    // frees the buffer.
    explicit CResRef(const char* src);

    ~CResRef();

    // Returns a copy of the string. Caller must free() the returned pointer!
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