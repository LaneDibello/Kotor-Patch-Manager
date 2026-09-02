#include "CSWSEncounter.h"
#include "CExoString.h"
#include "CExoLocString.h"
#include "GameVersion.h"

int CSWSEncounter::offsetFaction = -1;
int CSWSEncounter::offsetLocalizedName = -1;
int CSWSEncounter::offsetActive = -1;
int CSWSEncounter::offsetReset = -1;
int CSWSEncounter::offsetResetTime = -1;
int CSWSEncounter::offsetSpawnOption = -1;
int CSWSEncounter::offsetDifficulty = -1;
int CSWSEncounter::offsetDifficultyIndex = -1;
int CSWSEncounter::offsetRecCreatures = -1;
int CSWSEncounter::offsetMaxCreatures = -1;
int CSWSEncounter::offsetNumberSpawned = -1;
int CSWSEncounter::offsetHeartbeatDay = -1;
int CSWSEncounter::offsetHeartbeatTime = -1;
int CSWSEncounter::offsetLastSpawnDay = -1;
int CSWSEncounter::offsetLastSpawnTime = -1;
int CSWSEncounter::offsetStarted = -1;
int CSWSEncounter::offsetExhausted = -1;
int CSWSEncounter::offsetAreaList = -1;
int CSWSEncounter::offsetAreaCount = -1;
int CSWSEncounter::offsetGeometryList = -1;
int CSWSEncounter::offsetGeometryCount = -1;
int CSWSEncounter::offsetCreaturesCount = -1;
int CSWSEncounter::offsetSpawnPointsCount = -1;
int CSWSEncounter::offsetMaxSpawns = -1;
int CSWSEncounter::offsetCurrentSpawns = -1;
int CSWSEncounter::offsetAreaPoints = -1;
int CSWSEncounter::offsetSpawnPoolActive = -1;
int CSWSEncounter::offsetScriptOnEntered = -1;
int CSWSEncounter::offsetScriptOnExit = -1;
int CSWSEncounter::offsetScriptOnHeartbeat = -1;
int CSWSEncounter::offsetScriptOnExhausted = -1;
int CSWSEncounter::offsetScriptOnUserDefined = -1;
int CSWSEncounter::offsetCustomScriptId = -1;
int CSWSEncounter::offsetPlayerOnly = -1;

bool CSWSEncounter::functionsInitialized = false;
bool CSWSEncounter::offsetsInitialized = false;

void CSWSEncounter::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    // No CSWSEncounter functions wrapped yet
    functionsInitialized = true;
}

void CSWSEncounter::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSEncounter] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetFaction = GameVersion::GetOffset("CSWSEncounter", "faction");
        offsetLocalizedName = GameVersion::GetOffset("CSWSEncounter", "localized_name");
        offsetActive = GameVersion::GetOffset("CSWSEncounter", "active");
        offsetReset = GameVersion::GetOffset("CSWSEncounter", "reset");
        offsetResetTime = GameVersion::GetOffset("CSWSEncounter", "reset_time");
        offsetSpawnOption = GameVersion::GetOffset("CSWSEncounter", "spawn_option");
        offsetDifficulty = GameVersion::GetOffset("CSWSEncounter", "difficulty");
        offsetDifficultyIndex = GameVersion::GetOffset("CSWSEncounter", "difficulty_index");
        offsetRecCreatures = GameVersion::GetOffset("CSWSEncounter", "rec_creatures");
        offsetMaxCreatures = GameVersion::GetOffset("CSWSEncounter", "max_creatures");
        offsetNumberSpawned = GameVersion::GetOffset("CSWSEncounter", "number_spawned");
        offsetHeartbeatDay = GameVersion::GetOffset("CSWSEncounter", "heartbeat_day");
        offsetHeartbeatTime = GameVersion::GetOffset("CSWSEncounter", "heartbeat_time");
        offsetLastSpawnDay = GameVersion::GetOffset("CSWSEncounter", "last_spawn_day");
        offsetLastSpawnTime = GameVersion::GetOffset("CSWSEncounter", "last_spawn_time");
        offsetStarted = GameVersion::GetOffset("CSWSEncounter", "started");
        offsetExhausted = GameVersion::GetOffset("CSWSEncounter", "exhausted");
        offsetAreaList = GameVersion::GetOffset("CSWSEncounter", "area_list");
        offsetAreaCount = GameVersion::GetOffset("CSWSEncounter", "area_count");
        offsetGeometryList = GameVersion::GetOffset("CSWSEncounter", "geometry_list");
        offsetGeometryCount = GameVersion::GetOffset("CSWSEncounter", "geometry_count");
        offsetCreaturesCount = GameVersion::GetOffset("CSWSEncounter", "creatures_count");
        offsetSpawnPointsCount = GameVersion::GetOffset("CSWSEncounter", "spawn_points_count");
        offsetMaxSpawns = GameVersion::GetOffset("CSWSEncounter", "max_spawns");
        offsetCurrentSpawns = GameVersion::GetOffset("CSWSEncounter", "current_spawns");
        offsetAreaPoints = GameVersion::GetOffset("CSWSEncounter", "area_points");
        offsetSpawnPoolActive = GameVersion::GetOffset("CSWSEncounter", "spawn_pool_active");
        offsetScriptOnEntered = GameVersion::GetOffset("CSWSEncounter", "script_on_entered");
        offsetScriptOnExit = GameVersion::GetOffset("CSWSEncounter", "script_on_exit");
        offsetScriptOnHeartbeat = GameVersion::GetOffset("CSWSEncounter", "script_on_heartbeat");
        offsetScriptOnExhausted = GameVersion::GetOffset("CSWSEncounter", "script_on_exhausted");
        offsetScriptOnUserDefined = GameVersion::GetOffset("CSWSEncounter", "script_on_user_defined");
        offsetCustomScriptId = GameVersion::GetOffset("CSWSEncounter", "custom_script_id");
        offsetPlayerOnly = GameVersion::GetOffset("CSWSEncounter", "player_only");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSEncounter] ERROR: %s\n", e.what());
    }
}

