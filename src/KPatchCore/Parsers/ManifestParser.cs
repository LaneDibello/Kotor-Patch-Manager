using KPatchCore.Models;
using Tomlyn;
using Tomlyn.Model;

namespace KPatchCore.Parsers;

/// <summary>
/// Parses manifest.toml files from .kpatch archives
/// </summary>
public static class ManifestParser
{
    /// <summary>
    /// Parses a manifest.toml file and returns a PatchManifest
    /// </summary>
    /// <param name="manifestPath">Path to manifest.toml file</param>
    /// <returns>Result containing PatchManifest or error</returns>
    public static PatchResult<PatchManifest> ParseFile(string manifestPath)
    {
        if (!File.Exists(manifestPath))
        {
            return PatchResult<PatchManifest>.Fail($"Manifest file not found: {manifestPath}");
        }

        try
        {
            var tomlContent = File.ReadAllText(manifestPath);
            return ParseString(tomlContent);
        }
        catch (Exception ex)
        {
            return PatchResult<PatchManifest>.Fail($"Failed to read manifest file: {ex.Message}");
        }
    }

    /// <summary>
    /// Parses manifest TOML content from a string
    /// </summary>
    /// <param name="tomlContent">TOML content as string</param>
    /// <returns>Result containing PatchManifest or error</returns>
    public static PatchResult<PatchManifest> ParseString(string tomlContent)
    {
        try
        {
            var model = Toml.ToModel(tomlContent);

            if (!model.TryGetValue("patch", out var patchObj) || patchObj is not TomlTable patchTable)
            {
                return PatchResult<PatchManifest>.Fail("Manifest missing [patch] section");
            }

            // Required fields
            if (!TryGetString(patchTable, "id", out var id))
                return PatchResult<PatchManifest>.Fail("Manifest missing required field: patch.id");

            if (!TryGetString(patchTable, "name", out var name))
                return PatchResult<PatchManifest>.Fail("Manifest missing required field: patch.name");

            if (!TryGetString(patchTable, "version", out var version))
                return PatchResult<PatchManifest>.Fail("Manifest missing required field: patch.version");

            if (!TryGetString(patchTable, "author", out var author))
                return PatchResult<PatchManifest>.Fail("Manifest missing required field: patch.author");

            if (!TryGetString(patchTable, "description", out var description))
                return PatchResult<PatchManifest>.Fail("Manifest missing required field: patch.description");

            // Optional fields
            var requires = TryGetStringArray(patchTable, "requires") ?? new List<string>();
            var conflicts = TryGetStringArray(patchTable, "conflicts") ?? new List<string>();

            // Supported versions dictionary
            var supportedVersions = new Dictionary<string, string>();
            if (patchTable.TryGetValue("supported_versions", out var versionsObj) &&
                versionsObj is TomlTable versionsTable)
            {
                foreach (var kvp in versionsTable)
                {
                    if (kvp.Value is string hash)
                    {
                        supportedVersions[kvp.Key] = hash;
                    }
                }
            }

            var optionsResult = ParseOptions(patchTable);
            if (!optionsResult.Success || optionsResult.Data == null)
                return PatchResult<PatchManifest>.Fail(optionsResult.Error ?? "Failed to parse patch.options");

            var manifest = new PatchManifest
            {
                Id = id,
                Name = name,
                Version = version,
                Author = author,
                Description = description,
                Requires = requires,
                Conflicts = conflicts,
                SupportedVersions = supportedVersions,
                Options = optionsResult.Data
            };

            return PatchResult<PatchManifest>.Ok(manifest, "Manifest parsed successfully");
        }
        catch (Exception ex)
        {
            return PatchResult<PatchManifest>.Fail($"Failed to parse manifest TOML: {ex.Message}");
        }
    }

