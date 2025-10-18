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

        // Check for valid register names
        string[] validRegisters = { "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp" };

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

        // Check if it's a stack offset like "esp+4"
        if (source.StartsWith("esp+") || source.StartsWith("esp-"))
        {
            var offsetPart = source[4..];
            if (int.TryParse(offsetPart, out _))
            {
                error = null;
                return true;
            }
        }

        error = $"Invalid parameter source: '{Source}'. Expected register (eax, ebx, etc.) or stack offset (esp+0, esp+4, etc.)";
        return false;
    }

    public override string ToString() =>
        $"{Type} from {Source}";
}
