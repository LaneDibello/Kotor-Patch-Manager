using SqliteTools.Commands;

class Program
{
    static void Main(string[] args)
    {
        if (args.Length == 0)
        {
            ShowHelp();
            return;
        }

        var command = args[0].ToLower();
        var commandArgs = args.Skip(1).ToArray();

        try
        {
            switch (command)
            {
                case "toml-to-sqlite":
                    new TomlToSqliteCommand().Execute(commandArgs);
                    break;
                case "import-ghidra":
                    new ImportGhidraCommand().Execute(commandArgs);
                    break;
                case "import-offsets":
                    new ImportOffsetsCommand().Execute(commandArgs);
                    break;
                case "migrate":
                    new MigrateSchemaCommand().Execute(commandArgs);
                    break;
                case "validate":
                    new ValidateCommand().Execute(commandArgs);
                    break;
                default:
                    Console.Error.WriteLine($"Unknown command: {command}");
                    ShowHelp();
                    Environment.Exit(1);
                    break;
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error: {ex.Message}");
            if (args.Contains("--verbose"))
            {
                Console.Error.WriteLine(ex.StackTrace);
            }
            Environment.Exit(1);
        }
    }

    static void ShowHelp()
    {
        Console.WriteLine("SqliteTools - Database management utility for KotOR Patch Manager");
        Console.WriteLine();
        Console.WriteLine("Usage: SqliteTools <command> [options]");
        Console.WriteLine();
        Console.WriteLine("Commands:");
        Console.WriteLine("  toml-to-sqlite <input.toml> <output.db>");
        Console.WriteLine("      Convert TOML address database to SQLite format");
        Console.WriteLine();
        Console.WriteLine("  import-ghidra --csv <file.csv> --database <database.db> [--mode append|replace]");
        Console.WriteLine("      Import Ghidra function export into SQLite database");
        Console.WriteLine("      Modes: append (update/insert), replace (delete all first)");
        Console.WriteLine();
        Console.WriteLine("  import-offsets --csv <file.csv> --database <database.db> [--mode append|replace]");
        Console.WriteLine("      Import class member offsets from CSV into SQLite database");
        Console.WriteLine("      CSV columns: class_name, member_name, offset, notes");
        Console.WriteLine("      Modes: append (update/insert), replace (delete all first)");
        Console.WriteLine();
        Console.WriteLine("  migrate --database <database.db>");
        Console.WriteLine("      Run schema migrations on database");
        Console.WriteLine();
        Console.WriteLine("  validate --database <database.db>");
        Console.WriteLine("      Validate database schema and integrity");
        Console.WriteLine();
        Console.WriteLine("Options:");
        Console.WriteLine("  --verbose    Show detailed error information");
    }
}
