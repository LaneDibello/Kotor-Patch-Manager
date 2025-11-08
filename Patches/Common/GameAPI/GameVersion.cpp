#include "GameVersion.h"
#include <fstream>
#include <sstream>
#include <windows.h>

// Include toml++ header-only library (copied to Patches/Common for independence)
#define TOML_EXCEPTIONS 0  // Disable exceptions for better error handling
#include "../toml.hpp"

bool GameVersion::initialized = false;
std::string GameVersion::versionSha;
std::unordered_map<std::string, void*> GameVersion::functionAddresses;
std::unordered_map<std::string, void*> GameVersion::globalPointers;
std::unordered_map<std::string, int> GameVersion::offsets;

// Helper function to convert hex string to address
static void* ParseHexAddress(const std::string& hexStr) {
    std::string cleaned = hexStr;
    if (cleaned.size() >= 2 && cleaned[0] == '0' && (cleaned[1] == 'x' || cleaned[1] == 'X')) {
        cleaned = cleaned.substr(2);
    }

    char* endPtr = nullptr;
    unsigned long long value = strtoull(cleaned.c_str(), &endPtr, 16);

    if (endPtr == cleaned.c_str() || *endPtr != '\0') {
        return nullptr;  // Parse failed
    }

    return reinterpret_cast<void*>(static_cast<uintptr_t>(value));
}

bool GameVersion::Initialize() {
    Reset();

    char envBuffer[512] = {0};
    DWORD result = GetEnvironmentVariableA("KOTOR_VERSION_SHA", envBuffer, sizeof(envBuffer));

    if (result == 0 || result >= sizeof(envBuffer)) {
        OutputDebugStringA("[GameVersion] ERROR: KOTOR_VERSION_SHA environment variable not set or too long\n");
        return false;
    }

    versionSha = std::string(envBuffer);
    OutputDebugStringA(("[GameVersion] Target version SHA from env: " + versionSha.substr(0, 16) + "...\n").c_str());

    if (!LoadAddressDatabase()) {
        OutputDebugStringA("[GameVersion] Failed to load address database\n");
        return false;
    }

    initialized = true;

    OutputDebugStringA(("[GameVersion] Initialized successfully. Loaded " +
        std::to_string(functionAddresses.size()) + " functions, " +
        std::to_string(globalPointers.size()) + " pointers, " +
        std::to_string(offsets.size()) + " offsets\n").c_str());

    return true;
}

bool GameVersion::LoadAddressDatabase() {
    const char* addressDbPath = "addresses.toml";

    OutputDebugStringA("[GameVersion] Loading address database from: addresses.toml (CWD)\n");

    std::ifstream dbFile(addressDbPath);
    if (!dbFile.is_open()) {
        OutputDebugStringA("[GameVersion] ERROR: addresses.toml not found in current working directory\n");
        return false;
    }

    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    std::string dbContent = buffer.str();
    dbFile.close();

    toml::parse_result result = toml::parse(dbContent);
    if (!result) {
        OutputDebugStringA(("[GameVersion] Failed to parse addresses.toml: " + std::string(result.error().description()) + "\n").c_str());
        return false;
    }

    toml::table tbl = std::move(result).table();

    auto versionsSha = tbl["versions_sha"].value<std::string>();
    if (!versionsSha) {
        OutputDebugStringA("[GameVersion] ERROR: No versions_sha field in addresses.toml\n");
        return false;
    }

    if (*versionsSha != versionSha) {
        OutputDebugStringA("[GameVersion] ERROR: Version SHA mismatch!\n");
        OutputDebugStringA(("  Expected (from env): " + versionSha.substr(0, 16) + "...\n").c_str());
        OutputDebugStringA(("  Found (in TOML):     " + versionsSha->substr(0, 16) + "...\n").c_str());
        return false;
    }

    OutputDebugStringA(("[GameVersion] Version SHA validated: " + versionSha.substr(0, 16) + "...\n").c_str());

    auto globalPointersNode = tbl["global_pointers"];
    if (globalPointersNode) {
        auto globalPointersSection = globalPointersNode.as_table();
        if (!globalPointersSection && globalPointersNode.is_table()) {
            globalPointersSection = globalPointersNode.as_table();
        }

        if (globalPointersSection) {
            for (const auto& [key, value] : *globalPointersSection) {
                if (value.is_string()) {
                    void* addr = ParseHexAddress(value.value_or<std::string>(""));
                    if (addr) {
                        std::string keyStr(key.data(), key.length());
                        globalPointers[keyStr] = addr;
                    }
                }
            }
        }
    }

    auto functionsSection = tbl["functions"].as_table();
    if (functionsSection) {
        for (const auto& [className, classTable] : *functionsSection) {
            if (classTable.is_table()) {
                for (const auto& [functionName, addrValue] : *classTable.as_table()) {
                    if (addrValue.is_string()) {
                        void* addr = ParseHexAddress(addrValue.value_or<std::string>(""));
                        if (addr) {
                            std::string key = MakeKey(std::string(className), std::string(functionName));
                            functionAddresses[key] = addr;
                        }
                    }
                }
            }
        }
    }

    auto offsetsSection = tbl["offsets"].as_table();
    if (offsetsSection) {
        for (const auto& [className, classTable] : *offsetsSection) {
            if (classTable.is_table()) {
                for (const auto& [propertyName, offsetValue] : *classTable.as_table()) {
                    if (offsetValue.is_string()) {
                        std::string offsetStr = offsetValue.value_or<std::string>("");
                        void* addr = ParseHexAddress(offsetStr);
                        if (addr) {
                            std::string key = MakeKey(std::string(className), std::string(propertyName));
                            offsets[key] = static_cast<int>(reinterpret_cast<uintptr_t>(addr));
                        }
                    }
                    else if (offsetValue.is_integer()) {
                        std::string key = MakeKey(std::string(className), std::string(propertyName));
                        offsets[key] = static_cast<int>(offsetValue.value_or<int64_t>(0));
                    }
                }
            }
        }
    }

    return true;
}

std::string GameVersion::GetVersionSha() {
    return versionSha;
}

bool GameVersion::IsInitialized() {
    return initialized;
}

void* GameVersion::GetFunctionAddress(const std::string& className, const std::string& functionName) {
    std::string key = MakeKey(className, functionName);
    auto it = functionAddresses.find(key);
    if (it != functionAddresses.end()) {
        return it->second;
    }

    throw GameVersionException("Function address not found: " + key);
}

void* GameVersion::GetGlobalPointer(const std::string& pointerName) {
    auto it = globalPointers.find(pointerName);
    if (it != globalPointers.end()) {
        return it->second;
    }
    return nullptr;
}

int GameVersion::GetOffset(const std::string& className, const std::string& propertyName) {
    std::string key = MakeKey(className, propertyName);
    auto it = offsets.find(key);
    if (it != offsets.end()) {
        return it->second;
    }

    throw GameVersionException("Offset not found: " + key);
}

bool GameVersion::HasFunction(const std::string& className, const std::string& functionName) {
    return GetFunctionAddress(className, functionName) != nullptr;
}

bool GameVersion::HasOffset(const std::string& className, const std::string& propertyName) {
    std::string key = MakeKey(className, propertyName);
    return offsets.find(key) != offsets.end();
}

void GameVersion::Reset() {
    initialized = false;
    versionSha.clear();
    functionAddresses.clear();
    globalPointers.clear();
    offsets.clear();
}

std::string GameVersion::MakeKey(const std::string& className, const std::string& memberName) {
    return className + "." + memberName;
}
