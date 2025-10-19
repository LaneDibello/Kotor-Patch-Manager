using KPatchCore.Models;
using Tomlyn;
using Tomlyn.Model;

namespace KPatchCore.Parsers;

/// <summary>
/// Parses hooks.toml files from .kpatch archives
/// </summary>
public static class HooksParser
{
    /// <summary>
    /// Parses a hooks.toml file and returns a list of Hooks
    /// </summary>
    /// <param name="hooksPath">Path to hooks.toml file</param>
    /// <returns>Result containing list of Hooks or error</returns>
    public static PatchResult<List<Hook>> ParseFile(string hooksPath)
    {
        if (!File.Exists(hooksPath))
        {
            return PatchResult<List<Hook>>.Fail($"Hooks file not found: {hooksPath}");
        }

        try
        {
            var tomlContent = File.ReadAllText(hooksPath);
            return ParseString(tomlContent);
        }
        catch (Exception ex)
        {
            return PatchResult<List<Hook>>.Fail($"Failed to read hooks file: {ex.Message}");
        }
    }

    /// <summary>
    /// Parses hooks TOML content from a string
    /// </summary>
    /// <param name="tomlContent">TOML content as string</param>
    /// <returns>Result containing list of Hooks or error</returns>
    public static PatchResult<List<Hook>> ParseString(string tomlContent)
    {
        try
        {
            var model = Toml.ToModel(tomlContent);

            if (!model.TryGetValue("hooks", out var hooksObj) || hooksObj is not TomlTableArray hooksArray)
            {
                return PatchResult<List<Hook>>.Fail("Hooks file missing [[hooks]] array");
            }

            var hooks = new List<Hook>();

            for (int i = 0; i < hooksArray.Count; i++)
            {
                var hookTable = hooksArray[i];

                // Parse required fields
                if (!TryGetUInt(hookTable, "address", out var address))
                {
                    return PatchResult<List<Hook>>.Fail($"Hook [{i}] missing required field: address");
                }

                if (!TryGetString(hookTable, "function", out var function))
                {
                    return PatchResult<List<Hook>>.Fail($"Hook [{i}] missing required field: function");
                }

                if (!TryGetByteArray(hookTable, "original_bytes", out var originalBytes))
                {
                    return PatchResult<List<Hook>>.Fail($"Hook [{i}] missing required field: original_bytes");
                }

                // Parse optional fields
                var type = ParseHookType(hookTable);
                var preserveRegisters = TryGetBool(hookTable, "preserve_registers") ?? true;
                var preserveFlags = TryGetBool(hookTable, "preserve_flags") ?? true;
                var excludeFromRestore = TryGetStringArray(hookTable, "exclude_from_restore") ?? new List<string>();

                // Parse parameters (optional, for Detour hooks)
                var parametersResult = ParseParameters(hookTable, i);
                if (!parametersResult.Success)
                {
                    return PatchResult<List<Hook>>.Fail(parametersResult.Error ?? "Failed to parse parameters");
                }
                var parameters = parametersResult.Data ?? new List<Parameter>();

                var hook = new Hook
                {
                    Address = address,
                    Function = function,
                    OriginalBytes = originalBytes,
                    Type = type,
                    PreserveRegisters = preserveRegisters,
                    PreserveFlags = preserveFlags,
                    ExcludeFromRestore = excludeFromRestore,
                    Parameters = parameters
                };

                // Validate the hook
                if (!hook.IsValid(out var error))
                {
                    return PatchResult<List<Hook>>.Fail($"Hook [{i}] ({function}) validation failed: {error}");
                }

                hooks.Add(hook);
            }

            return PatchResult<List<Hook>>.Ok(hooks, $"Parsed {hooks.Count} hook(s) successfully");
        }
        catch (Exception ex)
        {
            return PatchResult<List<Hook>>.Fail($"Failed to parse hooks TOML: {ex.Message}");
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

    private static bool TryGetUInt(TomlTable table, string key, out uint value)
    {
        if (table.TryGetValue(key, out var obj))
        {
            // TOML numbers can be long or int
            if (obj is long longVal)
            {
                value = (uint)longVal;
                return true;
            }
            else if (obj is int intVal)
            {
                value = (uint)intVal;
                return true;
            }
        }

        value = 0;
        return false;
    }

    private static bool? TryGetBool(TomlTable table, string key)
    {
        if (table.TryGetValue(key, out var obj) && obj is bool boolVal)
        {
            return boolVal;
        }
        return null;
    }

    private static bool TryGetByteArray(TomlTable table, string key, out byte[] value)
    {
        if (table.TryGetValue(key, out var obj) && obj is TomlArray array)
        {
            var bytes = new List<byte>();
            foreach (var item in array)
            {
                if (item is long longVal && longVal >= 0 && longVal <= 255)
                {
                    bytes.Add((byte)longVal);
                }
                else if (item is int intVal && intVal >= 0 && intVal <= 255)
                {
                    bytes.Add((byte)intVal);
                }
                else
                {
                    value = Array.Empty<byte>();
                    return false;
                }
            }

            value = bytes.ToArray();
            return bytes.Count > 0;
        }

        value = Array.Empty<byte>();
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

    private static HookType ParseHookType(TomlTable table)
    {
        if (!TryGetString(table, "type", out var typeStr))
            return HookType.Detour; // Default

        return typeStr.ToLowerInvariant() switch
        {
            "detour" => HookType.Detour,
            _ => HookType.Detour // Invalid types default to Detour
        };
    }

    private static PatchResult<List<Parameter>> ParseParameters(TomlTable hookTable, int hookIndex)
    {
        // Parameters are optional
        if (!hookTable.TryGetValue("parameters", out var paramsObj) || paramsObj is not TomlTableArray paramsArray)
        {
            return PatchResult<List<Parameter>>.Ok(new List<Parameter>(), "No parameters specified");
        }

        var parameters = new List<Parameter>();

        for (int i = 0; i < paramsArray.Count; i++)
        {
            var paramTable = paramsArray[i];

            // Parse source (required)
            if (!TryGetString(paramTable, "source", out var source))
            {
                return PatchResult<List<Parameter>>.Fail(
                    $"Hook [{hookIndex}] parameter [{i}] missing required field: source");
            }

            // Parse type (required)
            if (!TryGetString(paramTable, "type", out var typeStr))
            {
                return PatchResult<List<Parameter>>.Fail(
                    $"Hook [{hookIndex}] parameter [{i}] missing required field: type");
            }

            var paramType = ParseParameterType(typeStr);
            if (!paramType.HasValue)
            {
                return PatchResult<List<Parameter>>.Fail(
                    $"Hook [{hookIndex}] parameter [{i}] has invalid type: '{typeStr}'. " +
                    $"Valid types: int, uint, pointer, float, byte, short");
            }

            var parameter = new Parameter
            {
                Source = source,
                Type = paramType.Value
            };

            // Validate parameter
            if (!parameter.IsValid(out var error))
            {
                return PatchResult<List<Parameter>>.Fail(
                    $"Hook [{hookIndex}] parameter [{i}] validation failed: {error}");
            }

            parameters.Add(parameter);
        }

        return PatchResult<List<Parameter>>.Ok(parameters, $"Parsed {parameters.Count} parameter(s)");
    }

    private static ParameterType? ParseParameterType(string typeStr)
    {
        return typeStr.ToLowerInvariant() switch
        {
            "int" => ParameterType.Int,
            "uint" => ParameterType.UInt,
            "pointer" => ParameterType.Pointer,
            "float" => ParameterType.Float,
            "byte" => ParameterType.Byte,
            "short" => ParameterType.Short,
            _ => null
        };
    }
}
