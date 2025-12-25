using Microsoft.Data.Sqlite;
using Tomlyn;
using Tomlyn.Model;

namespace SqliteTools.Commands;

public class TomlToSqliteCommand : ICommand
{
    public void Execute(string[] args)
    {
        if (args.Length != 2)
        {
            Console.Error.WriteLine("Usage: SqliteTools toml-to-sqlite <input.toml> <output.db>");
            Environment.Exit(1);
        }

        string tomlPath = args[0];
        string dbPath = args[1];

        if (!File.Exists(tomlPath))
        {
            throw new FileNotFoundException($"Input file not found: {tomlPath}");
        }

        // Delete existing database
        if (File.Exists(dbPath))
        {
            File.Delete(dbPath);
        }

        // Parse TOML
        var tomlContent = File.ReadAllText(tomlPath);
        var tomlTable = Toml.ToModel(tomlContent);

        // Create SQLite database
        using var connection = new SqliteConnection($"Data Source={dbPath}");
        connection.Open();

        // Create schema
        CreateSchema(connection);

        // Insert game version metadata
        InsertGameVersion(connection, tomlTable, Path.GetFileNameWithoutExtension(tomlPath));

        // Insert data
        InsertGlobalPointers(connection, tomlTable);
        InsertFunctions(connection, tomlTable);
        InsertOffsets(connection, tomlTable);

        Console.WriteLine($"Successfully converted {tomlPath} to {dbPath}");
    }

    private static void CreateSchema(SqliteConnection connection)
    {
        string schema = @"
            CREATE TABLE schema_version (
                version INTEGER PRIMARY KEY,
                applied_date TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            );
            INSERT INTO schema_version (version) VALUES (1);

            CREATE TABLE game_version (
                id INTEGER PRIMARY KEY CHECK (id = 1),
                sha256_hash TEXT NOT NULL UNIQUE,
                game_name TEXT NOT NULL,
                version_string TEXT NOT NULL,
                description TEXT
            );

            CREATE TABLE functions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                class_name TEXT NOT NULL,
                function_name TEXT NOT NULL,
                address INTEGER NOT NULL,
                notes TEXT,
                UNIQUE(class_name, function_name)
            );

            CREATE TABLE global_pointers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                pointer_name TEXT NOT NULL UNIQUE,
                address INTEGER NOT NULL,
                notes TEXT
            );

            CREATE TABLE offsets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                class_name TEXT NOT NULL,
                member_name TEXT NOT NULL,
                offset INTEGER NOT NULL,
                notes TEXT,
                UNIQUE(class_name, member_name)
            );

            CREATE INDEX idx_functions_class ON functions(class_name);
            CREATE INDEX idx_functions_name ON functions(function_name);
            CREATE INDEX idx_offsets_class ON offsets(class_name);
            CREATE INDEX idx_offsets_member ON offsets(member_name);
        ";

        using var cmd = connection.CreateCommand();
        cmd.CommandText = schema;
        cmd.ExecuteNonQuery();
    }

    private static void InsertGameVersion(SqliteConnection connection, TomlTable tomlTable, string fileName)
    {
        // Get SHA256 from top-level "versions_sha" key
        string sha256 = (string)tomlTable["versions_sha"];

        // Parse game name and version from filename (e.g., "kotor1_gog_103")
        var parts = fileName.Split('_');
        string gameName = parts[0].ToUpper();
        string versionString = string.Join(" ", parts.Skip(1));

        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO game_version (id, sha256_hash, game_name, version_string)
            VALUES (1, @sha256, @game, @version)
        ";
        cmd.Parameters.AddWithValue("@sha256", sha256);
        cmd.Parameters.AddWithValue("@game", gameName);
        cmd.Parameters.AddWithValue("@version", versionString);
        cmd.ExecuteNonQuery();
    }

    private static void InsertGlobalPointers(SqliteConnection connection, TomlTable tomlTable)
    {
        if (!tomlTable.ContainsKey("global_pointers")) return;

        var pointers = (TomlTable)tomlTable["global_pointers"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO global_pointers (pointer_name, address)
            VALUES (@name, @address)
        ";

        foreach (var kvp in pointers)
        {
            long address = ParseAddress(kvp.Value);
            cmd.Parameters.Clear();
            cmd.Parameters.AddWithValue("@name", kvp.Key);
            cmd.Parameters.AddWithValue("@address", address);
            cmd.ExecuteNonQuery();
        }
    }

    private static void InsertFunctions(SqliteConnection connection, TomlTable tomlTable)
    {
        if (!tomlTable.ContainsKey("functions")) return;

        var functionsTable = (TomlTable)tomlTable["functions"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO functions (class_name, function_name, address)
            VALUES (@class, @function, @address)
        ";

        foreach (var classKvp in functionsTable)
        {
            string className = classKvp.Key;
            var classTable = (TomlTable)classKvp.Value;

            foreach (var funcKvp in classTable)
            {
                string functionName = funcKvp.Key;
                long address = ParseAddress(funcKvp.Value);

                cmd.Parameters.Clear();
                cmd.Parameters.AddWithValue("@class", className);
                cmd.Parameters.AddWithValue("@function", functionName);
                cmd.Parameters.AddWithValue("@address", address);
                cmd.ExecuteNonQuery();
            }
        }
    }

    private static void InsertOffsets(SqliteConnection connection, TomlTable tomlTable)
    {
        if (!tomlTable.ContainsKey("offsets")) return;

        var offsetsTable = (TomlTable)tomlTable["offsets"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO offsets (class_name, member_name, offset)
            VALUES (@class, @member, @offset)
        ";

        foreach (var classKvp in offsetsTable)
        {
            string className = classKvp.Key;
            var classTable = (TomlTable)classKvp.Value;

            foreach (var memberKvp in classTable)
            {
                string memberName = memberKvp.Key;
                long offset = Convert.ToInt64(memberKvp.Value);

                cmd.Parameters.Clear();
                cmd.Parameters.AddWithValue("@class", className);
                cmd.Parameters.AddWithValue("@member", memberName);
                cmd.Parameters.AddWithValue("@offset", offset);
                cmd.ExecuteNonQuery();
            }
        }
    }

    private static long ParseAddress(object value)
    {
        string str = value.ToString() ?? "";
        if (str.StartsWith("0x"))
        {
            return Convert.ToInt64(str.Substring(2), 16);
        }
        return Convert.ToInt64(str);
    }
}
