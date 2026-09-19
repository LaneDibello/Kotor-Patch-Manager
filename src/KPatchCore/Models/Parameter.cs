namespace KPatchCore.Models;

/// <summary>
/// Parameter type for hook function parameters
/// </summary>
public enum ParameterType
{
    /// <summary>32-bit integer (DWORD)</summary>
    Int,

    /// <summary>Unsigned 32-bit integer (DWORD)</summary>
    UInt,

    /// <summary>32-bit pointer (void*)</summary>
    Pointer,

    /// <summary>32-bit floating point</summary>
    Float,

    /// <summary>8-bit value</summary>
    Byte,

    /// <summary>16-bit value</summary>
    Short
}

/// <summary>
/// Represents a parameter to be extracted and passed to a hook function
/// </summary>
public sealed class Parameter
{
    /// <summary>
    /// Source location to read parameter from
    /// Examples: "eax", "esp+0", "esp+4", "[eax]", "[esp+8]"
    /// </summary>
    public required string Source { get; init; }

    /// <summary>
    /// Data type of the parameter
    /// </summary>
    public required ParameterType Type { get; init; }

    /// <summary>
    /// Validates that the parameter configuration is valid
    /// </summary>
    public bool IsValid(out string? error)
    {
        if (string.IsNullOrWhiteSpace(Source))
        {
            error = "Parameter source cannot be empty";
            return false;
        }

        // Basic validation of source syntax
        var source = Source.Trim().ToLowerInvariant();

        // Check for valid register names. The 64-bit spellings are here for the macOS
        // build, whose wrapper generator accepts either width and maps both onto the
        // same physical register, so a hook may say "rbp" where it means the frame
        // pointer rather than borrowing the 32-bit name for it.
        string[] validRegisters =
        {
            "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
            "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
            "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
        };

        // Handle dereferenced sources like "[eax]" or "[esp+4]"
        if (source.StartsWith("[") && source.EndsWith("]"))
        {
            source = source[1..^1].Trim(); // Remove brackets
        }

        // Check if it's a register
        if (validRegisters.Contains(source))
        {
            error = null;
            return true;
        }

        // Check if it's a stack offset like "esp+4". Both generators read the offset
        // from the game's stack pointer at the hook site and pass the address of that
        // slot, so "rsp+8" means on macOS what "esp+8" means elsewhere.
        if (source.StartsWith("esp+") || source.StartsWith("esp-") ||
            source.StartsWith("rsp+") || source.StartsWith("rsp-"))
        {
            var offsetPart = source[3..];
            if (int.TryParse(offsetPart, out _))
            {
                error = null;
                return true;
            }
        }

        error = $"Invalid parameter source: '{Source}'. Expected register (eax, rbp, etc.) or stack offset (esp+0, rsp+8, etc.)";
        return false;
    }

    public override string ToString() =>
        $"{Type} from {Source}";
}
