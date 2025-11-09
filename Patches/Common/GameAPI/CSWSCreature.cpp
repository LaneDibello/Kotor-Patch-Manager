#include "CSWSCreature.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CSWCCreature.h"
#include "CSWSCreatureStats.h"
#include "CSWInventory.h"

CSWSCreature::GetClientCreatureFn CSWSCreature::getClientCreature = nullptr;
bool CSWSCreature::functionsInitialized = false;

int CSWSCreature::offsetCreatureStats = -1;
int CSWSCreature::offsetInventory = -1;
bool CSWSCreature::offsetsInitialized = false;

void CSWSCreature::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    // Call base class initialization first
    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getClientCreature = reinterpret_cast<GetClientCreatureFn>(
            GameVersion::GetFunctionAddress("CSWSCreature", "GetClientCreature")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreature] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSCreature::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    // Call base class offset initialization (which includes CSWSObject offsets)
    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // CSWSCreature-specific offsets only
        offsetCreatureStats = GameVersion::GetOffset("CSWSCreature", "CreatureStats");
        offsetInventory = GameVersion::GetOffset("CSWSCreature", "Inventory");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreature] ERROR: %s\n", e.what());
    }
}

CSWSCreature::CSWSCreature(void* creaturePtr)
    : CSWSObject(creaturePtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSCreature::~CSWSCreature() {
    // Base class destructor will handle objectPtr cleanup
}

CSWCCreature* CSWSCreature::GetClientCreature() {
    if (!objectPtr || !getClientCreature) {
        return nullptr;
    }
    void* clientCreaturePtr = getClientCreature(objectPtr);
    if (!clientCreaturePtr) {
        return nullptr;
    }
    return new CSWCCreature(clientCreaturePtr);
}

CSWSCreatureStats* CSWSCreature::GetCreatureStats() {
    if (!objectPtr || offsetCreatureStats < 0) {
        return nullptr;
    }
    void* statsPtr = getObjectProperty<void*>(objectPtr, offsetCreatureStats);
    if (!statsPtr) {
        return nullptr;
    }
    return new CSWSCreatureStats(statsPtr);
}

CSWInventory* CSWSCreature::GetInventory() {
    if (!objectPtr || offsetInventory < 0) {
        return nullptr;
    }
    void* inventoryPtr = getObjectProperty<void*>(objectPtr, offsetInventory);
    if (!inventoryPtr) {
        return nullptr;
    }
    return new CSWInventory(inventoryPtr);
}
