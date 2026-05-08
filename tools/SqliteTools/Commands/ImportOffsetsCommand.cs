using Microsoft.Data.Sqlite;
using SqliteTools.Models;

namespace SqliteTools.Commands;

public class ImportOffsetsCommand : ICommand
{
    public void Execute(string[] args)
    {
        var csvPath = GetArgument(args, "--csv");
        var dbPath = GetArgument(args, "--database");
        var mode = GetArgument(args, "--mode", "append"); // append or replace

        if (string.IsNullOrEmpty(csvPath) || string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools import-offsets --csv <file.csv> --database <database.db> [--mode append|replace]");
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
            deleteCmd.CommandText = "DELETE FROM offsets";
            int deletedCount = deleteCmd.ExecuteNonQuery();
            Console.WriteLine($"Deleted {deletedCount} existing offsets (replace mode)");
        }

        int importedCount = 0;
        int updatedCount = 0;

        foreach (var record in records)
        {
            bool wasUpdate = InsertOrUpdateOffset(connection, record);
            if (wasUpdate)
                updatedCount++;
            else
                importedCount++;
        }

        transaction.Commit();
        Console.WriteLine($"Import complete: {importedCount} new offsets, {updatedCount} updated");
    }

    private List<GhidraOffset> ParseCsv(string csvPath)
    {
        var records = new List<GhidraOffset>();
        var lines = File.ReadAllLines(csvPath);

        if (lines.Length == 0)
        {
            throw new InvalidDataException("CSV file is empty");
        }

        // Parse header
        var header = ParseCsvLine(lines[0]);
        int classIdx = header.IndexOf("class_name");
        int memberIdx = header.IndexOf("member_name");
        int offsetIdx = header.IndexOf("offset");
        int notesIdx = header.IndexOf("notes");

        if (classIdx == -1 || memberIdx == -1 || offsetIdx == -1)
        {
            throw new InvalidDataException("CSV must have class_name, member_name, and offset columns");
        }

        // Parse data rows
        for (int i = 1; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            if (string.IsNullOrEmpty(line))
                continue;

            var parts = ParseCsvLine(line);

            if (parts.Count < 3)
                continue;

            if (!int.TryParse(parts[offsetIdx], out int offsetValue))
            {
                Console.Error.WriteLine($"Warning: Skipping line {i + 1}, invalid offset value: {parts[offsetIdx]}");
                continue;
            }

            var offset = new GhidraOffset
            {
                ClassName = parts[classIdx],
                MemberName = parts[memberIdx],
                Offset = offsetValue,
                Notes = notesIdx >= 0 && notesIdx < parts.Count ? parts[notesIdx] : null
            };

            records.Add(offset);
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

    private bool InsertOrUpdateOffset(SqliteConnection conn, GhidraOffset offset)
    {
        // Check if offset already exists
        var checkCmd = conn.CreateCommand();
        checkCmd.CommandText = "SELECT COUNT(*) FROM offsets WHERE class_name = @class AND member_name = @member";
        checkCmd.Parameters.AddWithValue("@class", offset.ClassName);
        checkCmd.Parameters.AddWithValue("@member", offset.MemberName);
        bool exists = Convert.ToInt32(checkCmd.ExecuteScalar()) > 0;

        var cmd = conn.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO offsets (class_name, member_name, offset, notes)
            VALUES (@class, @member, @offset, @notes)
            ON CONFLICT(class_name, member_name) DO UPDATE SET
                offset = excluded.offset,
                notes = excluded.notes";

        cmd.Parameters.AddWithValue("@class", offset.ClassName);
        cmd.Parameters.AddWithValue("@member", offset.MemberName);
        cmd.Parameters.AddWithValue("@offset", offset.Offset);
        cmd.Parameters.AddWithValue("@notes", offset.Notes ?? (object)DBNull.Value);

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
