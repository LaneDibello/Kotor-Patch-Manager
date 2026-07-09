#include "GameVersion.h"
#include <windows.h>
#include <sqlite3.h>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {
    // Case-insensitive comparison of two ASCII strings.
    bool iequals(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) {
                return false;
            }
        }
        return true;
    }

    GameTitle ParseTitle(const std::string& name) {
        if (iequals(name, "KOTOR1")) return GameTitle::KOTOR1;
        if (iequals(name, "KOTOR2")) return GameTitle::KOTOR2;
        return GameTitle::Unknown;
    }

    GamePlatform ParsePlatform(const std::string& name) {
        if (iequals(name, "Windows")) return GamePlatform::Windows;
        if (iequals(name, "MacOS"))   return GamePlatform::MacOS;
        if (iequals(name, "Linux"))   return GamePlatform::Linux;
        return GamePlatform::Unknown;
    }
}

bool GameVersion::initialized = false;
std::string GameVersion::versionSha;
GameTitle GameVersion::gameTitle = GameTitle::Unknown;
GamePlatform GameVersion::gamePlatform = GamePlatform::Unknown;
sqlite3* GameVersion::db = nullptr;
sqlite3_stmt* GameVersion::stmt_function = nullptr;
sqlite3_stmt* GameVersion::stmt_pointer = nullptr;
sqlite3_stmt* GameVersion::stmt_offset = nullptr;
sqlite3_stmt* GameVersion::stmt_class_size = nullptr;
sqlite3_stmt* GameVersion::stmt_class_vtable = nullptr;

