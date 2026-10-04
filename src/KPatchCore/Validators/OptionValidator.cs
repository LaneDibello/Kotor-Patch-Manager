using KPatchCore.Models;

namespace KPatchCore.Validators;

/// <summary>
/// Checks a patch's options against the hooks that depend on them, and resolves the
/// values a player chose into the hooks that get installed
/// </summary>
/// <remarks>
/// Options live in manifest.toml and conditions in the hooks files, so neither parser can
/// tell whether a hook names an option that exists. This is where both are known.
/// </remarks>
public static class OptionValidator
{
    /// <summary>
    /// Validates every hook condition of a patch against the options its manifest declares
    /// </summary>
    /// <param name="manifest">Manifest declaring the options</param>
    /// <param name="hooks">Hooks for one game version, conditions unresolved</param>
    /// <returns>Failure naming every hook whose condition cannot be satisfied or that
    /// shares an address with a hook that could be installed alongside it</returns>
    public static PatchResult ValidateHookConditions(PatchManifest manifest, IEnumerable<Hook> hooks)
    {
        var hookList = hooks.ToList();
        var errors = new List<string>();

        foreach (var hook in hookList)
        {
            if (hook.When == null)
                continue;

            var option = manifest.Options.FirstOrDefault(o => o.Id == hook.When.OptionId);
            if (option == null)
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: when names option '{hook.When.OptionId}', " +
                           $"which {manifest.Id} does not declare");
            }
            else if (!option.Accepts(hook.When.Value))
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: option '{option.Id}' can never be '{hook.When.Value}' " +
                           $"(it takes {string.Join(", ", option.Values)})");
            }
        }

        // One hook per address still holds for any one set of option values: two hooks may
        // share an address only when their conditions can never both be true.
        foreach (var group in hookList.GroupBy(h => h.Address).Where(g => g.Count() > 1))
        {
            var sharing = group.ToList();
            for (int a = 0; a < sharing.Count; a++)
            {
                for (int b = a + 1; b < sharing.Count; b++)
                {
                    var first = sharing[a].When;
                    var second = sharing[b].When;
                    if (first == null && second == null)
                    {
                        // Nothing to do with options: the rule a patch has always had.
                        errors.Add($"Multiple hooks at address 0x{group.Key:X8}");
                    }
                    else if (first == null || second == null || !first.Excludes(second))
                    {
                        errors.Add($"Multiple hooks at address 0x{group.Key:X8} can be installed together: " +
                                   $"each needs a when that tests the same option for a different value");
                    }
                }
            }
        }

        return errors.Count > 0
            ? PatchResult.Fail($"Hook validation failed for {manifest.Id}:\n  - {string.Join("\n  - ", errors.Distinct())}")
            : PatchResult.Ok("Hook conditions are valid");
    }

    /// <summary>
    /// Resolves the values a patch's options take: the chosen value where one was given
    /// and is valid, the option's default otherwise
    /// </summary>
    /// <param name="manifest">Manifest declaring the options</param>
    /// <param name="chosen">Values the player chose, by option id (may be null)</param>
    /// <returns>A value for every declared option, in declaration order; failure when a
    /// chosen value names an unknown option or one the option cannot hold</returns>
    public static PatchResult<Dictionary<string, string>> ResolveValues(
        PatchManifest manifest,
        IReadOnlyDictionary<string, string>? chosen)
    {
        var values = new Dictionary<string, string>();
        foreach (var option in manifest.Options)
            values[option.Id] = option.Default;

        if (chosen == null)
            return PatchResult<Dictionary<string, string>>.Ok(values);

        foreach (var (optionId, value) in chosen)
        {
            var option = manifest.Options.FirstOrDefault(o => o.Id == optionId);
            if (option == null)
            {
                return PatchResult<Dictionary<string, string>>.Fail(
                    $"{manifest.Id} has no option '{optionId}'" +
                    (manifest.Options.Count > 0
                        ? $" (it has {string.Join(", ", manifest.Options.Select(o => o.Id))})"
                        : " (it declares none)"));
            }

            var normalised = option.Type == PatchOptionType.Toggle ? value.ToLowerInvariant() : value;
            if (!option.Accepts(normalised))
            {
                return PatchResult<Dictionary<string, string>>.Fail(
                    $"{manifest.Id}: option '{optionId}' cannot be '{value}' " +
                    $"(it takes {string.Join(", ", option.Values)})");
            }

            values[optionId] = normalised;
        }

        return PatchResult<Dictionary<string, string>>.Ok(values);
    }

    /// <summary>
    /// The hooks that are installed for a resolved set of option values: every hook
    /// without a condition, and every hook whose condition holds
    /// </summary>
    public static List<Hook> SelectHooks(IEnumerable<Hook> hooks, IReadOnlyDictionary<string, string> values) =>
        hooks.Where(h => h.When == null || h.When.Holds(values)).ToList();
}
