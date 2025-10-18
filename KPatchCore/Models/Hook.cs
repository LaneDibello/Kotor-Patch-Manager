namespace KPatchCore.Models;

/// <summary>
/// Hook type determines how the runtime patcher applies the hook
/// </summary>
public enum HookType
{
    /// <summary>
    /// INLINE: Wrapper with automatic state management (default, recommended)
    /// </summary>
    Inline,

    /// <summary>
    /// REPLACE: Direct JMP, patch handles everything (advanced users)
    /// </summary>
    Replace,

    /// <summary>
    /// WRAP: Call patch then original function (requires detours - Phase 2)
    /// </summary>
    Wrap
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
    public required string Function { get; init; }

    /// <summary>
    /// Expected original bytes at hook address (for verification)
    /// </summary>
    public required byte[] OriginalBytes { get; init; }

    /// <summary>
    /// Bytes overritten after the hook address
    /// </summary>
    public required byte[] StolenBytes { get; init; }

    /// <summary>
    /// Hook type (Inline, Replace, or Wrap)
    /// Default: Inline (safest, easiest)
    /// </summary>
    public HookType Type { get; init; } = HookType.Inline;

    /// <summary>
    /// Preserve all registers (for Inline/Wrap types)
    /// Default: true
    /// </summary>
    public bool PreserveRegisters { get; init; } = true;

    /// <summary>
    /// Preserve EFLAGS register (for Inline/Wrap types)
    /// Default: true
    /// </summary>
    public bool PreserveFlags { get; init; } = true;

    /// <summary>
    /// Registers to exclude from restoration (e.g., ["eax", "edx"])
    /// Allows patch to modify specific registers
    /// </summary>
    public List<string> ExcludeFromRestore { get; init; } = new();

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

        if (string.IsNullOrWhiteSpace(Function))
        {
            error = "Function name cannot be empty";
            return false;
        }

        if (OriginalBytes == null || OriginalBytes.Length == 0)
        {
            error = "OriginalBytes must contain at least one byte";
            return false;
        }

        if (OriginalBytes.Length < 5 && Type != HookType.Replace)
        {
            error = "OriginalBytes should be at least 5 bytes for proper verification";
            return false;
        }

        error = null;
        return true;
    }

    public override string ToString() =>
        $"{Function} @ 0x{Address:X8} ({Type})";
}
