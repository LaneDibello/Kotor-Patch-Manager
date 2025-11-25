#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"
#include "GameAPIObject.h"

class CGameObject : public GameAPIObject {
protected:
    // Static function pointers
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // Static offsets
    static int offsetId;
    static int offsetObjectType;

public:
    CGameObject(void* objectPtr);
    virtual ~CGameObject();

    // Public accessor methods
    DWORD GetId();
    WORD GetObjectType();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;
};