bool GameVersion::Initialize(bool force) {
    // Quick return if already initialized
    if (initialized && !force) {
        OutputDebugStringA("[GameVersion] Already initialized, skipping redundant initialization\n");
        return true;
    }

    Reset(true);

    // Get version SHA from environment variable
    char envBuffer[512] = {0};
    DWORD result = GetEnvironmentVariableA("KOTOR_VERSION_SHA", envBuffer, sizeof(envBuffer));

    if (result == 0 || result >= sizeof(envBuffer)) {
        OutputDebugStringA("[GameVersion] ERROR: KOTOR_VERSION_SHA environment variable not set or too long\n");
        return false;
    }

    versionSha = std::string(envBuffer);
    OutputDebugStringA(("[GameVersion] Target version SHA from env: " + versionSha.substr(0, 16) + "...\n").c_str());

    if (!OpenDatabase()) {
        OutputDebugStringA("[GameVersion] Failed to open database\n");
        return false;
    }

    if (!PrepareStatements()) {
        OutputDebugStringA("[GameVersion] Failed to prepare statements\n");
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    initialized = true;
    OutputDebugStringA("[GameVersion] Initialized successfully with SQLite database\n");

    return true;
}

bool GameVersion::OpenDatabase() {
    const char* dbPath = "addresses.db";

    // Get and log the full path of the database file
    char fullPath[MAX_PATH];
    if (GetFullPathNameA(dbPath, MAX_PATH, fullPath, nullptr)) {
        OutputDebugStringA(("[GameVersion] Opening SQLite database: " + std::string(fullPath) + "\n").c_str());
    } else {
        OutputDebugStringA("[GameVersion] Opening SQLite database: addresses.db (could not resolve full path)\n");
    }

    // Open database with read-only and no-mutex flags for performance
    int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX;
    int rc = sqlite3_open_v2(dbPath, &db, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string error = "[GameVersion] ERROR: Failed to open addresses.db: ";
        if (db) {
            error += sqlite3_errmsg(db);
            sqlite3_close(db);
            db = nullptr;
        }
        else {
            error += "unable to allocate database";
        }
        OutputDebugStringA((error + "\n").c_str());
        return false;
    }

    // Verify game version SHA matches, and capture the game name / platform of
    // the matching row. The platform column was added in schema v4, so an older
    // addresses.db may not have it -- fall back to a name-only query in that case
    // and leave the platform as Unknown rather than failing initialization.
    sqlite3_stmt* stmt = nullptr;
    bool havePlatformColumn = true;
    rc = sqlite3_prepare_v2(db, "SELECT sha256_hash, game_name, platform FROM game_version", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        OutputDebugStringA("[GameVersion] WARNING: platform column missing, falling back to name-only version query\n");
        havePlatformColumn = false;
        rc = sqlite3_prepare_v2(db, "SELECT sha256_hash, game_name FROM game_version", -1, &stmt, nullptr);
    }
    if (rc != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare version query: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    bool foundAnyRow = false;
    bool matchFound = false;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        foundAnyRow = true;

        const unsigned char* txt = sqlite3_column_text(stmt, 0);
        if (!txt) continue;

        const char* dbHash = reinterpret_cast<const char*>(txt);
        if (versionSha == dbHash) {
            matchFound = true;

            const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
            if (nameTxt) {
                gameTitle = ParseTitle(reinterpret_cast<const char*>(nameTxt));
            }

            if (havePlatformColumn) {
                const unsigned char* platTxt = sqlite3_column_text(stmt, 2);
                if (platTxt) {
                    gamePlatform = ParsePlatform(reinterpret_cast<const char*>(platTxt));
                }
            }

            break;
        }
    }

    sqlite3_finalize(stmt);

    if (!foundAnyRow) {
        OutputDebugStringA("[GameVersion] ERROR: No game version found in database\n");
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    if (!matchFound) {
        OutputDebugStringA("[GameVersion] ERROR: Version SHA mismatch!\n");
        OutputDebugStringA(("  Expected (from env): " + versionSha.substr(0, 16) + "...\n").c_str());
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    OutputDebugStringA(("[GameVersion] Version SHA validated: " + versionSha.substr(0, 16) + "...\n").c_str());

    return true;
}

bool GameVersion::PrepareStatements() {
    // Prepare function address lookup
    const char* sql_function = "SELECT address FROM functions WHERE class_name = ? AND function_name = ?";
    if (sqlite3_prepare_v2(db, sql_function, -1, &stmt_function, nullptr) != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare function statement: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        return false;
    }

    // Prepare global pointer lookup
    const char* sql_pointer = "SELECT address FROM global_pointers WHERE pointer_name = ?";
    if (sqlite3_prepare_v2(db, sql_pointer, -1, &stmt_pointer, nullptr) != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare pointer statement: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        FinalizeStatements();
        return false;
    }

    // Prepare offset lookup
    const char* sql_offset = "SELECT offset FROM offsets WHERE class_name = ? AND member_name = ?";
    if (sqlite3_prepare_v2(db, sql_offset, -1, &stmt_offset, nullptr) != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare offset statement: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        FinalizeStatements();
        return false;
    }

    // Prepare class size lookup. The classes table was added in schema v3, so an
    // older addresses.db may not have it. Treat this as non-fatal: leave the
    // statement null and let GetClassSize/HasClass report the size as unavailable
    // rather than breaking every other lookup.
    const char* sql_class_size = "SELECT size FROM classes WHERE class_name = ?";
    if (sqlite3_prepare_v2(db, sql_class_size, -1, &stmt_class_size, nullptr) != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] WARNING: Failed to prepare class size statement (classes table may be missing): " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        stmt_class_size = nullptr;
    }

    // Prepare class vtable lookup. The vtable column was added in schema v5, so a
    // DB migrated only to v3/v4 has the classes table but not this column. Treat
    // a failed prepare as non-fatal: leave the statement null and let
    // GetClassVtable report the vtable as unavailable (returns nullptr).
    const char* sql_class_vtable = "SELECT vtable FROM classes WHERE class_name = ?";
    if (sqlite3_prepare_v2(db, sql_class_vtable, -1, &stmt_class_vtable, nullptr) != SQLITE_OK) {
        OutputDebugStringA(("[GameVersion] WARNING: Failed to prepare class vtable statement (vtable column may be missing): " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
        stmt_class_vtable = nullptr;
    }

    return true;
}

void GameVersion::FinalizeStatements() {
    if (stmt_function) {
        sqlite3_finalize(stmt_function);
        stmt_function = nullptr;
    }
    if (stmt_pointer) {
        sqlite3_finalize(stmt_pointer);
        stmt_pointer = nullptr;
    }
    if (stmt_offset) {
        sqlite3_finalize(stmt_offset);
        stmt_offset = nullptr;
    }
    if (stmt_class_size) {
        sqlite3_finalize(stmt_class_size);
        stmt_class_size = nullptr;
    }
    if (stmt_class_vtable) {
        sqlite3_finalize(stmt_class_vtable);
        stmt_class_vtable = nullptr;
    }
}

void GameVersion::Shutdown() {
    if (!initialized) return;

    FinalizeStatements();

    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }

    initialized = false;
    OutputDebugStringA("[GameVersion] Shutdown complete\n");
}

std::string GameVersion::GetVersionSha() {
    return versionSha;
}

bool GameVersion::IsInitialized() {
    return initialized;
}

GameTitle GameVersion::GetTitle() {
    return gameTitle;
}

GamePlatform GameVersion::GetPlatform() {
    return gamePlatform;
}

void* GameVersion::GetFunctionAddress(const std::string& className, const std::string& functionName) {
    if (!initialized) {
        throw GameVersionException("GameVersion not initialized");
    }

    // Bind parameters
    sqlite3_reset(stmt_function);
    sqlite3_bind_text(stmt_function, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_function, 2, functionName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_function);
    if (rc != SQLITE_ROW) {
        std::stringstream ss;
        ss << "Function address not found: " << className << "::" << functionName;
        throw GameVersionException(ss.str());
    }

    // Get result (stored as integer)
    sqlite3_int64 address = sqlite3_column_int64(stmt_function, 0);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(address));
}

void* GameVersion::GetGlobalPointer(const std::string& pointerName) {
    if (!initialized) {
        throw GameVersionException("GameVersion not initialized");
    }

    // Bind parameter
    sqlite3_reset(stmt_pointer);
    sqlite3_bind_text(stmt_pointer, 1, pointerName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_pointer);
    if (rc != SQLITE_ROW) {
        return nullptr;
    }

    // Get result
    sqlite3_int64 address = sqlite3_column_int64(stmt_pointer, 0);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(address));
}

int GameVersion::GetOffset(const std::string& className, const std::string& propertyName) {
    if (!initialized) {
        throw GameVersionException("GameVersion not initialized");
    }

    // Bind parameters
    sqlite3_reset(stmt_offset);
    sqlite3_bind_text(stmt_offset, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_offset, 2, propertyName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_offset);
    if (rc != SQLITE_ROW) {
        std::stringstream ss;
        ss << "Offset not found: " << className << "::" << propertyName;
        throw GameVersionException(ss.str());
    }

    // Get result
    return sqlite3_column_int(stmt_offset, 0);
}

int GameVersion::GetClassSize(const std::string& className) {
    if (!initialized) {
        throw GameVersionException("GameVersion not initialized");
    }

    if (!stmt_class_size) {
        throw GameVersionException("Class size lookup unavailable (classes table missing)");
    }

    // Bind parameter
    sqlite3_reset(stmt_class_size);
    sqlite3_bind_text(stmt_class_size, 1, className.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_class_size);
    if (rc != SQLITE_ROW) {
        std::stringstream ss;
        ss << "Class size not found: " << className;
        throw GameVersionException(ss.str());
    }

    // Get result
    return sqlite3_column_int(stmt_class_size, 0);
}

void* GameVersion::GetClassVtable(const std::string& className) {
    // Non-throwing: a missing class, NULL vtable, or a DB without the vtable
    // column (statement never prepared) all mean "no vtable recorded".
    if (!initialized || !stmt_class_vtable) {
        return nullptr;
    }

    // Bind parameter
    sqlite3_reset(stmt_class_vtable);
    sqlite3_bind_text(stmt_class_vtable, 1, className.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_class_vtable);
    if (rc != SQLITE_ROW) {
        return nullptr;
    }

    // A row with a NULL vtable is legitimate (class has no vtable recorded).
    if (sqlite3_column_type(stmt_class_vtable, 0) == SQLITE_NULL) {
        return nullptr;
    }

    sqlite3_int64 address = sqlite3_column_int64(stmt_class_vtable, 0);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(address));
}

