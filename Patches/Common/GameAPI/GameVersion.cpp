#include "GameVersion.h"
#include <fstream>
#include <sstream>
#include <windows.h>

// Include toml++ header-only library (copied to Patches/Common for independence)
#define TOML_EXCEPTIONS 0  // Disable exceptions for better error handling
#include "../toml.hpp"

// Static member initialization
bool GameVersion::s_initialized = false;
std::string GameVersion::s_versionSha;
std::unordered_map<std::string, void*> GameVersion::s_functionAddresses;
std::unordered_map<std::string, void*> GameVersion::s_globalPointers;
std::unordered_map<std::string, int> GameVersion::s_offsets;

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
    // Reset any previous state
    Reset();

    // Read SHA from environment variable (set by KotorPatcher before loading patch DLLs)
    char envBuffer[512] = {0};
    DWORD result = GetEnvironmentVariableA("KOTOR_VERSION_SHA", envBuffer, sizeof(envBuffer));

    if (result == 0 || result >= sizeof(envBuffer)) {
        OutputDebugStringA("[GameVersion] ERROR: KOTOR_VERSION_SHA environment variable not set or too long\n");
        return false;
    }

    s_versionSha = std::string(envBuffer);
    OutputDebugStringA(("[GameVersion] Target version SHA from env: " + s_versionSha.substr(0, 16) + "...\n").c_str());

    // Load address database from current working directory
    if (!LoadAddressDatabase()) {
        OutputDebugStringA("[GameVersion] Failed to load address database\n");
        return false;
    }

    s_initialized = true;

    OutputDebugStringA(("[GameVersion] Initialized successfully. Loaded " +
        std::to_string(s_functionAddresses.size()) + " functions, " +
        std::to_string(s_globalPointers.size()) + " pointers, " +
        std::to_string(s_offsets.size()) + " offsets\n").c_str());

    return true;
}

bool GameVersion::LoadAddressDatabase() {
    // Load addresses.toml from current working directory (game directory)
    const char* addressDbPath = "addresses.toml";

    OutputDebugStringA("[GameVersion] Loading address database from: addresses.toml (CWD)\n");

    // Open file
    std::ifstream dbFile(addressDbPath);
    if (!dbFile.is_open()) {
        OutputDebugStringA("[GameVersion] ERROR: addresses.toml not found in current working directory\n");
        OutputDebugStringA("[GameVersion] This file should be copied by the C# installer from AddressDatabases/\n");
        return false;
    }

    // Read file content
    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    std::string dbContent = buffer.str();
    dbFile.close();

    // Parse TOML
    toml::parse_result result = toml::parse(dbContent);
    if (!result) {
        OutputDebugStringA(("[GameVersion] Failed to parse addresses.toml: " + std::string(result.error().description()) + "\n").c_str());
        return false;
    }

    toml::table tbl = std::move(result).table();

    // Validate versions_sha matches environment variable
    auto versionsSha = tbl["versions_sha"].value<std::string>();
    if (!versionsSha) {
        OutputDebugStringA("[GameVersion] ERROR: No versions_sha field in addresses.toml\n");
        return false;
    }

    if (*versionsSha != s_versionSha) {
        OutputDebugStringA("[GameVersion] ERROR: Version SHA mismatch!\n");
        OutputDebugStringA(("  Expected (from env): " + s_versionSha.substr(0, 16) + "...\n").c_str());
        OutputDebugStringA(("  Found (in TOML):     " + versionsSha->substr(0, 16) + "...\n").c_str());
        return false;
    }

    OutputDebugStringA(("[GameVersion] Version SHA validated: " + s_versionSha.substr(0, 16) + "...\n").c_str());

    // Parse global_pointers section
    auto globalPointersSection = tbl["global_pointers"].as_table();
    if (globalPointersSection) {
        for (const auto& [key, value] : *globalPointersSection) {
            if (value.is_string()) {
                void* addr = ParseHexAddress(value.value_or<std::string>(""));
                if (addr) {
                    s_globalPointers[std::string(key)] = addr;
                }
            }
        }
    }

    // Parse functions.* sections
    auto functionsSection = tbl["functions"].as_table();
    if (functionsSection) {
        for (const auto& [className, classTable] : *functionsSection) {
            if (classTable.is_table()) {
                for (const auto& [functionName, addrValue] : *classTable.as_table()) {
                    if (addrValue.is_string()) {
                        void* addr = ParseHexAddress(addrValue.value_or<std::string>(""));
                        if (addr) {
                            std::string key = MakeKey(std::string(className), std::string(functionName));
                            s_functionAddresses[key] = addr;
                        }
                    }
                }
            }
        }
    }

    // Parse offsets.* sections
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
                            s_offsets[key] = static_cast<int>(reinterpret_cast<uintptr_t>(addr));
                        }
                    }
                    else if (offsetValue.is_integer()) {
                        std::string key = MakeKey(std::string(className), std::string(propertyName));
                        s_offsets[key] = static_cast<int>(offsetValue.value_or<int64_t>(0));
                    }
                }
            }
        }
    }

    return true;  // Successfully loaded
}

std::string GameVersion::GetVersionSha() {
    return s_versionSha;
}

bool GameVersion::IsInitialized() {
    return s_initialized;
}

void* GameVersion::GetFunctionAddress(const std::string& className, const std::string& functionName) {
    std::string key = MakeKey(className, functionName);
    auto it = s_functionAddresses.find(key);
    if (it != s_functionAddresses.end()) {
        return it->second;
    }
    return nullptr;
}

void* GameVersion::GetGlobalPointer(const std::string& pointerName) {
    auto it = s_globalPointers.find(pointerName);
    if (it != s_globalPointers.end()) {
        return it->second;
    }
    return nullptr;
}

int GameVersion::GetOffset(const std::string& className, const std::string& propertyName) {
    std::string key = MakeKey(className, propertyName);
    auto it = s_offsets.find(key);
    if (it != s_offsets.end()) {
        return it->second;
    }
    return -1;
}

bool GameVersion::HasFunction(const std::string& className, const std::string& functionName) {
    return GetFunctionAddress(className, functionName) != nullptr;
}

bool GameVersion::HasOffset(const std::string& className, const std::string& propertyName) {
    return GetOffset(className, propertyName) >= 0;
}

void GameVersion::Reset() {
    s_initialized = false;
    s_versionSha.clear();
    s_functionAddresses.clear();
    s_globalPointers.clear();
    s_offsets.clear();
}

std::string GameVersion::MakeKey(const std::string& className, const std::string& memberName) {
    return className + "." + memberName;
}
