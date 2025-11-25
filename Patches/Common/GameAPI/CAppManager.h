#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CServerExoApp;
class CClientExoApp;

class CAppManager : public GameAPIObject {
public:
    static CAppManager* GetInstance();
    ~CAppManager();

    CServerExoApp* GetServer();
    CClientExoApp* GetClient();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    explicit CAppManager(void* appManagerPtr);

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetClient;
    static int offsetServer;
};
