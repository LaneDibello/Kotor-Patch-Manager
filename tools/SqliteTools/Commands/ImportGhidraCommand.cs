using Microsoft.Data.Sqlite;
using SqliteTools.Models;
using System.Globalization;

namespace SqliteTools.Commands;

public class ImportGhidraCommand : ICommand
{
    public void Execute(string[] args)
    {
        var csvPath = GetArgument(args, "--csv");
        var dbPath = GetArgument(args, "--database");
        var mode = GetArgument(args, "--mode", "append"); // append or replace

        if (string.IsNullOrEmpty(csvPath) || string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools import-ghidra --csv <file.csv> --database <database.db> [--mode append|replace]");
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

        // Check if schema has been migrated to v2 (has calling_convention column)
        if (!HasCallingConventionColumn(connection))
        {
            Console.Error.WriteLine("Error: Database schema needs migration to v2 before importing Ghidra data.");
            Console.Error.WriteLine("Run: SqliteTools migrate --database " + dbPath);
            Environment.Exit(1);
        }

        var records = ParseCsv(csvPath);

        using var transaction = connection.BeginTransaction();

        if (mode == "replace")
        {
            // Delete all existing functions
            var deleteCmd = connection.CreateCommand();
            deleteCmd.CommandText = "DELETE FROM functions";
            int deletedCount = deleteCmd.ExecuteNonQuery();
            Console.WriteLine($"Deleted {deletedCount} existing functions (replace mode)");
        }

        int importedCount = 0;
        int updatedCount = 0;

        foreach (var record in records)
        {
            bool wasUpdate = InsertOrUpdateFunction(connection, record);
            if (wasUpdate)
                updatedCount++;
            else
                importedCount++;
        }

        transaction.Commit();
        Console.WriteLine($"Import complete: {importedCount} new functions, {updatedCount} updated");
    }

    private bool HasCallingConventionColumn(SqliteConnection connection)
    {
        var cmd = connection.CreateCommand();
        cmd.CommandText = "PRAGMA table_info(functions)";
        using var reader = cmd.ExecuteReader();
        while (reader.Read())
        {
            string columnName = reader.GetString(1);
            if (columnName == "calling_convention")
                return true;
        }
        return false;
    }

    private List<GhidraFunction> ParseCsv(string csvPath)
    {
        var records = new List<GhidraFunction>();
        var lines = File.ReadAllLines(csvPath);

        if (lines.Length == 0)
        {
            throw new InvalidDataException("CSV file is empty");
        }

        // Parse header
        var header = lines[0].Split(',');
        int classIdx = Array.IndexOf(header, "class_name");
        int funcIdx = Array.IndexOf(header, "function_name");
        int addrIdx = Array.IndexOf(header, "address");
        int convIdx = Array.IndexOf(header, "calling_convention");
        int sizeIdx = Array.IndexOf(header, "param_size_bytes");
        int notesIdx = Array.IndexOf(header, "notes");

        if (classIdx == -1 || funcIdx == -1 || addrIdx == -1)
        {
            throw new InvalidDataException("CSV must have class_name, function_name, and address columns");
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

            var func = new GhidraFunction
            {
                ClassName = parts[classIdx],
                FunctionName = parts[funcIdx],
                Address = parts[addrIdx],
                CallingConvention = convIdx >= 0 && convIdx < parts.Count ? parts[convIdx] : null,
                ParamSizeBytes = sizeIdx >= 0 && sizeIdx < parts.Count && int.TryParse(parts[sizeIdx], out int size) ? size : null,
                Notes = notesIdx >= 0 && notesIdx < parts.Count ? parts[notesIdx] : null
            };

            records.Add(func);
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

    private bool InsertOrUpdateFunction(SqliteConnection conn, GhidraFunction func)
    {
        // Check if function already exists
        var checkCmd = conn.CreateCommand();
        checkCmd.CommandText = "SELECT COUNT(*) FROM functions WHERE class_name = @class AND function_name = @func";
        checkCmd.Parameters.AddWithValue("@class", func.ClassName);
        checkCmd.Parameters.AddWithValue("@func", func.FunctionName);
        bool exists = Convert.ToInt32(checkCmd.ExecuteScalar()) > 0;

        var cmd = conn.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO functions (class_name, function_name, address, calling_convention, param_size_bytes, notes)
            VALUES (@class, @func, @addr, @conv, @size, @notes)
            ON CONFLICT(class_name, function_name) DO UPDATE SET
                address = excluded.address,
                calling_convention = excluded.calling_convention,
                param_size_bytes = excluded.param_size_bytes,
                notes = excluded.notes";

        cmd.Parameters.AddWithValue("@class", func.ClassName);
        cmd.Parameters.AddWithValue("@func", func.FunctionName);
        cmd.Parameters.AddWithValue("@addr", ParseHexAddress(func.Address));
        cmd.Parameters.AddWithValue("@conv", func.CallingConvention ?? (object)DBNull.Value);
        cmd.Parameters.AddWithValue("@size", func.ParamSizeBytes ?? (object)DBNull.Value);
        cmd.Parameters.AddWithValue("@notes", func.Notes ?? (object)DBNull.Value);

        cmd.ExecuteNonQuery();
        return exists;
    }

    private long ParseHexAddress(string address)
    {
        string addr = address.Trim();
        if (addr.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
        {
            return Convert.ToInt64(addr.Substring(2), 16);
        }
        return Convert.ToInt64(addr);
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
