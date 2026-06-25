using Microsoft.Data.Sqlite;
using SqliteTools.Models;

namespace SqliteTools.Commands;

public class ImportClassesCommand : ICommand
{
    public void Execute(string[] args)
    {
        var csvPath = GetArgument(args, "--csv");
        var dbPath = GetArgument(args, "--database");
        var mode = GetArgument(args, "--mode", "append"); // append or replace

        if (string.IsNullOrEmpty(csvPath) || string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools import-classes --csv <file.csv> --database <database.db> [--mode append|replace]");
            Environment.Exit(1);
        }

        if (!File.Exists(csvPath))
        {
            throw new FileNotFoundException($"CSV file not found: {csvPath}");
        }

        if (!File.Exists(dbPath))
        {
            throw new FileNotFoundException($"Database file not found: {dbPath}");
        }

        using var connection = new SqliteConnection($"Data Source={dbPath}");
        connection.Open();

        var records = ParseCsv(csvPath);

        using var transaction = connection.BeginTransaction();

        if (mode == "replace")
        {
            var deleteCmd = connection.CreateCommand();
            deleteCmd.CommandText = "DELETE FROM classes";
            int deletedCount = deleteCmd.ExecuteNonQuery();
            Console.WriteLine($"Deleted {deletedCount} existing classes (replace mode)");
        }

        int importedCount = 0;
        int updatedCount = 0;

        foreach (var record in records)
        {
            bool wasUpdate = InsertOrUpdateClass(connection, record);
            if (wasUpdate)
                updatedCount++;
            else
                importedCount++;
        }

        transaction.Commit();
        Console.WriteLine($"Import complete: {importedCount} new classes, {updatedCount} updated");
    }

    private List<GameClass> ParseCsv(string csvPath)
    {
        var records = new List<GameClass>();
        var lines = File.ReadAllLines(csvPath);

        if (lines.Length == 0)
        {
            throw new InvalidDataException("CSV file is empty");
        }

        // Parse header
        var header = ParseCsvLine(lines[0]);
        int classIdx = header.IndexOf("class_name");
        int sizeIdx = header.IndexOf("size");
        int notesIdx = header.IndexOf("notes");

        if (classIdx == -1 || sizeIdx == -1)
        {
            throw new InvalidDataException("CSV must have class_name and size columns");
        }

        // Parse data rows
        for (int i = 1; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            if (string.IsNullOrEmpty(line))
                continue;

            var parts = ParseCsvLine(line);

            if (parts.Count < 2)
                continue;

            if (!int.TryParse(parts[sizeIdx], out int sizeValue))
            {
                Console.Error.WriteLine($"Warning: Skipping line {i + 1}, invalid size value: {parts[sizeIdx]}");
                continue;
            }

            var gameClass = new GameClass
            {
                ClassName = parts[classIdx],
                Size = sizeValue,
                Notes = notesIdx >= 0 && notesIdx < parts.Count ? parts[notesIdx] : null
            };

            records.Add(gameClass);
        }

        return records;
    }

    private List<string> ParseCsvLine(string line)
    {
        var result = new List<string>();
        var current = new System.Text.StringBuilder();
        bool inQuotes = false;

        for (int i = 0; i < line.Length; i++)
        {
            char c = line[i];

            if (c == '"')
            {
                inQuotes = !inQuotes;
            }
            else if (c == ',' && !inQuotes)
            {
                result.Add(current.ToString().Trim());
                current.Clear();
            }
            else
            {
                current.Append(c);
            }
        }

        result.Add(current.ToString().Trim());
        return result;
    }

    private bool InsertOrUpdateClass(SqliteConnection conn, GameClass gameClass)
    {
        // Check if class already exists
        var checkCmd = conn.CreateCommand();
        checkCmd.CommandText = "SELECT COUNT(*) FROM classes WHERE class_name = @class";
        checkCmd.Parameters.AddWithValue("@class", gameClass.ClassName);
        bool exists = Convert.ToInt32(checkCmd.ExecuteScalar()) > 0;

        var cmd = conn.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO classes (class_name, size, notes)
            VALUES (@class, @size, @notes)
            ON CONFLICT(class_name) DO UPDATE SET
                size = excluded.size,
                notes = excluded.notes";

        cmd.Parameters.AddWithValue("@class", gameClass.ClassName);
        cmd.Parameters.AddWithValue("@size", gameClass.Size);
        cmd.Parameters.AddWithValue("@notes", gameClass.Notes ?? (object)DBNull.Value);

        cmd.ExecuteNonQuery();
        return exists;
    }

    private string GetArgument(string[] args, string key, string defaultValue = "")
    {
        for (int i = 0; i < args.Length - 1; i++)
        {
            if (args[i] == key)
            {
                return args[i + 1];
            }
        }
        return defaultValue;
    }
}
