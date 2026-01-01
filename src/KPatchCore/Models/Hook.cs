namespace KPatchCore.Models;

/// <summary>
/// Hook type determines how the runtime patcher applies the hook
/// </summary>
public enum HookType
{
    /// <summary>
    /// DETOUR: Trampoline with JMP, wrapper with automatic state management (default for DLL hooks)
    /// </summary>
    Detour,

    /// <summary>
    /// SIMPLE: Direct byte replacement in memory (no DLL required)
    /// </summary>
    Simple,

    /// <summary>
    /// REPLACE: JMP to allocated code block with raw assembly, then JMP back (no wrapper, no DLL)
    /// </summary>
    Replace,

    /// <summary>
    /// STATIC: Applied at install-time directly to executable file (for PE header patches)
    /// </summary>
    Static
}

/// <summary>
/// Represents a single hook point in game code
/// </summary>
public sealed class Hook
{
    /// <summary>
    /// Memory address to hook (e.g., 0x401234)
    /// </summary>
    public required uint Address { get; init; }

    /// <summary>
    /// Exported function name in patch DLL
    /// </summary>
    public string? Function { get; init; }

    /// <summary>
    /// Original bytes at hook address (for verification and execution)
    /// For Detour: Overwritten with JMP + NOPs and executed in the wrapper (must be >= 5 bytes)
    /// For Simple: Used for verification before replacement (any length)
    /// For Replace: Overwritten with JMP + NOPs, used for verification (must be >= 5 bytes)
    /// </summary>
    public required byte[] OriginalBytes { get; init; }

    /// <summary>
    /// Replacement bytes
    /// For Simple: Must be same length as OriginalBytes
    /// For Replace: Can be any length, executed then JMP back
    /// Not used for Detour hooks
    /// </summary>
    public byte[]? ReplacementBytes { get; init; }

    /// <summary>
    /// Hook type (Detour, Simple, or Replace)
    /// Default: Detour
    /// </summary>
    public HookType Type { get; init; } = HookType.Detour;

    /// <summary>
    /// Preserve all registers (for Detour hooks)
    /// Default: true
    /// </summary>
    public bool PreserveRegisters { get; init; } = true;

    /// <summary>
    /// Preserve EFLAGS register (for Detour hooks)
    /// Default: true
    /// </summary>
    public bool PreserveFlags { get; init; } = true;

    /// <summary>
    /// Registers to exclude from restoration (e.g., ["eax", "edx"])
    /// Allows patch to modify specific registers
    /// </summary>
    public List<string> ExcludeFromRestore { get; init; } = new();

    /// <summary>
    /// Parameters to extract and pass to the hook function (for Detour hooks)
    /// If empty, hook function takes no parameters
    /// </summary>
    public List<Parameter> Parameters { get; init; } = new();

    /// <summary>
    /// Skip executing original bytes after patch function returns (for Detour hooks)
    /// Set to true when fully replacing behavior instead of augmenting it
    /// Default: false
    /// </summary>
    public bool SkipOriginalBytes { get; init; } = false;

    /// <summary>
    /// Validates that the hook configuration is valid
    /// </summary>
    public bool IsValid(out string? error)
    {
        if (Address == 0)
        {
            error = "Address cannot be zero";
            return false;
        }

        if (OriginalBytes == null || OriginalBytes.Length == 0)
        {
            error = "OriginalBytes must contain at least one byte";
            return false;
        }

        // Type-specific validation
        if (Type == HookType.Detour)
        {
            if (string.IsNullOrWhiteSpace(Function))
            {
                error = "Function name cannot be empty";
                return false;
            }

            if (OriginalBytes.Length < 5)
            {
                error = "OriginalBytes must be at least 5 bytes for Detour hooks (JMP instruction)";
                return false;
            }

            // Validate parameters for Detour hooks
            for (int i = 0; i < Parameters.Count; i++)
            {
                if (!Parameters[i].IsValid(out var paramError))
                {
                    error = $"Parameter {i}: {paramError}";
                    return false;
                }
            }
        }
        else if (Type == HookType.Simple)
        {
            if (ReplacementBytes == null || ReplacementBytes.Length == 0)
            {
                error = "ReplacementBytes required for Simple hooks";
                return false;
            }

            if (ReplacementBytes.Length != OriginalBytes.Length)
            {
                error = $"ReplacementBytes length ({ReplacementBytes.Length}) must match OriginalBytes length ({OriginalBytes.Length})";
                return false;
            }

            if (!string.IsNullOrWhiteSpace(Function))
            {
                error = "Simple hooks should not have a function name";
                return false;
            }

            if (Parameters.Count > 0)
            {
                error = "Simple hooks cannot have parameters";
                return false;
            }
        }
        else if (Type == HookType.Replace)
        {
            if (ReplacementBytes == null || ReplacementBytes.Length == 0)
            {
                error = "ReplacementBytes required for Replace hooks";
                return false;
            }

            if (OriginalBytes.Length < 5)
            {
                error = "OriginalBytes must be at least 5 bytes for Replace hooks (JMP instruction)";
                return false;
            }

            if (!string.IsNullOrWhiteSpace(Function))
            {
                error = "Replace hooks should not have a function name";
                return false;
            }

            if (Parameters.Count > 0)
            {
                error = "Replace hooks cannot have parameters";
                return false;
            }
        }
        else if (Type == HookType.Static)
        {
            if (ReplacementBytes == null || ReplacementBytes.Length == 0)
            {
                error = "ReplacementBytes required for Static hooks";
                return false;
            }

            if (ReplacementBytes.Length != OriginalBytes.Length)
            {
                error = $"ReplacementBytes length ({ReplacementBytes.Length}) must match OriginalBytes length ({OriginalBytes.Length})";
                return false;
            }

            if (!string.IsNullOrWhiteSpace(Function))
            {
                error = "Static hooks should not have a function name";
                return false;
            }

            if (Parameters.Count > 0)
            {
                error = "Static hooks cannot have parameters";
                return false;
            }
        }

        error = null;
        return true;
    }

    public override string ToString() =>
        $"{Function} @ 0x{Address:X8} ({Type})";
}
