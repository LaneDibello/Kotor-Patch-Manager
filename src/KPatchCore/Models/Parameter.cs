namespace KPatchCore.Models;

/// <summary>
/// How much of a parameter's source reaches the patch function, and how the rest of the
/// argument is filled.
/// </summary>
public enum ParameterType
{
    /// <summary>32-bit integer, on either target</summary>
    Int,

    /// <summary>Unsigned 32-bit integer, on either target</summary>
    UInt,

    /// <summary>Pointer-width: 32 bits on x86, 64 on x86_64</summary>
    Pointer,

    /// <summary>32-bit floating point</summary>
    Float,

    /// <summary>8-bit value, zero-extended</summary>
    Byte,

    /// <summary>16-bit value, zero-extended</summary>
    Short
}

/// <summary>
/// Represents a parameter to be extracted and passed to a hook function
/// </summary>
public sealed class Parameter
{
    // Each wrapper generator reads a different set of sources, and a hook naming one its
    // generator cannot read does not install. The lists below mirror the register cases in
    // wrapper_x86.cpp and wrapper_x86_64.cpp; changing one side means changing the other.
    //
    // Neither list holds the stack pointer. It is the one register the wrapper has no saved
    // copy of, because by the time the patch function runs it points into the wrapper's own
    // frame. A hook wanting the game's stack asks for "esp+0", which is that address.
    private static readonly string[] X86Registers =
    {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"
    };

    // Both spellings of all sixteen general-purpose registers. The x86_64 generator maps the
    // narrow name onto the same physical register, so a hook may say "rbp" where it means the
    // frame pointer rather than borrowing the 32-bit name for it.
    private static readonly string[] X86_64Registers =
    {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp",
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
    };

    private static readonly string[] X86StackPrefixes = { "esp+", "esp-" };

    // "esp+8" as well as "rsp+8", so a hook ported off the Windows build keeps working
    // unedited. The x86_64 generator treats the two spellings alike.
    private static readonly string[] X86_64StackPrefixes = { "rsp+", "rsp-", "esp+", "esp-" };

    /// <summary>
    /// Source location to read parameter from
    /// Examples: "eax", "esp+0", "esp+4", "rbp", "r15", "rsp+8"
    /// </summary>
    public required string Source { get; init; }

    /// <summary>
    /// Data type of the parameter
    /// </summary>
    public required ParameterType Type { get; init; }

    /// <summary>
    /// Checks that some wrapper generator could read this source.
    /// </summary>
    /// <param name="error">Why the source is unreadable, or null when it is fine</param>
    /// <returns>True when at least one architecture can read the source</returns>
    /// <remarks>
    /// Patches are parsed during the repository scan, before a game has been chosen, so the
    /// architecture is not known yet. Accepting anything some generator can read defers the
    /// narrower question to install time, when the detected build settles which generator runs.
    /// See <see cref="IsValidFor"/>.
    /// </remarks>
    public bool IsValid(out string? error)
    {
        if (string.IsNullOrWhiteSpace(Source))
        {
            error = "Parameter source cannot be empty";
            return false;
        }

        if (IsReadableOn(Architecture.x86) || IsReadableOn(Architecture.x86_64))
        {
            error = null;
            return true;
        }

        error = Explain(Architecture.x86_64);
        return false;
    }

    /// <summary>
    /// Checks that the generator for a given architecture could read this source.
    /// </summary>
    /// <param name="architecture">Architecture of the game build being patched</param>
    /// <param name="error">Why the source is unreadable there, or null when it is fine</param>
    /// <returns>True when that architecture's generator can read the source</returns>
    /// <remarks>
    /// Catches the source that is well-formed but belongs to the other architecture: "rax" in a
    /// hook aimed at a 32-bit build is a register the x86 generator has never heard of.
    /// </remarks>
    public bool IsValidFor(Architecture architecture, out string? error)
    {
        if (string.IsNullOrWhiteSpace(Source))
        {
            error = "Parameter source cannot be empty";
            return false;
        }

        if (IsReadableOn(architecture))
        {
            error = null;
            return true;
        }

        error = Explain(architecture);
        return false;
    }

    // Why a source that has just been refused is unreadable. A constant fails for reasons
    // a register list would not explain, so it is described on its own terms.
    private string Explain(Architecture architecture)
    {
        if (ReadableSources(architecture) is not { } sources)
        {
            return $"No wrapper generator targets {architecture}, so parameter source " +
                   $"'{Source}' cannot be read there";
        }

        return $"Parameter source '{Source}' cannot be read on {architecture}. " +
               $"Readable there: {string.Join(", ", sources.Registers)}, " +
               $"or an offset from {string.Join(" / ", sources.StackPrefixes.Select(p => p[..3]).Distinct())}";
    }

    public override string ToString() =>
        $"{Type} from {Source}";

    // Null where nothing generates wrappers, which makes every source unreadable there.
    private static (string[] Registers, string[] StackPrefixes)? ReadableSources(
        Architecture architecture) => architecture switch
    {
        Architecture.x86 => (X86Registers, X86StackPrefixes),
        Architecture.x86_64 => (X86_64Registers, X86_64StackPrefixes),
        Architecture.ARM64 => null,
        // Every named value is above; this arm catches an integer cast in from outside the
        // enum, which has no generator either.
        _ => null
    };

    private bool IsReadableOn(Architecture architecture)
    {
        if (ReadableSources(architecture) is not { } sources)
        {
            return false;
        }

        var source = Source.Trim().ToLowerInvariant();

        if (sources.Registers.Contains(source))
        {
            return true;
        }

        // A stack source always carries an offset. The generators hand the patch function the
        // address of that slot, so "esp" on its own would name a slot the hook never picked.
        return sources.StackPrefixes.Any(prefix => source.StartsWith(prefix, StringComparison.Ordinal)) &&
               int.TryParse(source[3..], out _);
    }
}
