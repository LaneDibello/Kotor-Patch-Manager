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

            var manifest = new PatchManifest
            {
                Id = id,
                Name = name,
                Version = version,
                Author = author,
                Description = description,
                Requires = requires,
                Conflicts = conflicts,
                SupportedVersions = supportedVersions
            };

            return PatchResult<PatchManifest>.Ok(manifest, "Manifest parsed successfully");
        }
        catch (Exception ex)
        {
            return PatchResult<PatchManifest>.Fail($"Failed to parse manifest TOML: {ex.Message}");
        }
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
