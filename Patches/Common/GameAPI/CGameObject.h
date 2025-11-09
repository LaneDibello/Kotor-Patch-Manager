#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"

class CGameObject {
protected:
    void* objectPtr;

    // Static function pointers
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // Static offsets
    static int offsetId;
    static int offsetObjectType;

    // Initialize function pointers and offsets
    static void InitializeFunctions();
    static void InitializeOffsets();

public:
    CGameObject(void* objectPtr);
    virtual ~CGameObject();

    // Public accessor methods
    DWORD GetId();
    WORD GetObjectType();
    void* GetPtr() const;
};