    /// <summary>
    /// Parses [[patch.options]]. A manifest without any has no options.
    /// </summary>
    private static PatchResult<List<PatchOption>> ParseOptions(TomlTable patchTable)
    {
        var options = new List<PatchOption>();
        if (!patchTable.TryGetValue("options", out var optionsObj))
            return PatchResult<List<PatchOption>>.Ok(options);

        if (optionsObj is not TomlTableArray optionsArray)
            return PatchResult<List<PatchOption>>.Fail("patch.options must be an array of tables ([[patch.options]])");

        for (int i = 0; i < optionsArray.Count; i++)
        {
            var table = optionsArray[i];
            var where = $"patch.options[{i}]";

            if (!TryGetString(table, "id", out var id))
                return PatchResult<List<PatchOption>>.Fail($"{where} missing required field: id");
            where = $"patch.options '{id}'";

            if (!id.All(c => char.IsAsciiLetterOrDigit(c) || c is '_' or '-'))
                return PatchResult<List<PatchOption>>.Fail($"{where}: id may only use [a-zA-Z0-9_-]");

            if (options.Any(o => o.Id == id))
                return PatchResult<List<PatchOption>>.Fail($"{where}: declared more than once");

            if (!TryGetString(table, "name", out var name))
                return PatchResult<List<PatchOption>>.Fail($"{where} missing required field: name");

            TryGetString(table, "description", out var description);

            if (table.ContainsKey("type") && !TryGetString(table, "type", out _))
                return PatchResult<List<PatchOption>>.Fail($"{where}: type must be \"toggle\" or \"choice\"");

            var typeText = TryGetString(table, "type", out var t) ? t.ToLowerInvariant() : "toggle";
            PatchOptionType type;
            if (typeText == "toggle") type = PatchOptionType.Toggle;
            else if (typeText == "choice") type = PatchOptionType.Choice;
            else return PatchResult<List<PatchOption>>.Fail($"{where}: unknown type '{typeText}' (expected toggle or choice)");

            var choices = new List<PatchOptionChoice>();
            string defaultValue;

            if (type == PatchOptionType.Toggle)
            {
                if (table.ContainsKey("choices"))
                    return PatchResult<List<PatchOption>>.Fail($"{where}: a toggle takes no choices");

                // Off unless the manifest says otherwise.
                if (!table.TryGetValue("default", out var defaultObj))
                    defaultValue = PatchOption.Off;
                else if (defaultObj is bool on)
                    defaultValue = on ? PatchOption.On : PatchOption.Off;
                else
                    return PatchResult<List<PatchOption>>.Fail($"{where}: a toggle's default must be true or false");
            }
            else
            {
                // Inline tables (choices = [{ ... }]) and [[patch.options.choices]] both work.
                table.TryGetValue("choices", out var choicesObj);
                IEnumerable<object?>? choiceItems = choicesObj switch
                {
                    TomlTableArray tables => tables,
                    TomlArray array => array,
                    _ => null
                };
                if (choiceItems == null)
                    return PatchResult<List<PatchOption>>.Fail($"{where}: a choice needs a choices array");

                foreach (var item in choiceItems)
                {
                    if (item is not TomlTable choiceTable || !TryGetString(choiceTable, "id", out var choiceId))
                        return PatchResult<List<PatchOption>>.Fail($"{where}: every choice needs an id");

                    if (!choiceId.All(c => char.IsAsciiLetterOrDigit(c) || c is '_' or '-'))
                        return PatchResult<List<PatchOption>>.Fail($"{where}: choice id '{choiceId}' may only use [a-zA-Z0-9_-]");

                    // A toggle's values. patch_config.toml writes those two as booleans, and
                    // a hook's `is = true` means the toggle, so a choice may not be named either.
                    if (choiceId is PatchOption.On or PatchOption.Off)
                        return PatchResult<List<PatchOption>>.Fail($"{where}: a choice cannot be called '{choiceId}'");

                    if (choices.Any(c => c.Id == choiceId))
                        return PatchResult<List<PatchOption>>.Fail($"{where}: choice '{choiceId}' listed more than once");

                    choices.Add(new PatchOptionChoice
                    {
                        Id = choiceId,
                        Name = TryGetString(choiceTable, "name", out var choiceName) ? choiceName : choiceId
                    });
                }

                if (choices.Count < 2)
                    return PatchResult<List<PatchOption>>.Fail($"{where}: a choice needs at least two choices");

                if (!TryGetString(table, "default", out defaultValue))
                    return PatchResult<List<PatchOption>>.Fail($"{where} missing required field: default");

                if (choices.All(c => c.Id != defaultValue))
                    return PatchResult<List<PatchOption>>.Fail($"{where}: default '{defaultValue}' is not one of its choices");
            }

            options.Add(new PatchOption
            {
                Id = id,
                Name = name,
                Description = description,
                Type = type,
                Choices = choices,
                Default = defaultValue
            });
        }

        return PatchResult<List<PatchOption>>.Ok(options);
    }

    private static bool TryGetString(TomlTable table, string key, out string value)
    {
        if (table.TryGetValue(key, out var obj) && obj is string str && !string.IsNullOrWhiteSpace(str))
        {
            value = str;
            return true;
        }

        value = string.Empty;
        return false;
    }

    private static List<string>? TryGetStringArray(TomlTable table, string key)
    {
        if (!table.TryGetValue(key, out var obj) || obj is not TomlArray array)
            return null;

        var result = new List<string>();
        foreach (var item in array)
        {
            if (item is string str && !string.IsNullOrWhiteSpace(str))
            {
                result.Add(str);
            }
        }

        return result;
    }
}
