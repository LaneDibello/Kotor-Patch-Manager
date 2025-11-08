#pragma once

#include "../Common.h"

class CSWCCreature {
public:
    explicit CSWCCreature(void* creaturePtr);
    ~CSWCCreature();

    bool GetRunning();
    bool GetStealth();

    void SetRunning(bool running);
    void SetStealth(bool stealth);

    void* GetPtr() const { return creaturePtr; }

private:
    void* creaturePtr;

    static void InitializeOffsets();

    static bool offsetsInitialized;

    static int offsetRunning;
    static int offsetStealth;
};