CSWSEncounter::CSWSEncounter(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSEncounter::~CSWSEncounter() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

DWORD CSWSEncounter::GetFaction() {
    if (!objectPtr || offsetFaction < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetFaction);
}

void CSWSEncounter::SetFaction(DWORD value) {
    if (!objectPtr || offsetFaction < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetFaction, value);
}

CExoLocString* CSWSEncounter::GetLocalizedName() {
    if (!objectPtr || offsetLocalizedName < 0) {
        return nullptr;
    }
    return new CExoLocString(static_cast<BYTE*>(objectPtr) + offsetLocalizedName);
}

int CSWSEncounter::GetActive() {
    if (!objectPtr || offsetActive < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetActive);
}

void CSWSEncounter::SetActive(int value) {
    if (!objectPtr || offsetActive < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetActive, value);
}

int CSWSEncounter::GetReset() {
    if (!objectPtr || offsetReset < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetReset);
}

void CSWSEncounter::SetReset(int value) {
    if (!objectPtr || offsetReset < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetReset, value);
}

int CSWSEncounter::GetResetTime() {
    if (!objectPtr || offsetResetTime < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetResetTime);
}

void CSWSEncounter::SetResetTime(int value) {
    if (!objectPtr || offsetResetTime < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetResetTime, value);
}

int CSWSEncounter::GetSpawnOption() {
    if (!objectPtr || offsetSpawnOption < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetSpawnOption);
}

void CSWSEncounter::SetSpawnOption(int value) {
    if (!objectPtr || offsetSpawnOption < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetSpawnOption, value);
}

int CSWSEncounter::GetDifficulty() {
    if (!objectPtr || offsetDifficulty < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetDifficulty);
}

void CSWSEncounter::SetDifficulty(int value) {
    if (!objectPtr || offsetDifficulty < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetDifficulty, value);
}

int CSWSEncounter::GetDifficultyIndex() {
    if (!objectPtr || offsetDifficultyIndex < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetDifficultyIndex);
}

void CSWSEncounter::SetDifficultyIndex(int value) {
    if (!objectPtr || offsetDifficultyIndex < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetDifficultyIndex, value);
}

int CSWSEncounter::GetRecCreatures() {
    if (!objectPtr || offsetRecCreatures < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetRecCreatures);
}

void CSWSEncounter::SetRecCreatures(int value) {
    if (!objectPtr || offsetRecCreatures < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetRecCreatures, value);
}

int CSWSEncounter::GetMaxCreatures() {
    if (!objectPtr || offsetMaxCreatures < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMaxCreatures);
}

void CSWSEncounter::SetMaxCreatures(int value) {
    if (!objectPtr || offsetMaxCreatures < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMaxCreatures, value);
}

int CSWSEncounter::GetNumberSpawned() {
    if (!objectPtr || offsetNumberSpawned < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetNumberSpawned);
}

void CSWSEncounter::SetNumberSpawned(int value) {
    if (!objectPtr || offsetNumberSpawned < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetNumberSpawned, value);
}

DWORD CSWSEncounter::GetHeartbeatDay() {
    if (!objectPtr || offsetHeartbeatDay < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetHeartbeatDay);
}

void CSWSEncounter::SetHeartbeatDay(DWORD value) {
    if (!objectPtr || offsetHeartbeatDay < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetHeartbeatDay, value);
}

DWORD CSWSEncounter::GetHeartbeatTime() {
    if (!objectPtr || offsetHeartbeatTime < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetHeartbeatTime);
}

void CSWSEncounter::SetHeartbeatTime(DWORD value) {
    if (!objectPtr || offsetHeartbeatTime < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetHeartbeatTime, value);
}

DWORD CSWSEncounter::GetLastSpawnDay() {
    if (!objectPtr || offsetLastSpawnDay < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetLastSpawnDay);
}

void CSWSEncounter::SetLastSpawnDay(DWORD value) {
    if (!objectPtr || offsetLastSpawnDay < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetLastSpawnDay, value);
}

DWORD CSWSEncounter::GetLastSpawnTime() {
    if (!objectPtr || offsetLastSpawnTime < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetLastSpawnTime);
}

void CSWSEncounter::SetLastSpawnTime(DWORD value) {
    if (!objectPtr || offsetLastSpawnTime < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetLastSpawnTime, value);
}

int CSWSEncounter::GetStarted() {
    if (!objectPtr || offsetStarted < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetStarted);
}

void CSWSEncounter::SetStarted(int value) {
    if (!objectPtr || offsetStarted < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetStarted, value);
}

int CSWSEncounter::GetExhausted() {
    if (!objectPtr || offsetExhausted < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetExhausted);
}

void CSWSEncounter::SetExhausted(int value) {
    if (!objectPtr || offsetExhausted < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetExhausted, value);
}

DWORD* CSWSEncounter::GetAreaList() {
    if (!objectPtr || offsetAreaList < 0) {
        return nullptr;
    }
    return getObjectProperty<DWORD*>(objectPtr, offsetAreaList);
}

void CSWSEncounter::SetAreaList(DWORD* value) {
    if (!objectPtr || offsetAreaList < 0) {
        return;
    }
    setObjectProperty<DWORD*>(objectPtr, offsetAreaList, value);
}

int CSWSEncounter::GetAreaCount() {
    if (!objectPtr || offsetAreaCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetAreaCount);
}

void CSWSEncounter::SetAreaCount(int value) {
    if (!objectPtr || offsetAreaCount < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetAreaCount, value);
}

Vector* CSWSEncounter::GetGeometryList() {
    if (!objectPtr || offsetGeometryList < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetGeometryList);
}

void CSWSEncounter::SetGeometryList(Vector* value) {
    if (!objectPtr || offsetGeometryList < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetGeometryList, value);
}

int CSWSEncounter::GetGeometryCount() {
    if (!objectPtr || offsetGeometryCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetGeometryCount);
}

void CSWSEncounter::SetGeometryCount(int value) {
    if (!objectPtr || offsetGeometryCount < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetGeometryCount, value);
}

int CSWSEncounter::GetCreaturesCount() {
    if (!objectPtr || offsetCreaturesCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCreaturesCount);
}

void CSWSEncounter::SetCreaturesCount(int value) {
    if (!objectPtr || offsetCreaturesCount < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCreaturesCount, value);
}

int CSWSEncounter::GetSpawnPointsCount() {
    if (!objectPtr || offsetSpawnPointsCount < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetSpawnPointsCount);
}

void CSWSEncounter::SetSpawnPointsCount(int value) {
    if (!objectPtr || offsetSpawnPointsCount < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetSpawnPointsCount, value);
}

int CSWSEncounter::GetMaxSpawns() {
    if (!objectPtr || offsetMaxSpawns < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMaxSpawns);
}

void CSWSEncounter::SetMaxSpawns(int value) {
    if (!objectPtr || offsetMaxSpawns < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMaxSpawns, value);
}

int CSWSEncounter::GetCurrentSpawns() {
    if (!objectPtr || offsetCurrentSpawns < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCurrentSpawns);
}

void CSWSEncounter::SetCurrentSpawns(int value) {
    if (!objectPtr || offsetCurrentSpawns < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCurrentSpawns, value);
}

float CSWSEncounter::GetAreaPoints() {
    if (!objectPtr || offsetAreaPoints < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetAreaPoints);
}

void CSWSEncounter::SetAreaPoints(float value) {
    if (!objectPtr || offsetAreaPoints < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetAreaPoints, value);
}

float CSWSEncounter::GetSpawnPoolActive() {
    if (!objectPtr || offsetSpawnPoolActive < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetSpawnPoolActive);
}

void CSWSEncounter::SetSpawnPoolActive(float value) {
    if (!objectPtr || offsetSpawnPoolActive < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetSpawnPoolActive, value);
}

CExoString* CSWSEncounter::GetScriptOnEntered() {
    if (!objectPtr || offsetScriptOnEntered < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnEntered);
}

CExoString* CSWSEncounter::GetScriptOnExit() {
    if (!objectPtr || offsetScriptOnExit < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnExit);
}

CExoString* CSWSEncounter::GetScriptOnHeartbeat() {
    if (!objectPtr || offsetScriptOnHeartbeat < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnHeartbeat);
}

CExoString* CSWSEncounter::GetScriptOnExhausted() {
    if (!objectPtr || offsetScriptOnExhausted < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnExhausted);
}

CExoString* CSWSEncounter::GetScriptOnUserDefined() {
    if (!objectPtr || offsetScriptOnUserDefined < 0) {
        return nullptr;
    }
    return new CExoString(static_cast<BYTE*>(objectPtr) + offsetScriptOnUserDefined);
}

int CSWSEncounter::GetCustomScriptId() {
    if (!objectPtr || offsetCustomScriptId < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetCustomScriptId);
}

void CSWSEncounter::SetCustomScriptId(int value) {
    if (!objectPtr || offsetCustomScriptId < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetCustomScriptId, value);
}

int CSWSEncounter::GetPlayerOnly() {
    if (!objectPtr || offsetPlayerOnly < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetPlayerOnly);
}

void CSWSEncounter::SetPlayerOnly(int value) {
    if (!objectPtr || offsetPlayerOnly < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetPlayerOnly, value);
}
