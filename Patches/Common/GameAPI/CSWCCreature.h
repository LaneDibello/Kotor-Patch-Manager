#pragma once

#include "../Common.h"
#include "GameAPIObject.h"

class CSWCCreature : public GameAPIObject {
public:
    explicit CSWCCreature(void* creaturePtr);
    ~CSWCCreature();

    bool GetRunning();
    bool GetStealth();

    void SetRunning(bool running);
    void SetStealth(bool stealth);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetRunning;
    static int offsetStealth;
};
