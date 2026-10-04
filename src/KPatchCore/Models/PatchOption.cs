namespace KPatchCore.Models;

/// <summary>
/// What kind of value a patch option holds
/// </summary>
public enum PatchOptionType
{
    /// <summary>On or off. Its value is "true" or "false".</summary>
    Toggle,

    /// <summary>One of a fixed list. Its value is the id of the chosen entry.</summary>
    Choice
}

/// <summary>
/// One entry a <see cref="PatchOptionType.Choice"/> option can take
/// </summary>
public sealed class PatchOptionChoice
{
    /// <summary>
    /// Identifier hooks and patch code compare against (e.g., "compact")
    /// </summary>
    public required string Id { get; init; }

    /// <summary>
    /// Human-readable name shown in the launcher
    /// </summary>
    public required string Name { get; init; }
}

/// <summary>
/// A choice the player makes inside one patch, declared in manifest.toml as [[patch.options]]
/// </summary>
/// <remarks>
/// Options are resolved when patches are installed. A hook whose <see cref="Hook.When"/>
/// does not hold for the chosen values is left out, and the values are written under the
/// patch's id in patch-options.ini, where the patch's own code and scripts can read them.
/// The runtime never sees an unresolved option.
/// </remarks>
public sealed class PatchOption
{
    /// <summary>The value a toggle holds when on</summary>
    public const string On = "true";

    /// <summary>The value a toggle holds when off</summary>
    public const string Off = "false";

    /// <summary>
    /// Identifier unique within the patch (e.g., "map-notes")
    /// </summary>
    public required string Id { get; init; }

    /// <summary>
    /// Human-readable name shown in the launcher
    /// </summary>
    public required string Name { get; init; }

    /// <summary>
    /// Optional description of what the option changes
    /// </summary>
    public string Description { get; init; } = string.Empty;

    /// <summary>
    /// Kind of value
    /// </summary>
    public PatchOptionType Type { get; init; } = PatchOptionType.Toggle;

    /// <summary>
    /// The entries of a choice option, in the order they are shown. Empty for a toggle.
    /// </summary>
    public List<PatchOptionChoice> Choices { get; init; } = new();

    /// <summary>
    /// Value used when the player has not chosen one: "true"/"false" for a toggle,
    /// a choice id for a choice
    /// </summary>
    public required string Default { get; init; }

    /// <summary>
    /// Whether <paramref name="value"/> is one this option can hold
    /// </summary>
    public bool Accepts(string value) => Type == PatchOptionType.Toggle
        ? value is On or Off
        : Choices.Any(c => c.Id == value);

    /// <summary>
    /// The values this option can hold, in display order
    /// </summary>
    public IEnumerable<string> Values => Type == PatchOptionType.Toggle
        ? new[] { Off, On }
        : Choices.Select(c => c.Id);

    public override string ToString() => $"{Id} ({Type.ToString().ToLowerInvariant()}, default {Default})";
}

/// <summary>
/// The condition on a hook's <c>when</c> key: the hook is installed only while
/// <see cref="OptionId"/> holds <see cref="Value"/>
/// </summary>
/// <remarks>
/// <c>when = "map-notes"</c> is the short form for a toggle that is on.
/// <c>when = { option = "hud-style", is = "compact" }</c> names the value outright.
/// </remarks>
public sealed class HookCondition
{
    /// <summary>
    /// Id of the option this hook depends on
    /// </summary>
    public required string OptionId { get; init; }

    /// <summary>
    /// Value the option must hold for the hook to be installed
    /// </summary>
    public string Value { get; init; } = PatchOption.On;

    /// <summary>
    /// Whether this condition holds for a resolved set of option values
    /// </summary>
    public bool Holds(IReadOnlyDictionary<string, string> values) =>
        values.TryGetValue(OptionId, out var actual) && actual == Value;

    /// <summary>
    /// Whether this condition and <paramref name="other"/> can never both hold: they test
    /// the same option for different values
    /// </summary>
    public bool Excludes(HookCondition other) => OptionId == other.OptionId && Value != other.Value;

    public override string ToString() => $"{OptionId} = {Value}";
}
