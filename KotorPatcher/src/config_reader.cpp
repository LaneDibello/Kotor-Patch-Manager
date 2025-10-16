#include "config_reader.h"
#include <fstream>
#include <sstream>

// Include toml++ header-only library
#define TOML_EXCEPTIONS 0  // Disable exceptions for better error handling
#include "../external/toml.hpp"

namespace KotorPatcher {
    namespace Config {

        // Helper function to convert hex string to DWORD
        static bool ParseHexAddress(const std::string& hexStr, DWORD& outAddress) {
            std::string cleaned = hexStr;
            if (cleaned.size() >= 2 && cleaned[0] == '0' && (cleaned[1] == 'x' || cleaned[1] == 'X')) {
                cleaned = cleaned.substr(2);
            }

            char* endPtr = nullptr;
            unsigned long value = strtoul(cleaned.c_str(), &endPtr, 16);

            if (endPtr == cleaned.c_str() || *endPtr != '\0') {
                return false; // Parse failed
            }

            outAddress = static_cast<DWORD>(value);
            return true;
        }

        // Helper function to parse byte array from TOML
        static bool ParseByteArray(const toml::array* arr, std::vector<BYTE>& outBytes) {
            if (!arr) return false;

            outBytes.clear();
            outBytes.reserve(arr->size());

            for (const auto& elem : *arr) {
                if (elem.is_integer()) {
                    int64_t val = elem.as_integer()->get();
                    if (val < 0 || val > 255) {
                        OutputDebugStringA("[Config] Byte value out of range (0-255)\n");
                        return false;
                    }
                    outBytes.push_back(static_cast<BYTE>(val));
                }
                // Or as hex strings like "0x55"
                else if (elem.is_string()) {
                    std::string hexStr = elem.as_string()->get();
                    DWORD byteVal;
                    if (!ParseHexAddress(hexStr, byteVal) || byteVal > 255) {
                        OutputDebugStringA(("[Config] Invalid byte string: " + hexStr + "\n").c_str());
                        return false;
                    }
                    outBytes.push_back(static_cast<BYTE>(byteVal));
                }
                else {
                    OutputDebugStringA("[Config] Byte array element must be integer or hex string\n");
                    return false;
                }
            }

            return true;
        }

        bool ParseConfig(const std::string& configPath, std::vector<PatchInfo>& outPatches) {
            outPatches.clear();

            // Read the config file
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                OutputDebugStringA(("[Config] Failed to open config file: " + configPath + "\n").c_str());
                return false;
            }

            // Read entire file into string
            std::stringstream buffer;
            buffer << configFile.rdbuf();
            std::string configContent = buffer.str();
            configFile.close();

            // Parse TOML
            toml::parse_result result = toml::parse(configContent);
            if (!result) {
                OutputDebugStringA(("[Config] TOML parse error: " + std::string(result.error().description()) + "\n").c_str());
                return false;
            }

            toml::table tbl = std::move(result).table();

            // Get the patches array
            auto patchesArray = tbl["patches"].as_array();
            if (!patchesArray) {
                OutputDebugStringA("[Config] No 'patches' array found in config\n");
                return false;
            }

