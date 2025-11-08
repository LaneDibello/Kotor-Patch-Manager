#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

class GameVersionException : public std::runtime_error {
public:
    explicit GameVersionException(const std::string& message)
        : std::runtime_error(message) {}
};

class GameVersion {
public:
    static bool Initialize();
    static std::string GetVersionSha();
    static bool IsInitialized();

    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);
    static void* GetGlobalPointer(const std::string& pointerName);

    static int GetOffset(const std::string& className, const std::string& propertyName);

    static bool HasFunction(const std::string& className, const std::string& functionName);
    static bool HasOffset(const std::string& className, const std::string& propertyName);

    static void Reset();

private:
    static bool initialized;
    static std::string versionSha;
    static std::unordered_map<std::string, void*> functionAddresses;
    static std::unordered_map<std::string, void*> globalPointers;
    static std::unordered_map<std::string, int> offsets;

    static bool LoadAddressDatabase();
    static std::string MakeKey(const std::string& className, const std::string& memberName);
};
