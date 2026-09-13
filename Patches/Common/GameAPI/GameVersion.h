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

// Identifies which game this build is. Sourced from the game_version.game_name
// column of addresses.db.
enum class GameTitle { Unknown, KOTOR1, KOTOR2 };

// Identifies the OS/platform a build targets. Sourced from the
// game_version.platform column of addresses.db (added in schema v4). Used to
// select version-specific layout data such as vtable sizes.
enum class GamePlatform { Unknown, Windows, MacOS, Linux };

class GameVersion {
public:
    static bool Initialize(bool force = false);
    static void Shutdown();
    static std::string GetVersionSha();
    static bool IsInitialized();

    static GameTitle GetTitle();
    static GamePlatform GetPlatform();

    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);

    // Populates fn with the address of className::functionName, or leaves it
    // untouched when this version's database has no entry. Returns true when the
    // pointer was populated. Non-throwing, unlike GetFunctionAddress: coverage
    // differs between game versions, so a wrapper can resolve whatever exists and
    // null-guard the rest rather than abandoning every later lookup.
    template<typename Fn>
    static bool ResolveFunction(Fn& fn, const std::string& className, const std::string& functionName) {
        void* address = TryGetFunctionAddress(className, functionName);
        if (!address) {
            return false;
        }

        fn = reinterpret_cast<Fn>(address);
        return true;
    }
    static void* GetGlobalPointer(const std::string& pointerName);

    static int GetOffset(const std::string& className, const std::string& propertyName);

    static int GetClassSize(const std::string& className);

    // Returns a class's virtual table address, or nullptr when unavailable
    // (no such class, the vtable column/value is NULL, or the DB predates
    // schema v5). Non-throwing: not all classes have a vtable, so callers treat
    // a null result as "not recorded" rather than an error.
    static void* GetClassVtable(const std::string& className);

    static bool HasFunction(const std::string& className, const std::string& functionName);
    static bool HasOffset(const std::string& className, const std::string& propertyName);
    static bool HasClass(const std::string& className);

    static void Reset(bool force = false);

private:
    // Non-throwing GetFunctionAddress: logs and returns nullptr when the function
    // is absent. Backs the ResolveFunction template, keeping the logging
    // dependency out of this header.
    static void* TryGetFunctionAddress(const std::string& className, const std::string& functionName);

    static bool initialized;
    static std::string versionSha;
    static GameTitle gameTitle;
    static GamePlatform gamePlatform;

    // SQLite database handle and prepared statements
    static sqlite3* db;
    static sqlite3_stmt* stmt_function;
    static sqlite3_stmt* stmt_pointer;
    static sqlite3_stmt* stmt_offset;
    static sqlite3_stmt* stmt_class_size;
    static sqlite3_stmt* stmt_class_vtable;

    static bool OpenDatabase();
    static bool PrepareStatements();
    static void FinalizeStatements();
};
