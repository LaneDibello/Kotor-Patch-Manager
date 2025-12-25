using Microsoft.Data.Sqlite;

namespace SqliteTools.Commands;

public class ValidateCommand : ICommand
{
    public void Execute(string[] args)
    {
        var dbPath = GetArgument(args, "--database");

        if (string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools validate --database <database.db>");
            Environment.Exit(1);
        }

        if (!File.Exists(dbPath))
        {
            throw new FileNotFoundException($"Database file not found: {dbPath}");
        }

        using var connection = new SqliteConnection($"Data Source={dbPath}");
        connection.Open();

        Console.WriteLine($"Validating database: {dbPath}");
        Console.WriteLine();

        bool isValid = true;

        // Check schema version
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = "SELECT MAX(version) FROM schema_version";
            var version = cmd.ExecuteScalar();
            Console.WriteLine($"✓ Schema version: {version}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Schema version table error: {ex.Message}");
            isValid = false;
        }

        // Check game version
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = "SELECT game_name, version_string, sha256_hash FROM game_version";
            using var reader = cmd.ExecuteReader();
            int count = 0;
            while (reader.Read())
            {
                count++;
                string gameName = reader.GetString(0);
                string versionString = reader.GetString(1);
                string sha = reader.GetString(2);
                Console.WriteLine($"✓ Game version: {gameName} {versionString}");
                Console.WriteLine($"  SHA256: {sha.Substring(0, 16)}...");
            }

            if (count == 0)
            {
                Console.WriteLine("✗ No game version found");
                isValid = false;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Game version table error: {ex.Message}");
            isValid = false;
        }

        // Check functions table
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = "SELECT COUNT(*) FROM functions";
            int funcCount = Convert.ToInt32(cmd.ExecuteScalar());
            Console.WriteLine($"✓ Functions table: {funcCount} entries");

            // Check for duplicates
            cmd.CommandText = @"
                SELECT class_name, function_name, COUNT(*) as count
                FROM functions
                GROUP BY class_name, function_name
                HAVING count > 1";
            using var reader = cmd.ExecuteReader();
            if (reader.Read())
            {
                Console.WriteLine("✗ Duplicate functions found:");
                do
                {
                    Console.WriteLine($"  {reader.GetString(0)}::{reader.GetString(1)} ({reader.GetInt32(2)} duplicates)");
                    isValid = false;
                } while (reader.Read());
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Functions table error: {ex.Message}");
            isValid = false;
        }

        // Check global pointers table
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = "SELECT COUNT(*) FROM global_pointers";
            int ptrCount = Convert.ToInt32(cmd.ExecuteScalar());
            Console.WriteLine($"✓ Global pointers table: {ptrCount} entries");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Global pointers table error: {ex.Message}");
            isValid = false;
        }

        // Check offsets table
        try
        {
            var cmd = connection.CreateCommand();
            cmd.CommandText = "SELECT COUNT(*) FROM offsets";
            int offsetCount = Convert.ToInt32(cmd.ExecuteScalar());
            Console.WriteLine($"✓ Offsets table: {offsetCount} entries");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Offsets table error: {ex.Message}");
            isValid = false;
        }

        Console.WriteLine();
        if (isValid)
        {
            Console.WriteLine("Database validation passed.");
        }
        else
        {
            Console.WriteLine("Database validation FAILED.");
            Environment.Exit(1);
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
