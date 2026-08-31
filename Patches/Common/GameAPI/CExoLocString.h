#pragma once

#include "../Common.h"
#include "GameAPIObject.h"

class CExoLocString : public GameAPIObject {
public:
    explicit CExoLocString(void* objectPtr);
    ~CExoLocString();

    // Raw CExoLocStringInternal* -- no wrapper for that type yet.
    void* GetInternal();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetInternal;
};
