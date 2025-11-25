#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CSWCCreature;
class CSWSCreatureStats;
class CSWInventory;

class CSWSCreature : public CSWSObject {
public:
    explicit CSWSCreature(void* creaturePtr);
    virtual ~CSWSCreature();

    CSWCCreature* GetClientCreature();

    CSWSCreatureStats* GetCreatureStats();
    CSWInventory* GetInventory();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:

    typedef void* (__thiscall* GetClientCreatureFn)(void* thisPtr);

    static GetClientCreatureFn getClientCreature;
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetCreatureStats;
    static int offsetInventory;
};
