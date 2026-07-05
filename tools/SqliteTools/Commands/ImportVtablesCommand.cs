using Microsoft.Data.Sqlite;
using SqliteTools.Models;

namespace SqliteTools.Commands;

public class ImportVtablesCommand : ICommand
{
    public void Execute(string[] args)
    {
        var csvPath = GetArgument(args, "--csv");
        var dbPath = GetArgument(args, "--database");
        var mode = GetArgument(args, "--mode", "append"); // append or replace

        if (string.IsNullOrEmpty(csvPath) || string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools import-vtables --csv <file.csv> --database <database.db> [--mode append|replace]");
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

        // Check the schema has been migrated to v5 (classes.vtable column exists)
        if (!HasVtableColumn(connection))
        {
            Console.Error.WriteLine("Error: Database schema needs migration to v5 before importing vtables.");
            Console.Error.WriteLine("Run: SqliteTools migrate --database " + dbPath);
            Environment.Exit(1);
        }

        var records = ParseCsv(csvPath);

        using var transaction = connection.BeginTransaction();

        if (mode == "replace")
        {
            // Clear the vtable column only; leave size/notes intact.
            var clearCmd = connection.CreateCommand();
            clearCmd.CommandText = "UPDATE classes SET vtable = NULL";
            int clearedCount = clearCmd.ExecuteNonQuery();
            Console.WriteLine($"Cleared vtable on {clearedCount} existing classes (replace mode)");
        }

        int importedCount = 0;
        int updatedCount = 0;

        foreach (var record in records)
        {
            bool wasUpdate = InsertOrUpdateVtable(connection, record);
            if (wasUpdate)
                updatedCount++;
            else
                importedCount++;
        }

        transaction.Commit();
        Console.WriteLine($"Import complete: {importedCount} new classes, {updatedCount} updated");
    }

    private bool HasVtableColumn(SqliteConnection connection)
    {
        var cmd = connection.CreateCommand();
        cmd.CommandText = "PRAGMA table_info(classes)";
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            string columnName = reader.GetString(1);
            if (columnName == "vtable")
                return true;
        }
        return false;
    }

    private List<ClassVtable> ParseCsv(string csvPath)
    {
        var records = new List<ClassVtable>();
        var lines = File.ReadAllLines(csvPath);

        if (lines.Length == 0)
        {
            throw new InvalidDataException("CSV file is empty");
        }

        // Parse header
        var header = ParseCsvLine(lines[0]);
        int classIdx = header.IndexOf("class_name");
        int vtableIdx = header.IndexOf("vtable_address");

        if (classIdx == -1 || vtableIdx == -1)
        {
            throw new InvalidDataException("CSV must have class_name and vtable_address columns");
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

            if (!TryParseHexAddress(parts[vtableIdx], out long vtableValue))
            {
                Console.Error.WriteLine($"Warning: Skipping line {i + 1}, invalid vtable address: {parts[vtableIdx]}");
                continue;
            }

            records.Add(new ClassVtable
            {
                ClassName = parts[classIdx],
                Vtable = vtableValue
            });
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

    private bool InsertOrUpdateVtable(SqliteConnection conn, ClassVtable record)
    {
        // Check if class already exists
        var checkCmd = conn.CreateCommand();
        checkCmd.CommandText = "SELECT COUNT(*) FROM classes WHERE class_name = @class";
        checkCmd.Parameters.AddWithValue("@class", record.ClassName);
        bool exists = Convert.ToInt32(checkCmd.ExecuteScalar()) > 0;

        // Update only the vtable column; a brand-new row gets NULL size/notes.
        var cmd = conn.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO classes (class_name, vtable)
            VALUES (@class, @vtable)
            ON CONFLICT(class_name) DO UPDATE SET
                vtable = excluded.vtable";

        cmd.Parameters.AddWithValue("@class", record.ClassName);
        cmd.Parameters.AddWithValue("@vtable", record.Vtable);

        cmd.ExecuteNonQuery();
        return exists;
    }

    // Vtable addresses are hex, with or without a leading 0x (e.g. "00752e30").
    private bool TryParseHexAddress(string address, out long value)
    {
        value = 0;
        string addr = address.Trim();
        if (string.IsNullOrEmpty(addr))
            return false;

        if (addr.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
        {
            addr = addr.Substring(2);
        }

        try
        {
            value = Convert.ToInt64(addr, 16);
            return true;
        }
        catch
        {
            return false;
        }
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
