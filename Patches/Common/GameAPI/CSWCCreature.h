#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CSWCCreature : public CSWCObject {
public:
    explicit CSWCCreature(void* creaturePtr);
    virtual ~CSWCCreature();

    bool GetRunning();
    bool GetStealth();

    void SetRunning(bool running);
    void SetStealth(bool stealth);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetRunning;
    static int offsetStealth;
};
