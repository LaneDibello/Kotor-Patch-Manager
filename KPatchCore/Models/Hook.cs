namespace KPatchCore.Models;

/// <summary>
/// Hook type determines how the runtime patcher applies the hook
/// </summary>
public enum HookType
{
    /// <summary>
    /// DETOUR: Trampoline with JMP, wrapper with automatic state management (default, recommended)
    /// </summary>
    Detour
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
    /// Original bytes at hook address (for verification and execution)
    /// These bytes are overwritten with JMP + NOPs and executed in the wrapper
    /// Must be >= 5 bytes for Detour hooks (JMP instruction)
    /// </summary>
    public required byte[] OriginalBytes { get; init; }

    /// <summary>
    /// Hook type (currently only Detour is supported)
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

        if (OriginalBytes.Length < 5 && Type == HookType.Detour)
        {
            error = "OriginalBytes must be at least 5 bytes for Detour hooks (JMP instruction)";
            return false;
        }

        // Validate parameters
        for (int i = 0; i < Parameters.Count; i++)
        {
            if (!Parameters[i].IsValid(out var paramError))
            {
                error = $"Parameter {i}: {paramError}";
                return false;
            }
        }

        error = null;
        return true;
    }

    public override string ToString() =>
        $"{Function} @ 0x{Address:X8} ({Type})";
}
