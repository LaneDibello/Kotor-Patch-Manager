#include "GameVersion.h"
#include <windows.h>
#include <sqlite3.h>
#include <sstream>

bool GameVersion::initialized = false;
std::string GameVersion::versionSha;
sqlite3* GameVersion::db = nullptr;
sqlite3_stmt* GameVersion::stmt_function = nullptr;
sqlite3_stmt* GameVersion::stmt_pointer = nullptr;
sqlite3_stmt* GameVersion::stmt_offset = nullptr;

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

    // Verify game version SHA matches
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, "SELECT sha256_hash FROM game_version", -1, &stmt, nullptr);
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

void GameVersion::Reset(bool force) {
    // Quick return if already reset
    if (!initialized && !db && !stmt_function && !stmt_pointer && !stmt_offset && !force) {
        return;
    }

    initialized = false;
    versionSha.clear();

    FinalizeStatements();

    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}
