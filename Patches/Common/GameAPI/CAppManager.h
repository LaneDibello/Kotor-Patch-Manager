#pragma once

#include <windows.h>

class CServerExoApp;
class CClientExoApp;

class CAppManager {
public:
    static CAppManager* GetInstance();
    ~CAppManager();

    CServerExoApp* GetServer();
    CClientExoApp* GetClient();

    void* GetPtr() const { return appManagerPtr; }

private:
    explicit CAppManager(void* appManagerPtr);

    void* appManagerPtr;

    static void InitializeOffsets();
    static bool offsetsInitialized;

    static int offsetClient;
    static int offsetServer;
};
