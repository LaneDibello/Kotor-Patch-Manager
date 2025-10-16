#pragma once
#include <string>
#include <vector>
#include "patcher.h"

namespace KotorPatcher {
    namespace Config {
        // Parse patch_config.toml and return list of patches
        bool ParseConfig(const std::string& configPath, std::vector<PatchInfo>& outPatches);
    }
}