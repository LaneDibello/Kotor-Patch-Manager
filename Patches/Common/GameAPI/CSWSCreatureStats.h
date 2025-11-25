#pragma once

#include <windows.h>
#include "GameAPIObject.h"

class CSWSCreatureStats : public GameAPIObject {
public:
    explicit CSWSCreatureStats(void* statsPtr);
    ~CSWSCreatureStats();

    bool HasFeat(WORD feat);
    void AddFeat(WORD feat);
    void RemoveFeat(WORD feat);

    bool HasSpell(BYTE spellList, DWORD spellId, int checkUsable);
    void AddKnownSpell(BYTE classId, DWORD spellId);

    BYTE GetSTRBase();
    BYTE GetDEXBase();
    BYTE GetCONBase();
    BYTE GetINTBase();
    BYTE GetWISBase();
    BYTE GetCHABase();

    void SetSTRBase(BYTE value);
    void SetDEXBase(BYTE value);
    void SetCONBase(BYTE value, int setHP);
    void SetINTBase(BYTE value);
    void SetWISBase(BYTE value);
    void SetCHABase(BYTE value);

    BYTE GetSkillRank(BYTE skill, void* effectObject, int ignoreBonuses);
    void SetSkillRank(BYTE skill, BYTE rank);

    void SetMovementRate(int movementRate);

    void* GetClass(BYTE classIndex);
    BYTE GetClassCount();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:

    typedef bool (__thiscall* HasFeatFn)(void* thisPtr, WORD feat);
    typedef void (__thiscall* AddFeatFn)(void* thisPtr, WORD feat);
    typedef void (__thiscall* RemoveFeatFn)(void* thisPtr, WORD feat);
    typedef bool (__thiscall* HasSpellFn)(void* thisPtr, BYTE spellList, DWORD spellId, int checkUsable);
    typedef void (__thiscall* AddKnownSpellFn)(void* thisPtr, BYTE classId, DWORD spellId);
    typedef void (__thiscall* SetSTRBaseFn)(void* thisPtr, BYTE value);
    typedef void (__thiscall* SetDEXBaseFn)(void* thisPtr, BYTE value);
    typedef void (__thiscall* SetCONBaseFn)(void* thisPtr, BYTE value, int setHP);
    typedef void (__thiscall* SetINTBaseFn)(void* thisPtr, BYTE value);
    typedef void (__thiscall* SetWISBaseFn)(void* thisPtr, BYTE value);
    typedef void (__thiscall* SetCHABaseFn)(void* thisPtr, BYTE value);
    typedef BYTE (__thiscall* GetSkillRankFn)(void* thisPtr, BYTE skill, void* effectObject, int ignoreBonuses);
    typedef void (__thiscall* SetSkillRankFn)(void* thisPtr, BYTE skill, BYTE rank);
    typedef void (__thiscall* SetMovementRateFn)(void* thisPtr, int movementRate);
    typedef void* (__thiscall* GetClassFn)(void* thisPtr, BYTE classIndex);

    static HasFeatFn hasFeat;
    static AddFeatFn addFeat;
    static RemoveFeatFn removeFeat;
    static HasSpellFn hasSpell;
    static AddKnownSpellFn addKnownSpell;
    static SetSTRBaseFn setSTRBase;
    static SetDEXBaseFn setDEXBase;
    static SetCONBaseFn setCONBase;
    static SetINTBaseFn setINTBase;
    static SetWISBaseFn setWISBase;
    static SetCHABaseFn setCHABase;
    static GetSkillRankFn getSkillRank;
    static SetSkillRankFn setSkillRank;
    static SetMovementRateFn setMovementRate;
    static GetClassFn getClass;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetSTRBase;
    static int offsetDEXBase;
    static int offsetCONBase;
    static int offsetINTBase;
    static int offsetWISBase;
    static int offsetCHABase;
    static int offsetClassCount;
};
