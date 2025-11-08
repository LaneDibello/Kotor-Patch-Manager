#include "CSWSCreatureStats.h"
#include "GameVersion.h"
#include "../Common.h"

CSWSCreatureStats::HasFeatFn CSWSCreatureStats::hasFeat = nullptr;
CSWSCreatureStats::AddFeatFn CSWSCreatureStats::addFeat = nullptr;
CSWSCreatureStats::RemoveFeatFn CSWSCreatureStats::removeFeat = nullptr;
CSWSCreatureStats::HasSpellFn CSWSCreatureStats::hasSpell = nullptr;
CSWSCreatureStats::AddKnownSpellFn CSWSCreatureStats::addKnownSpell = nullptr;
CSWSCreatureStats::SetSTRBaseFn CSWSCreatureStats::setSTRBase = nullptr;
CSWSCreatureStats::SetDEXBaseFn CSWSCreatureStats::setDEXBase = nullptr;
CSWSCreatureStats::SetCONBaseFn CSWSCreatureStats::setCONBase = nullptr;
CSWSCreatureStats::SetINTBaseFn CSWSCreatureStats::setINTBase = nullptr;
CSWSCreatureStats::SetWISBaseFn CSWSCreatureStats::setWISBase = nullptr;
CSWSCreatureStats::SetCHABaseFn CSWSCreatureStats::setCHABase = nullptr;
CSWSCreatureStats::GetSkillRankFn CSWSCreatureStats::getSkillRank = nullptr;
CSWSCreatureStats::SetSkillRankFn CSWSCreatureStats::setSkillRank = nullptr;
CSWSCreatureStats::SetMovementRateFn CSWSCreatureStats::setMovementRate = nullptr;
CSWSCreatureStats::GetClassFn CSWSCreatureStats::getClass = nullptr;
bool CSWSCreatureStats::functionsInitialized = false;
bool CSWSCreatureStats::offsetsInitialized = false;

int CSWSCreatureStats::offsetSTRBase = -1;
int CSWSCreatureStats::offsetDEXBase = -1;
int CSWSCreatureStats::offsetCONBase = -1;
int CSWSCreatureStats::offsetINTBase = -1;
int CSWSCreatureStats::offsetWISBase = -1;
int CSWSCreatureStats::offsetCHABase = -1;
int CSWSCreatureStats::offsetClassCount = -1;

void CSWSCreatureStats::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CCreatureStats] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        hasFeat = reinterpret_cast<HasFeatFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "HasFeat")
        );
        addFeat = reinterpret_cast<AddFeatFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "AddFeat")
        );
        removeFeat = reinterpret_cast<RemoveFeatFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "RemoveFeat")
        );
        hasSpell = reinterpret_cast<HasSpellFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "HasSpell")
        );
        addKnownSpell = reinterpret_cast<AddKnownSpellFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "AddKnownSpell")
        );
        setSTRBase = reinterpret_cast<SetSTRBaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetSTRBase")
        );
        setDEXBase = reinterpret_cast<SetDEXBaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetDEXBase")
        );
        setCONBase = reinterpret_cast<SetCONBaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetCONBase")
        );
        setINTBase = reinterpret_cast<SetINTBaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetINTBase")
        );
        setWISBase = reinterpret_cast<SetWISBaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetWISBase")
        );
        setCHABase = reinterpret_cast<SetCHABaseFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetCHABase")
        );
        getSkillRank = reinterpret_cast<GetSkillRankFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "GetSkillRank")
        );
        setSkillRank = reinterpret_cast<SetSkillRankFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetSkillRank")
        );
        setMovementRate = reinterpret_cast<SetMovementRateFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "SetMovementRate")
        );
        getClass = reinterpret_cast<GetClassFn>(
            GameVersion::GetFunctionAddress("CSWSCreatureStats", "GetClass")
        );
    }
    catch (const GameVersionException& e) {
        debugLog("[CCreatureStats] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSCreatureStats::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSCreatureStats] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetSTRBase = GameVersion::GetOffset("CSWSCreatureStats", "STRBase");
        offsetDEXBase = GameVersion::GetOffset("CSWSCreatureStats", "DEXBase");
        offsetCONBase = GameVersion::GetOffset("CSWSCreatureStats", "CONBase");
        offsetINTBase = GameVersion::GetOffset("CSWSCreatureStats", "INTBase");
        offsetWISBase = GameVersion::GetOffset("CSWSCreatureStats", "WISBase");
        offsetCHABase = GameVersion::GetOffset("CSWSCreatureStats", "CHABase");
        offsetClassCount = GameVersion::GetOffset("CSWSCreatureStats", "ClassCount");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSCreatureStats] ERROR: %s\n", e.what());
    }
}

