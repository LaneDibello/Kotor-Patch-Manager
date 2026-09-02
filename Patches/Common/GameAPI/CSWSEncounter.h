#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoLocString;
class CExoString;

/// <summary>
/// Wraps a server-side encounter spawner.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWSEncounter : public CSWSObject {
public:
    explicit CSWSEncounter(void* objectPtr);
    virtual ~CSWSEncounter();


    // ===== Offsets =====
    DWORD GetFaction();
    void SetFaction(DWORD value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoLocString* GetLocalizedName();
    int GetActive();
    void SetActive(int value);
    int GetReset();
    void SetReset(int value);
    int GetResetTime();
    void SetResetTime(int value);
    int GetSpawnOption();
    void SetSpawnOption(int value);
    int GetDifficulty();
    void SetDifficulty(int value);
    int GetDifficultyIndex();
    void SetDifficultyIndex(int value);
    int GetRecCreatures();
    void SetRecCreatures(int value);
    int GetMaxCreatures();
    void SetMaxCreatures(int value);
    int GetNumberSpawned();
    void SetNumberSpawned(int value);
    DWORD GetHeartbeatDay();
    void SetHeartbeatDay(DWORD value);
    DWORD GetHeartbeatTime();
    void SetHeartbeatTime(DWORD value);
    DWORD GetLastSpawnDay();
    void SetLastSpawnDay(DWORD value);
    DWORD GetLastSpawnTime();
    void SetLastSpawnTime(DWORD value);
    int GetStarted();
    void SetStarted(int value);
    int GetExhausted();
    void SetExhausted(int value);
    DWORD* GetAreaList();
    void SetAreaList(DWORD* value);
    int GetAreaCount();
    void SetAreaCount(int value);
    Vector* GetGeometryList();
    void SetGeometryList(Vector* value);
    int GetGeometryCount();
    void SetGeometryCount(int value);
    int GetCreaturesCount();
    void SetCreaturesCount(int value);
    int GetSpawnPointsCount();
    void SetSpawnPointsCount(int value);
    int GetMaxSpawns();
    void SetMaxSpawns(int value);
    int GetCurrentSpawns();
    void SetCurrentSpawns(int value);
    float GetAreaPoints();
    void SetAreaPoints(float value);
    float GetSpawnPoolActive();
    void SetSpawnPoolActive(float value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnEntered();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnExit();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnHeartbeat();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnExhausted();
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CExoString* GetScriptOnUserDefined();
    int GetCustomScriptId();
    void SetCustomScriptId(int value);
    int GetPlayerOnly();
    void SetPlayerOnly(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetFaction;
    static int offsetLocalizedName;
    static int offsetActive;
    static int offsetReset;
    static int offsetResetTime;
    static int offsetSpawnOption;
    static int offsetDifficulty;
    static int offsetDifficultyIndex;
    static int offsetRecCreatures;
    static int offsetMaxCreatures;
    static int offsetNumberSpawned;
    static int offsetHeartbeatDay;
    static int offsetHeartbeatTime;
    static int offsetLastSpawnDay;
    static int offsetLastSpawnTime;
    static int offsetStarted;
    static int offsetExhausted;
    static int offsetAreaList;
    static int offsetAreaCount;
    static int offsetGeometryList;
    static int offsetGeometryCount;
    static int offsetCreaturesCount;
    static int offsetSpawnPointsCount;
    static int offsetMaxSpawns;
    static int offsetCurrentSpawns;
    static int offsetAreaPoints;
    static int offsetSpawnPoolActive;
    static int offsetScriptOnEntered;
    static int offsetScriptOnExit;
    static int offsetScriptOnHeartbeat;
    static int offsetScriptOnExhausted;
    static int offsetScriptOnUserDefined;
    static int offsetCustomScriptId;
    static int offsetPlayerOnly;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
