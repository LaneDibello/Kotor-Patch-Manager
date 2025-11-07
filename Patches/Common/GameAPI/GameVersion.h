#pragma once

#include <string>
#include <unordered_map>

/// <summary>
/// Centralized game version manager for loading and querying version-specific addresses
///
/// This class loads function addresses and memory offsets from addresses.toml in the current
/// working directory. The SHA-256 hash is read from the KOTOR_VERSION_SHA environment variable
/// set by KotorPatcher.dll. This design ensures complete separation between patch DLLs and
/// KotorPatcher infrastructure.
///
/// Usage:
///   1. Call Initialize() at DLL startup (no parameters - reads from env var and CWD)
///   2. Query addresses via GetFunctionAddress(), GetOffset(), GetGlobalPointer()
///   3. Wrapper classes cache results for performance
/// </summary>
class GameVersion {
public:
    /// <summary>
    /// Initialize the game version system by loading addresses.toml from current directory
    /// Reads target SHA from KOTOR_VERSION_SHA environment variable (set by KotorPatcher)
    /// </summary>
    /// <returns>true if initialization succeeded, false otherwise</returns>
    static bool Initialize();

    /// <summary>
    /// Get the SHA-256 hash of the current game version
    /// </summary>
    /// <returns>Full SHA-256 hash string (64 hex characters)</returns>
    static std::string GetVersionSha();

    /// <summary>
    /// Check if the GameVersion system has been initialized
    /// </summary>
    /// <returns>true if Initialize() has been called successfully</returns>
    static bool IsInitialized();

    /// <summary>
    /// Look up a game function address by class and function name
    /// </summary>
    /// <param name="className">Game class name (e.g., "CVirtualMachine", "CServerExoApp")</param>
    /// <param name="functionName">Function name (e.g., "StackPopInteger", "GetObjectArray")</param>
    /// <returns>Function address as void*, or nullptr if not found</returns>
    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);

    /// <summary>
    /// Look up a global pointer address by name
    /// </summary>
    /// <param name="pointerName">Global pointer name (e.g., "VIRTUAL_MACHINE_PTR", "APP_MANAGER_PTR")</param>
    /// <returns>Pointer address as void*, or nullptr if not found</returns>
    static void* GetGlobalPointer(const std::string& pointerName);

    /// <summary>
    /// Look up a memory offset for an object property
    /// </summary>
    /// <param name="className">Game class name (e.g., "ServerCreature", "CreatureStats")</param>
    /// <param name="propertyName">Property name (e.g., "CreatureStats", "Position")</param>
    /// <returns>Offset in bytes, or -1 if not found</returns>
    static int GetOffset(const std::string& className, const std::string& propertyName);

    /// <summary>
    /// Check if a function is available for the current game version
    /// </summary>
    /// <param name="className">Game class name</param>
    /// <param name="functionName">Function name</param>
    /// <returns>true if function address is available, false otherwise</returns>
    static bool HasFunction(const std::string& className, const std::string& functionName);

    /// <summary>
    /// Check if an offset is available for the current game version
    /// </summary>
    /// <param name="className">Game class name</param>
    /// <param name="propertyName">Property name</param>
    /// <returns>true if offset is available, false otherwise</returns>
    static bool HasOffset(const std::string& className, const std::string& propertyName);

    /// <summary>
    /// Reset the GameVersion system (mainly for testing)
    /// </summary>
    static void Reset();

private:
    // Initialization state
    static bool s_initialized;

    // Current game version SHA-256
    static std::string s_versionSha;

    // Address lookup tables (key = "ClassName.FunctionName")
    static std::unordered_map<std::string, void*> s_functionAddresses;
    static std::unordered_map<std::string, void*> s_globalPointers;
    static std::unordered_map<std::string, int> s_offsets;

    /// <summary>
    /// Load addresses.toml from current working directory
    /// Validates versions_sha field matches s_versionSha (from env var)
    /// </summary>
    /// <returns>true if database loaded successfully, false otherwise</returns>
    static bool LoadAddressDatabase();

    /// <summary>
    /// Build lookup key for function/offset maps
    /// </summary>
    /// <param name="className">Class name</param>
    /// <param name="memberName">Function or property name</param>
    /// <returns>Key string in format "ClassName.MemberName"</returns>
    static std::string MakeKey(const std::string& className, const std::string& memberName);
};
