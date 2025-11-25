#pragma once
#include <windows.h>
#include "GameAPIObject.h"
#include "GameAPI/CClientOptions.h"

class CAppManager;

class CClientExoApp : public GameAPIObject {
public:
    static CClientExoApp* GetInstance();
    ~CClientExoApp();

    CClientOptions* GetClientOptions();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    friend class CAppManager;
    explicit CClientExoApp(void* clientPtr);

    typedef void* (__thiscall* GetClientOptionsFn)(void* thisPtr);

    static GetClientOptionsFn getClientOptions;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
