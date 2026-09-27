using System.Globalization;

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
    Short,

    /// <summary>8-bit value, sign-extended</summary>
    SByte,

    /// <summary>16-bit value, sign-extended</summary>
    SShort,

    /// <summary>64-bit integer (x86_64 only)</summary>
    Int64,

    /// <summary>Unsigned 64-bit integer (x86_64 only)</summary>
    UInt64,

    /// <summary>64-bit floating point (x86_64 only)</summary>
    Double
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
    //
    // Nor "const:<value>", which reads nothing at all and so is taken wherever a
    // generator exists.
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

    private const string ConstantPrefix = "const:";

    private static readonly string[] X86StackPrefixes = { "esp+", "esp-" };

    // "esp+8" as well as "rsp+8", so a hook ported off the Windows build keeps working
    // unedited. The x86_64 generator treats the two spellings alike.
    private static readonly string[] X86_64StackPrefixes = { "rsp+", "rsp-", "esp+", "esp-" };

    /// <summary>
    /// Source location to read parameter from
    /// Examples: "eax", "esp+0", "esp+4", "rbp", "r15", "rsp+8", "const:0xBC"
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

        // x86_64 is the wider of the two, so it gives a constant the most generous limit
        // any target would apply.
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

        if (NeedsSixtyFourBits && architecture != Architecture.x86_64)
        {
            return $"Parameter type {Type} is 64 bits wide, which {architecture} has neither " +
                   "a register to read from nor a single argument slot to pass in";
        }

        var source = Source.Trim().ToLowerInvariant();

        if (source.StartsWith(ConstantPrefix, StringComparison.Ordinal))
        {
            if (Type == ParameterType.Float)
            {
                return $"Constant parameter '{Source}' cannot be a Float. There is no float " +
                       "literal syntax, and an integer parse would not be the value meant";
            }

            return TryParseConstant(source[ConstantPrefix.Length..], out _)
                ? $"Constant parameter '{Source}' is not a {Type} that {architecture} can hold. " +
                  $"The widest {Type} there is 0x{WidestValue(architecture):X}"
                : $"Constant parameter '{Source}' is not a number. Write it unsigned, in " +
                  "decimal or with a 0x prefix";
        }

        if (NarrowOrFloat &&
            sources.StackPrefixes.Any(p => source.StartsWith(p, StringComparison.Ordinal)))
        {
            return $"Parameter source '{Source}' yields the address of a stack slot, which is " +
                   $"pointer-width, so it cannot be read as {Type}. Bracket it, as " +
                   $"'[{Source}]', to read what the slot holds instead";
        }

        return $"Parameter source '{Source}' cannot be read on {architecture}. " +
               $"Readable there: {string.Join(", ", sources.Registers)}, " +
               $"an offset from {string.Join(" / ", sources.StackPrefixes.Select(p => p[..3]).Distinct())}, " +
               "or a constant (const:0xBC)";
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

    // The register named inside brackets, with an optional signed offset. The stack pointer
    // is readable here although it is not on its own, a dereference reaching the slot's
    // contents rather than needing a saved copy of the pointer.
    private static bool IsDereferenceable(
        string inner, (string[] Registers, string[] StackPrefixes) sources)
    {
        var sign = inner.IndexOfAny(new[] { '+', '-' });
        var name = sign < 0 ? inner : inner[..sign];

        if (name.Length == 0 || (sign >= 0 && !TryParseSignedOffset(inner[sign..], out _)))
        {
            return false;
        }

        // Taken off the stack prefixes so the bracketed form accepts the same spellings the
        // address form does, including "esp" on x86_64.
        return sources.StackPrefixes.Any(prefix => name == prefix[..3]) ||
               sources.Registers.Contains(name);
    }

    // The same grammar as ParseSignedOffset in wrapper_base.h: a sign, then digits as a
    // constant's are written, fitting an int. signed-offsets.tsv holds both to it. Internal so
    // that corpus can reach it.
    internal static bool TryParseSignedOffset(string text, out int value)
    {
        value = 0;
        if (text.Length == 0 || (text[0] != '+' && text[0] != '-') ||
            !TryParseConstant(text[1..], out var magnitude))
        {
            return false;
        }

        // The most negative int has one more unit of magnitude than the most positive.
        var negative = text[0] == '-';
        var limit = (ulong)int.MaxValue + (negative ? 1UL : 0UL);
        if (magnitude > limit)
        {
            return false;
        }

        value = (int)(negative ? -(long)magnitude : (long)magnitude);
        return true;
    }

    // The same grammar as ParseConstantSource in wrapper_base.h; constant-sources.tsv holds
    // both to it, and explains why it exists twice. Internal so that corpus can reach it.
    internal static bool TryParseConstant(string text, out ulong value)
    {
        value = 0;

        // A sign would wrap into a huge unsigned value rather than fail, and leading
        // whitespace would be skipped, so both are turned away before parsing.
        if (text.Length == 0 || !char.IsAsciiDigit(text[0]))
        {
            return false;
        }

        var hexadecimal = text.Length > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
        var digits = hexadecimal ? text[2..] : text;
        var style = hexadecimal ? NumberStyles.AllowHexSpecifier : NumberStyles.None;

        return ulong.TryParse(digits, style, CultureInfo.InvariantCulture, out value);
    }

    private bool ConstantFits(string text, Architecture architecture) =>
        // There is no syntax for a float literal here, and a bit pattern parsed as an
        // integer would not be the float the hook meant.
        Type != ParameterType.Float &&
        TryParseConstant(text, out var value) &&
        value <= WidestValue(architecture);

    private ulong WidestValue(Architecture architecture) => Type switch
    {
        ParameterType.Byte or ParameterType.SByte => byte.MaxValue,
        ParameterType.Short or ParameterType.SShort => ushort.MaxValue,
        // The types that hold a full register, on the target that has 64-bit ones.
        ParameterType.Pointer or ParameterType.Int64 or ParameterType.UInt64
            when architecture == Architecture.x86_64 => ulong.MaxValue,
        ParameterType.Int or ParameterType.UInt or ParameterType.Pointer => uint.MaxValue,
        // A float has no literal syntax, a 64-bit type does not exist on a 32-bit target,
        // and nothing outside the enum has a width. All refuse every value rather than
        // defaulting to the widest on offer.
        _ => 0
    };

    // The types that describe a value rather than an address, so they cannot be asked of a
    // source that yields one.
    private bool NarrowOrFloat =>
        Type is ParameterType.Byte or ParameterType.Short or ParameterType.SByte
             or ParameterType.SShort or ParameterType.Float or ParameterType.Double;

    // Only x86_64 has a register wide enough to read one of these out of, and only its
    // convention has a single argument slot that holds one.
    private bool NeedsSixtyFourBits =>
        Type is ParameterType.Int64 or ParameterType.UInt64 or ParameterType.Double;

    private bool IsReadableOn(Architecture architecture)
    {
        if (ReadableSources(architecture) is not { } sources)
        {
            return false;
        }

        if (NeedsSixtyFourBits && architecture != Architecture.x86_64)
        {
            return false;
        }

        var source = Source.Trim().ToLowerInvariant();

        // A constant is a literal rather than something read from anywhere, so it is
        // readable wherever a generator exists, subject only to its width.
        if (source.StartsWith(ConstantPrefix, StringComparison.Ordinal))
        {
            return ConstantFits(source[ConstantPrefix.Length..], architecture);
        }

        // A bracketed source is a dereference, so what has to be readable is the register
        // inside it. No type is refused here: a value arrives, so any width describes it.
        if (source.Length > 2 && source[0] == '[' && source[^1] == ']')
        {
            return IsDereferenceable(source[1..^1], sources);
        }

        if (sources.Registers.Contains(source))
        {
            return true;
        }

        // A stack source always carries an offset. The generators hand the patch function the
        // address of that slot, so "esp" on its own would name a slot the hook never picked.
        if (!sources.StackPrefixes.Any(prefix => source.StartsWith(prefix, StringComparison.Ordinal)) ||
            !TryParseSignedOffset(source[3..], out _))
        {
            return false;
        }

        // What arrives is the slot's address, which is pointer-width whatever the slot holds,
        // so a narrow type or a float describes something else. Both generators refuse this;
        // saying so here turns a hook that fails to install into a patch that fails to pass.
        return !NarrowOrFloat;
    }
}
