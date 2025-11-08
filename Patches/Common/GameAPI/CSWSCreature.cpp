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
int CSWSCreature::offsetPosition = -1;
int CSWSCreature::offsetOrientation = -1;
int CSWSCreature::offsetAreaId = -1;
bool CSWSCreature::offsetsInitialized = false;

void CSWSCreature::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

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

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetCreatureStats = GameVersion::GetOffset("CSWSCreature", "CreatureStats");
        offsetInventory = GameVersion::GetOffset("CSWSCreature", "Inventory");
        offsetPosition = GameVersion::GetOffset("CSWSCreature", "Position");
        offsetOrientation = GameVersion::GetOffset("CSWSCreature", "Orientation");
        offsetAreaId = GameVersion::GetOffset("CSWSCreature", "AreaId");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreature] ERROR: %s\n", e.what());
    }
}

CSWSCreature::CSWSCreature(void* creaturePtr)
    : creaturePtr(creaturePtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSCreature::~CSWSCreature() {
    creaturePtr = nullptr;
}

CSWCCreature* CSWSCreature::GetClientCreature() {
    if (!creaturePtr || !getClientCreature) {
        return nullptr;
    }
    void* clientCreaturePtr = getClientCreature(creaturePtr);
    if (!clientCreaturePtr) {
        return nullptr;
    }
    return new CSWCCreature(clientCreaturePtr);
}

CSWSCreatureStats* CSWSCreature::GetCreatureStats() {
    if (!creaturePtr || offsetCreatureStats < 0) {
        return nullptr;
    }
    void* statsPtr = getObjectProperty<void*>(creaturePtr, offsetCreatureStats);
    if (!statsPtr) {
        return nullptr;
    }
    return new CSWSCreatureStats(statsPtr);
}

CSWInventory* CSWSCreature::GetInventory() {
    if (!creaturePtr || offsetInventory < 0) {
        return nullptr;
    }
    void* inventoryPtr = getObjectProperty<void*>(creaturePtr, offsetInventory);
    if (!inventoryPtr) {
        return nullptr;
    }
    return new CSWInventory(inventoryPtr);
}

Vector CSWSCreature::GetPosition() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!creaturePtr || offsetPosition < 0) {
        return result;
    }

    return getObjectProperty<Vector>(creaturePtr, offsetPosition);
}

float CSWSCreature::GetOrientation() {
    if (!creaturePtr || offsetOrientation < 0) {
        return 0.0f;
    }
    return getObjectProperty<float>(creaturePtr, offsetOrientation);
}

Vector CSWSCreature::GetOrientationVector() {
    Vector result = {0.0f, 0.0f, 0.0f};

    if (!creaturePtr || offsetOrientation < 0) {
        return result;
    }

    return getObjectProperty<Vector>(creaturePtr, offsetOrientation);
}

DWORD CSWSCreature::GetAreaId() {
    if (!creaturePtr || offsetAreaId < 0) {
        return 0x7F000000;
    }
    return getObjectProperty<DWORD>(creaturePtr, offsetAreaId);
}

void CSWSCreature::SetPosition(const Vector& position) {
    if (!creaturePtr || offsetPosition < 0) {
        return;
    }
    setObjectProperty<Vector>(creaturePtr, offsetPosition, position);
}

void CSWSCreature::SetOrientation(float orientation) {
    if (!creaturePtr || offsetOrientation < 0) {
        return;
    }
    setObjectProperty<float>(creaturePtr, offsetOrientation, orientation);
}

void CSWSCreature::SetOrientationVector(const Vector& orientation) {
    if (!creaturePtr || offsetOrientation < 0) {
        return;
    }
    setObjectProperty<Vector>(creaturePtr, offsetOrientation, orientation);
}

void CSWSCreature::SetAreaId(DWORD areaId) {
    if (!creaturePtr || offsetAreaId < 0) {
        return;
    }
    setObjectProperty<DWORD>(creaturePtr, offsetAreaId, areaId);
}
