using KPatchCore.Models;
using Tomlyn;
using Tomlyn.Model;

namespace KPatchCore.Applicators;

/// <summary>
/// Generates patch_config.toml for the C++ runtime
/// </summary>
public static class ConfigGenerator
{
    /// <summary>
    /// Generates patch_config.toml and saves it to a file
    /// </summary>
    /// <param name="config">Patch configuration</param>
    /// <param name="outputPath">Where to save the config file</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult GenerateConfigFile(PatchConfig config, string outputPath)
    {
        try
        {
            var tomlString = GenerateConfigString(config);
            File.WriteAllText(outputPath, tomlString);

            return PatchResult.Ok($"Config generated: {outputPath}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to generate config file: {ex.Message}");
        }
    }

    /// <summary>
    /// Generates patch_config.toml content as a string
    /// </summary>
    /// <param name="config">Patch configuration</param>
    /// <returns>TOML content as string</returns>
    public static string GenerateConfigString(PatchConfig config)
    {
        var model = new TomlTable();

        // Create array of patches
        var patchesArray = new TomlTableArray();

        foreach (var patch in config.Patches)
        {
            var patchTable = new TomlTable
            {
                ["id"] = patch.Id,
                ["dll"] = patch.Dll
            };

            // Add hooks array if there are any
            if (patch.Hooks.Count > 0)
            {
                var hooksArray = new TomlTableArray();

                foreach (var hook in patch.Hooks)
                {
                    var originalBytesArray = new TomlArray();
                    foreach (var b in hook.OriginalBytes)
                    {
                        originalBytesArray.Add((long)b);
                    }

                    var stolenBytesArray = new TomlArray();
                    foreach (var b in hook.StolenBytes)
                    {
                        stolenBytesArray.Add((long)b);
                    }

                    var hookTable = new TomlTable
                    {
                        ["address"] = (long)hook.Address,
                        ["function"] = hook.Function,
                        ["original_bytes"] = originalBytesArray,
                        ["stolen_bytes"] = stolenBytesArray,
                        ["type"] = hook.Type.ToString().ToLowerInvariant()
                    };

                    // optional wrapper system fields
                    if (!hook.PreserveRegisters)
                    {
                        hookTable["preserve_registers"] = false;
                    }

                    if (!hook.PreserveFlags)
                    {
                        hookTable["preserve_flags"] = false;
                    }

                    if (hook.ExcludeFromRestore.Count > 0)
                    {
                        var excludeArray = new TomlArray();
                        foreach (var reg in hook.ExcludeFromRestore)
                        {
                            excludeArray.Add(reg);
                        }
                        hookTable["exclude_from_restore"] = excludeArray;
                    }

                    // Add parameters if any (for INLINE hooks)
                    if (hook.Parameters.Count > 0)
                    {
                        var parametersArray = new TomlTableArray();
                        foreach (var param in hook.Parameters)
                        {
                            var paramTable = new TomlTable
                            {
                                ["source"] = param.Source,
                                ["type"] = param.Type.ToString().ToLowerInvariant()
                            };
                            parametersArray.Add(paramTable);
                        }
                        hookTable["parameters"] = parametersArray;
                    }

                    hooksArray.Add(hookTable);
                }

                patchTable["hooks"] = hooksArray;
            }

            patchesArray.Add(patchTable);
        }

        model["patches"] = patchesArray;

        return Toml.FromModel(model);
    }

    /// <summary>
    /// Validates that a generated config can be parsed back correctly
    /// </summary>
    /// <param name="config">Config to validate</param>
    /// <returns>Result indicating if round-trip is successful</returns>
    public static PatchResult ValidateConfig(PatchConfig config)
    {
        try
        {
            // Generate TOML
            var tomlString = GenerateConfigString(config);

            var model = Toml.ToModel(tomlString);

            if (!model.ContainsKey("patches"))
            {
                return PatchResult.Fail("Generated config missing 'patches' section");
            }

            var patches = model["patches"] as TomlTableArray;
            if (patches == null)
            {
                return PatchResult.Fail("'patches' is not an array");
            }

            if (patches.Count != config.Patches.Count)
            {
                return PatchResult.Fail(
                    $"Patch count mismatch: expected {config.Patches.Count}, got {patches.Count}"
                );
            }

            return PatchResult.Ok("Config validation passed");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Config validation failed: {ex.Message}");
        }
    }
}