CSWSCreatureStats::CSWSCreatureStats(void* statsPtr)
    : statsPtr(statsPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSCreatureStats::~CSWSCreatureStats() {
    statsPtr = nullptr;
}

bool CSWSCreatureStats::HasFeat(WORD feat) {
    if (!statsPtr || !hasFeat) {
        return false;
    }
    return hasFeat(statsPtr, feat);
}

void CSWSCreatureStats::AddFeat(WORD feat) {
    if (!statsPtr || !addFeat) {
        return;
    }
    addFeat(statsPtr, feat);
}

void CSWSCreatureStats::RemoveFeat(WORD feat) {
    if (!statsPtr || !removeFeat) {
        return;
    }
    removeFeat(statsPtr, feat);
}

bool CSWSCreatureStats::HasSpell(BYTE spellList, DWORD spellId, int checkUsable) {
    if (!statsPtr || !hasSpell) {
        return false;
    }
    return hasSpell(statsPtr, spellList, spellId, checkUsable);
}

void CSWSCreatureStats::AddKnownSpell(BYTE classId, DWORD spellId) {
    if (!statsPtr || !addKnownSpell) {
        return;
    }
    addKnownSpell(statsPtr, classId, spellId);
}

BYTE CSWSCreatureStats::GetSTRBase() {
    if (!statsPtr || offsetSTRBase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetSTRBase);
}

BYTE CSWSCreatureStats::GetDEXBase() {
    if (!statsPtr || offsetDEXBase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetDEXBase);
}

BYTE CSWSCreatureStats::GetCONBase() {
    if (!statsPtr || offsetCONBase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetCONBase);
}

BYTE CSWSCreatureStats::GetINTBase() {
    if (!statsPtr || offsetINTBase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetINTBase);
}

BYTE CSWSCreatureStats::GetWISBase() {
    if (!statsPtr || offsetWISBase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetWISBase);
}

BYTE CSWSCreatureStats::GetCHABase() {
    if (!statsPtr || offsetCHABase < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetCHABase);
}

void CSWSCreatureStats::SetSTRBase(BYTE value) {
    if (!statsPtr || !setSTRBase) {
        return;
    }
    setSTRBase(statsPtr, value);
}

void CSWSCreatureStats::SetDEXBase(BYTE value) {
    if (!statsPtr || !setDEXBase) {
        return;
    }
    setDEXBase(statsPtr, value);
}

void CSWSCreatureStats::SetCONBase(BYTE value, int setHP) {
    if (!statsPtr || !setCONBase) {
        return;
    }
    setCONBase(statsPtr, value, setHP);
}

void CSWSCreatureStats::SetINTBase(BYTE value) {
    if (!statsPtr || !setINTBase) {
        return;
    }
    setINTBase(statsPtr, value);
}

void CSWSCreatureStats::SetWISBase(BYTE value) {
    if (!statsPtr || !setWISBase) {
        return;
    }
    setWISBase(statsPtr, value);
}

void CSWSCreatureStats::SetCHABase(BYTE value) {
    if (!statsPtr || !setCHABase) {
        return;
    }
    setCHABase(statsPtr, value);
}

BYTE CSWSCreatureStats::GetSkillRank(BYTE skill, void* effectObject, int ignoreBonuses) {
    if (!statsPtr || !getSkillRank) {
        return 0;
    }
    return getSkillRank(statsPtr, skill, effectObject, ignoreBonuses);
}

void CSWSCreatureStats::SetSkillRank(BYTE skill, BYTE rank) {
    if (!statsPtr || !setSkillRank) {
        return;
    }
    setSkillRank(statsPtr, skill, rank);
}

void CSWSCreatureStats::SetMovementRate(int movementRate) {
    if (!statsPtr || !setMovementRate) {
        return;
    }
    setMovementRate(statsPtr, movementRate);
}

void* CSWSCreatureStats::GetClass(BYTE classIndex) {
    if (!statsPtr || !getClass) {
        return nullptr;
    }
    return getClass(statsPtr, classIndex);
}

BYTE CSWSCreatureStats::GetClassCount() {
    if (!statsPtr || offsetClassCount < 0) {
        return 0;
    }
    return *((BYTE*)statsPtr + offsetClassCount);
}
