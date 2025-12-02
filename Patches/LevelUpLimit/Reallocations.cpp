#include "Common.h"

extern "C" void __cdecl InitRequiredExpPerLevel(void* rules)
{
    debugLog("[LevelUpLimit] Running InitRequiredExpPerLevel\nRules: %X", rules);
    int* requiredExpPerLevel = new int[0x33];
    setObjectProperty<int*>(rules, 0x38, requiredExpPerLevel); //required_exp_per_level
    debugLog("[LevelUpLimit] Finished InitRequiredExpPerLevel\nrequiredExpPerLevel: %X", requiredExpPerLevel);

}

extern "C" void __cdecl DisposeRequiredExpPerLevel(void* rules)
{
    debugLog("[LevelUpLimit] Running DisposeRequiredExpPerLevel");
    int* requiredExpPerLevel = getObjectProperty<int*>(rules, 0x38);
    if (requiredExpPerLevel) {
        delete[] requiredExpPerLevel;
    }
}

extern "C" void __cdecl InitNumSpellLevels(void* thisClass)
{
    debugLog("[LevelUpLimit] Running InitNumSpellLevels");
    setObjectProperty<BYTE*>(thisClass, 0x114, new BYTE[0x32]); //level_num_spell_levels
}

extern "C" void __cdecl InitPowerGain(void* thisClass)
{
    debugLog("[LevelUpLimit] Running InitPowerGain");
    BYTE* powerGain = new BYTE[0x32];
    memset(powerGain, 0xff, 0x32);
    setObjectProperty<BYTE*>(thisClass, 0x128, powerGain); //level_power_gain
}

extern "C" void __cdecl InitOtherClassTables(void* thisClass)
{
    debugLog("[LevelUpLimit] Running InitOtherClassTables");
    setObjectProperty<BYTE*>(thisClass, 0x13c, new BYTE[0x32]); //level_bonus_feat_gains
    setObjectProperty<BYTE*>(thisClass, 0x150, new BYTE[0x32]); //level_feat_gains
    setObjectProperty<BYTE*>(thisClass, 0x184, new BYTE[0x32]); //level_effective_cr
}

extern "C" void __cdecl DisposeClassTables(void* thisClass)
{
    debugLog("[LevelUpLimit] Running DisposeClassTables");

    BYTE* numSpellLevels = getObjectProperty<BYTE*>(thisClass, 0x114);
    if (numSpellLevels) {
        delete[] numSpellLevels;
    }
    
    BYTE* levelPowerGain = getObjectProperty<BYTE*>(thisClass, 0x128);
    if (levelPowerGain) {
        delete[] levelPowerGain;
    }

    BYTE* levelBonusFeatGains = getObjectProperty<BYTE*>(thisClass, 0x13c);
    if (levelBonusFeatGains) {
        delete[] levelBonusFeatGains;
    }

    BYTE* levelFeatGains = getObjectProperty<BYTE*>(thisClass, 0x150);
    if (levelFeatGains) {
        delete[] levelFeatGains;
    }

    BYTE* levelEffectiveCR = getObjectProperty<BYTE*>(thisClass, 0x184);
    if (levelEffectiveCR) {
        delete[] levelEffectiveCR;
    }
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}