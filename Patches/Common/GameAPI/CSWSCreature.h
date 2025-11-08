#pragma once

#include "../Common.h"

class CSWCCreature;
class CSWSCreatureStats;
class CSWInventory;

class CSWSCreature {
public:
    explicit CSWSCreature(void* creaturePtr);
    ~CSWSCreature();

    CSWCCreature* GetClientCreature();

    CSWSCreatureStats* GetCreatureStats();
    CSWInventory* GetInventory();
    Vector GetPosition();
    float GetOrientation();
    Vector GetOrientationVector();
    DWORD GetAreaId();

    void SetPosition(const Vector& position);
    void SetOrientation(float orientation);
    void SetOrientationVector(const Vector& orientation);
    void SetAreaId(DWORD areaId);

    void* GetPtr() const { return creaturePtr; }

private:
    void* creaturePtr;

    typedef void* (__thiscall* GetClientCreatureFn)(void* thisPtr);

    static GetClientCreatureFn getClientCreature;
    static void InitializeFunctions();
    static bool functionsInitialized;

    static void InitializeOffsets();
    static bool offsetsInitialized;

    static int offsetCreatureStats;
    static int offsetInventory;
    static int offsetPosition;
    static int offsetOrientation;
    static int offsetAreaId;
};
