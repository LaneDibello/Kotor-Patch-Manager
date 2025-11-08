#pragma once

#include <windows.h>

class CAppManager;

class CClientExoApp {
public:
    static CClientExoApp* GetInstance();
    ~CClientExoApp();

    void* GetPtr() const { return clientPtr; }

private:
    friend class CAppManager;
    explicit CClientExoApp(void* clientPtr);

    void* clientPtr;

    static void InitializeFunctions();
    static bool functionsInitialized;
};