bool GameVersion::HasFunction(const std::string& className, const std::string& functionName) {
    if (!initialized) {
        return false;
    }

    // Bind parameters
    sqlite3_reset(stmt_function);
    sqlite3_bind_text(stmt_function, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_function, 2, functionName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_function);
    return (rc == SQLITE_ROW);
}

bool GameVersion::HasOffset(const std::string& className, const std::string& propertyName) {
    if (!initialized) {
        return false;
    }

    // Bind parameters
    sqlite3_reset(stmt_offset);
    sqlite3_bind_text(stmt_offset, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_offset, 2, propertyName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_offset);
    return (rc == SQLITE_ROW);
}

bool GameVersion::HasClass(const std::string& className) {
    if (!initialized || !stmt_class_size) {
        return false;
    }

    // Bind parameter
    sqlite3_reset(stmt_class_size);
    sqlite3_bind_text(stmt_class_size, 1, className.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_class_size);
    return (rc == SQLITE_ROW);
}

void GameVersion::Reset(bool force) {
    // Quick return if already reset
    if (!initialized && !db && !stmt_function && !stmt_pointer && !stmt_offset && !stmt_class_size && !stmt_class_vtable && !force) {
        return;
    }

    initialized = false;
    versionSha.clear();
    gameTitle = GameTitle::Unknown;
    gamePlatform = GamePlatform::Unknown;

    FinalizeStatements();

    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}
