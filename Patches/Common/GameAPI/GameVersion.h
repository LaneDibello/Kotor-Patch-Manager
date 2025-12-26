#pragma once

#include <string>
#include <stdexcept>

// Forward declare sqlite3 types to avoid including sqlite3.h in header
struct sqlite3;
struct sqlite3_stmt;

class GameVersionException : public std::runtime_error {
public:
    explicit GameVersionException(const std::string& message)
        : std::runtime_error(message) {}
};

class GameVersion {
public:
    static bool Initialize(bool force = false);
    static void Shutdown();
    static std::string GetVersionSha();
    static bool IsInitialized();

    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);
    static void* GetGlobalPointer(const std::string& pointerName);

    static int GetOffset(const std::string& className, const std::string& propertyName);

    static bool HasFunction(const std::string& className, const std::string& functionName);
    static bool HasOffset(const std::string& className, const std::string& propertyName);

    static void Reset(bool force = false);

private:
    static bool initialized;
    static std::string versionSha;

    // SQLite database handle and prepared statements
    static sqlite3* db;
    static sqlite3_stmt* stmt_function;
    static sqlite3_stmt* stmt_pointer;
    static sqlite3_stmt* stmt_offset;

    static bool OpenDatabase();
    static bool PrepareStatements();
    static void FinalizeStatements();
};
