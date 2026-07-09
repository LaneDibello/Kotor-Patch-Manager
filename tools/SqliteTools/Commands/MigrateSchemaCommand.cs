using Microsoft.Data.Sqlite;

namespace SqliteTools.Commands;

public class MigrateSchemaCommand : ICommand
{
    public void Execute(string[] args)
    {
        var dbPath = GetArgument(args, "--database");

        if (string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools migrate --database <database.db>");
            Environment.Exit(1);
        }

        if (!File.Exists(dbPath))
        {
            throw new FileNotFoundException($"Database file not found: {dbPath}");
        }

        using var connection = new SqliteConnection($"Data Source={dbPath}");
        connection.Open();

        int currentVersion = GetCurrentSchemaVersion(connection);
        Console.WriteLine($"Current schema version: {currentVersion}");

        // Apply migrations in sequence
        if (currentVersion < 2)
        {
            ApplyMigration(connection, "002_add_function_metadata.sql");
            Console.WriteLine("Applied migration 002: Add function metadata columns");
        }

        if (currentVersion < 3)
        {
            ApplyMigration(connection, "003_add_classes_table.sql");
            Console.WriteLine("Applied migration 003: Add classes table");
        }

        if (currentVersion < 4)
        {
            ApplyMigration(connection, "004_add_platform.sql");
            Console.WriteLine("Applied migration 004: Add platform column");
        }

        if (currentVersion < 5)
        {
            ApplyMigration(connection, "005_add_class_vtable.sql");
            Console.WriteLine("Applied migration 005: Add class vtable column");
        }

        // Future migrations would go here
        // if (currentVersion < 6) { ApplyMigration(connection, "006_..."); }

        int newVersion = GetCurrentSchemaVersion(connection);
        Console.WriteLine($"Schema migration complete. Current version: {newVersion}");
    }

    private int GetCurrentSchemaVersion(SqliteConnection connection)
    {
        var cmd = connection.CreateCommand();
        cmd.CommandText = "SELECT MAX(version) FROM schema_version";
        var result = cmd.ExecuteScalar();
        return result != null && result != DBNull.Value ? Convert.ToInt32(result) : 1;
    }

    private void ApplyMigration(SqliteConnection connection, string migrationFile)
    {
        // Look for migration file in Migrations directory relative to executable
        string exeDir = AppDomain.CurrentDomain.BaseDirectory;
        string migrationPath = Path.Combine(exeDir, "Migrations", migrationFile);

        if (!File.Exists(migrationPath))
        {
            throw new FileNotFoundException($"Migration file not found: {migrationPath}");
        }

        var sql = File.ReadAllText(migrationPath);

        using var transaction = connection.BeginTransaction();
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = sql;
            cmd.ExecuteNonQuery();
            transaction.Commit();
        }
        catch
        {
            transaction.Rollback();
            throw;
        }
    }

    private string GetArgument(string[] args, string key)
    {
        for (int i = 0; i < args.Length - 1; i++)
        {
            if (args[i] == key)
            {
                return args[i + 1];
            }
        }
        return string.Empty;
    }
}
