#pragma once
#include <windows.h>
#include "GameAPI/CClientOptions.h"

class CAppManager;

class CClientExoApp {
public:
    static CClientExoApp* GetInstance();
    ~CClientExoApp();

    CClientOptions* GetClientOptions();
    void* GetPtr() const { return clientPtr; }

private:
    friend class CAppManager;
    explicit CClientExoApp(void* clientPtr);

    void* clientPtr;

    typedef void* (__thiscall* GetClientOptionsFn)(void* thisPtr);

    static GetClientOptionsFn getClientOptions;

    static void InitializeFunctions();
    static bool functionsInitialized;
};
