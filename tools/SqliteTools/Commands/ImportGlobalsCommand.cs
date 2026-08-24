using Microsoft.Data.Sqlite;
using SqliteTools.Models;

namespace SqliteTools.Commands;

public class ImportGlobalsCommand : ICommand
{
    public void Execute(string[] args)
    {
        var csvPath = GetArgument(args, "--csv");
        var dbPath = GetArgument(args, "--database");
        var mode = GetArgument(args, "--mode", "append"); // append or replace

        if (string.IsNullOrEmpty(csvPath) || string.IsNullOrEmpty(dbPath))
        {
            Console.Error.WriteLine("Usage: SqliteTools import-globals --csv <file.csv> --database <database.db> [--mode append|replace]");
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
            deleteCmd.CommandText = "DELETE FROM global_pointers";
            int deletedCount = deleteCmd.ExecuteNonQuery();
            Console.WriteLine($"Deleted {deletedCount} existing global pointers (replace mode)");
        }

        int importedCount = 0;
        int updatedCount = 0;

        foreach (var record in records)
        {
            bool wasUpdate = InsertOrUpdateGlobal(connection, record);
            if (wasUpdate)
                updatedCount++;
            else
                importedCount++;
        }

        transaction.Commit();
        Console.WriteLine($"Import complete: {importedCount} new global pointers, {updatedCount} updated");
    }

    private List<GlobalPointer> ParseCsv(string csvPath)
    {
        var records = new List<GlobalPointer>();
        var lines = File.ReadAllLines(csvPath);

        if (lines.Length == 0)
        {
            throw new InvalidDataException("CSV file is empty");
        }

        var header = ParseCsvLine(lines[0]);
        int nameIdx = header.IndexOf("pointer_name");
        int addrIdx = header.IndexOf("address");
        int notesIdx = header.IndexOf("notes");

        if (nameIdx == -1 || addrIdx == -1)
        {
            throw new InvalidDataException("CSV must have pointer_name and address columns");
        }

        for (int i = 1; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            if (string.IsNullOrEmpty(line))
                continue;

            var parts = ParseCsvLine(line);

            if (parts.Count < 2)
                continue;

            if (string.IsNullOrEmpty(parts[nameIdx]))
            {
                Console.Error.WriteLine($"Warning: Skipping line {i + 1}, empty pointer_name");
                continue;
            }

            // Checked now so the warning can name the line it came from.
            if (!TryParseAddress(parts[addrIdx], out _))
            {
                Console.Error.WriteLine($"Warning: Skipping line {i + 1}, invalid address value: {parts[addrIdx]}");
                continue;
            }

            records.Add(new GlobalPointer
            {
                PointerName = parts[nameIdx],
                Address = parts[addrIdx],
                Notes = notesIdx >= 0 && notesIdx < parts.Count && !string.IsNullOrEmpty(parts[notesIdx])
                    ? parts[notesIdx]
                    : null
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

    private bool InsertOrUpdateGlobal(SqliteConnection conn, GlobalPointer pointer)
    {
        var checkCmd = conn.CreateCommand();
        checkCmd.CommandText = "SELECT COUNT(*) FROM global_pointers WHERE pointer_name = @name";
        checkCmd.Parameters.AddWithValue("@name", pointer.PointerName);
        bool exists = Convert.ToInt32(checkCmd.ExecuteScalar()) > 0;

        TryParseAddress(pointer.Address, out long address);

        var cmd = conn.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO global_pointers (pointer_name, address, notes)
            VALUES (@name, @address, @notes)
            ON CONFLICT(pointer_name) DO UPDATE SET
                address = excluded.address,
                notes = excluded.notes";

        cmd.Parameters.AddWithValue("@name", pointer.PointerName);
        cmd.Parameters.AddWithValue("@address", address);
        cmd.Parameters.AddWithValue("@notes", pointer.Notes ?? (object)DBNull.Value);

        cmd.ExecuteNonQuery();
        return exists;
    }

    // Hex with an 0x prefix, or plain decimal.
    private bool TryParseAddress(string address, out long value)
    {
        value = 0;
        if (string.IsNullOrEmpty(address))
            return false;

        try
        {
            value = address.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? Convert.ToInt64(address.Substring(2), 16)
                : Convert.ToInt64(address);
            return true;
        }
        catch (Exception e) when (e is FormatException || e is OverflowException || e is ArgumentException)
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