            // Iterate through each patch
            for (const auto& patchElem : *patchesArray) {
                auto patchTable = patchElem.as_table();
                if (!patchTable) {
                    OutputDebugStringA("[Config] Patch entry is not a table\n");
                    continue;
                }

                // Get patch ID (optional, for debugging)
                std::string patchId = patchTable->at_path("id").value_or<std::string>("");

                // Get DLL path (required)
                auto dllPath = patchTable->at_path("dll").value<std::string>();
                if (!dllPath) {
                    OutputDebugStringA(("[Config] Patch '" + patchId + "' missing 'dll' field\n").c_str());
                    continue;
                }

                // Get hooks array (required)
                auto hooksArray = patchTable->at_path("hooks").as_array();
                if (!hooksArray) {
                    OutputDebugStringA(("[Config] Patch '" + patchId + "' missing 'hooks' array\n").c_str());
                    continue;
                }

                // Parse each hook
                for (const auto& hookElem : *hooksArray) {
                    auto hookTable = hookElem.as_table();
                    if (!hookTable) {
                        OutputDebugStringA("[Config] Hook entry is not a table\n");
                        continue;
                    }

                    PatchInfo patch;
                    patch.dllPath = *dllPath;

                    // Get hook address (required)
                    auto addressStr = hookTable->at_path("address").value<std::string>();
                    auto addressInt = hookTable->at_path("address").value<int64_t>();

                    if (addressStr) {
                        // Address specified as hex string "0x401234"
                        if (!ParseHexAddress(*addressStr, patch.hookAddress)) {
                            OutputDebugStringA(("[Config] Invalid address format: " + *addressStr + "\n").c_str());
                            continue;
                        }
                    }
                    else if (addressInt) {
                        // Address specified as integer
                        patch.hookAddress = static_cast<DWORD>(*addressInt);
                    }
                    else {
                        OutputDebugStringA("[Config] Hook missing 'address' field\n");
                        continue;
                    }

                    // Get function name (required)
                    auto functionName = hookTable->at_path("function").value<std::string>();
                    if (!functionName) {
                        OutputDebugStringA("[Config] Hook missing 'function' field\n");
                        continue;
                    }
                    patch.functionName = *functionName;

                    // Get original bytes (required for verification)
                    auto originalBytesArray = hookTable->at_path("original_bytes").as_array();
                    if (!originalBytesArray) {
                        OutputDebugStringA("[Config] Hook missing 'original_bytes' array\n");
                        continue;
                    }

                    if (!ParseByteArray(originalBytesArray, patch.originalBytes)) {
                        OutputDebugStringA("[Config] Failed to parse original_bytes\n");
                        continue;
                    }

                    if (patch.originalBytes.empty()) {
                        OutputDebugStringA("[Config] original_bytes array is empty\n");
                        continue;
                    }

                    // === Parse Hook Type (Optional, defaults to INLINE) ===
                    auto typeStr = hookTable->at_path("type").value<std::string>();
                    if (typeStr) {
                        std::string type = *typeStr;
                        if (_stricmp(type.c_str(), "inline") == 0) {
                            patch.type = HookType::INLINE;
                        }
                        else if (_stricmp(type.c_str(), "replace") == 0) {
                            patch.type = HookType::REPLACE;
                        }
                        else if (_stricmp(type.c_str(), "wrap") == 0) {
                            patch.type = HookType::WRAP;
                        }
                        else {
                            OutputDebugStringA(("[Config] Unknown hook type '" + type + "', defaulting to INLINE\n").c_str());
                            patch.type = HookType::INLINE;
                        }
                    }
                    else {
                        // Default to INLINE (safest option)
                        patch.type = HookType::INLINE;
                    }

                    // === Parse State Preservation Options (Optional) ===
                    auto preserveRegs = hookTable->at_path("preserve_registers").value<bool>();
                    if (preserveRegs) {
                        patch.preserveRegisters = *preserveRegs;
                    }

                    auto preserveFlags = hookTable->at_path("preserve_flags").value<bool>();
                    if (preserveFlags) {
                        patch.preserveFlags = *preserveFlags;
                    }

                    // === Parse Exclude From Restore (Optional) ===
                    auto excludeArray = hookTable->at_path("exclude_from_restore").as_array();
                    if (excludeArray) {
                        for (const auto& regElem : *excludeArray) {
                            if (regElem.is_string()) {
                                std::string regName = regElem.as_string()->get();
                                patch.excludeFromRestore.push_back(regName);
                            }
                        }
                    }

                    // Successfully parsed hook - add to list
                    outPatches.push_back(patch);

                    char debugMsg[256];
                    sprintf_s(debugMsg, "[Config] Loaded hook: %s -> %s @ 0x%08X (%zu bytes)\n",
                        patchId.c_str(), patch.functionName.c_str(),
                        patch.hookAddress, patch.originalBytes.size());
                    OutputDebugStringA(debugMsg);
                }
            }

            if (outPatches.empty()) {
                OutputDebugStringA("[Config] Warning: No valid patches loaded\n");
                return false;
            }

            char successMsg[128];
            sprintf_s(successMsg, "[Config] Successfully loaded %zu patch hook(s)\n", outPatches.size());
            OutputDebugStringA(successMsg);

            return true;
        }

    } // namespace Config
} // namespace KotorPatcher
