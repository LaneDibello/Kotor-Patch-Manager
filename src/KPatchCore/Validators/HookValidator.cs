using KPatchCore.Models;

namespace KPatchCore.Validators;

/// <summary>
/// Validates hook configurations
/// </summary>
public static class HookValidator
{
    // A hook address is an absolute virtual address in the game image. These windows
    // exist to catch a mistyped or truncated address, not to bound the image exactly.
    // The 32-bit targets sit in the low 2 GB: a Windows PE at 0x400000, the Linux ELF
    // at 0x8048000. A 64-bit Mach-O is fixed just above its 4 GB __PAGEZERO, which is
    // what puts a macOS hook at 0x1004EB6C2 rather than 0x4EB6C2.
    private const ulong MinAddress = 0x00400000;
    private const ulong MaxAddress = 0x7FFFFFFF;
    private const ulong MinMachOAddress = 0x1_00000000;
    private const ulong MaxMachOAddress = 0x1_FFFFFFFF;

    private static bool IsPlausibleImageAddress(ulong address) =>
        (address >= MinAddress && address <= MaxAddress) ||
        (address >= MinMachOAddress && address <= MaxMachOAddress);

    /// <summary>
    /// Validates a single hook
    /// </summary>
    /// <param name="hook">Hook to validate</param>
    /// <returns>Result indicating if hook is valid</returns>
    public static PatchResult ValidateHook(Hook hook)
    {
        // Use the Hook's built-in validation
        if (!hook.IsValid(out var error))
        {
            return PatchResult.Fail($"Hook validation failed: {error}");
        }

        // Additional address range check
        if (!IsPlausibleImageAddress(hook.Address))
        {
            return PatchResult.Fail(
                $"Hook address 0x{hook.Address:X8} is outside the ranges a game image occupies " +
                $"(0x{MinAddress:X8}-0x{MaxAddress:X8}, or 0x{MinMachOAddress:X8}-0x{MaxMachOAddress:X8} for a 64-bit Mach-O)"
            );
        }

        return PatchResult.Ok("Hook is valid");
    }

    /// <summary>
    /// Validates a collection of hooks and checks for overlaps
    /// </summary>
    /// <param name="hooks">Collection of hooks to validate</param>
    /// <returns>Result indicating if all hooks are valid and non-overlapping</returns>
    public static PatchResult ValidateHooks(IEnumerable<Hook> hooks)
    {
        var hookList = hooks.ToList();
        var errors = new List<string>();

        // Validate each hook individually
        for (int i = 0; i < hookList.Count; i++)
        {
            var result = ValidateHook(hookList[i]);
            if (!result.Success)
            {
                errors.Add($"Hook {i} ({hookList[i].Function}): {result.Error}");
            }
        }

        // Check for overlapping hooks
        var overlaps = DetectOverlappingHooks(hookList);
        if (overlaps.Count > 0)
        {
            foreach (var overlap in overlaps)
            {
                errors.Add(overlap);
            }
        }

        if (errors.Count > 0)
        {
            return PatchResult.Fail($"Hook validation failed:\n  - {string.Join("\n  - ", errors)}");
        }

        return PatchResult.Ok($"All {hookList.Count} hooks are valid");
    }

    /// <summary>
    /// Detects overlapping hooks (hooks at the same address)
    /// </summary>
    /// <param name="hooks">Collection of hooks to check</param>
    /// <returns>List of error messages for overlapping hooks</returns>
    public static List<string> DetectOverlappingHooks(IEnumerable<Hook> hooks)
    {
        var errors = new List<string>();
        var hooksByAddress = hooks
            .GroupBy(h => h.Address)
            .Where(g => g.Count() > 1)
            .ToList();

        foreach (var group in hooksByAddress)
        {
            var functions = string.Join(", ", group.Select(h => h.Function));
            errors.Add(
                $"Multiple hooks at address 0x{group.Key:X8}: {functions}"
            );
        }

        return errors;
    }

    /// <summary>
    /// Validates hooks against multiple patches to detect inter-patch conflicts
    /// </summary>
    /// <param name="patches">Dictionary of patch ID to hooks</param>
    /// <returns>Result indicating if there are any conflicts between patches</returns>
    public static PatchResult ValidateMultiPatchHooks(Dictionary<string, List<Hook>> patches)
    {
        var allHooks = new List<(string PatchId, Hook Hook)>();

        // Flatten all hooks with their patch ID
        foreach (var kvp in patches)
        {
            foreach (var hook in kvp.Value)
            {
                allHooks.Add((kvp.Key, hook));
            }
        }

        // Group by address
        var conflicts = allHooks
            .GroupBy(h => h.Hook.Address)
            .Where(g => g.Count() > 1)
            .ToList();

        if (conflicts.Count == 0)
        {
            return PatchResult.Ok("No hook conflicts between patches");
        }

        var errors = new List<string>();
        foreach (var group in conflicts)
        {
            var patchInfo = group.Select(h => $"{h.PatchId}:{h.Hook.Function}");
            errors.Add(
                $"Address 0x{group.Key:X8} used by multiple patches: {string.Join(", ", patchInfo)}"
            );
        }

        return PatchResult.Fail(
            $"Hook conflicts detected:\n  - {string.Join("\n  - ", errors)}"
        );
    }

    /// <summary>
    /// Checks if a hook's function name is valid
    /// </summary>
    /// <param name="functionName">Function name to validate</param>
    /// <returns>True if valid, false otherwise</returns>
    public static bool IsValidFunctionName(string functionName)
    {
        if (string.IsNullOrWhiteSpace(functionName))
            return false;

        // Function name should start with letter or underscore
        if (!char.IsLetter(functionName[0]) && functionName[0] != '_')
            return false;

        // Rest should be alphanumeric or underscore
        return functionName.Skip(1).All(c => char.IsLetterOrDigit(c) || c == '_');
    }

    /// <summary>
    /// Validates that original bytes match what's actually in the executable
    /// </summary>
    /// <param name="hook">Hook to validate</param>
    /// <param name="actualBytes">Actual bytes read from the executable at hook address</param>
    /// <returns>Result indicating if bytes match</returns>
    public static PatchResult ValidateOriginalBytes(Hook hook, byte[] actualBytes)
    {
        if (actualBytes.Length < hook.OriginalBytes.Length)
        {
            return PatchResult.Fail(
                $"Not enough bytes at address 0x{hook.Address:X8} " +
                $"(expected {hook.OriginalBytes.Length}, got {actualBytes.Length})"
            );
        }

        // Compare bytes
        for (int i = 0; i < hook.OriginalBytes.Length; i++)
        {
            if (hook.OriginalBytes[i] != actualBytes[i])
            {
                return PatchResult.Fail(
                    $"Byte mismatch at address 0x{hook.Address + (ulong)i:X8} " +
                    $"(expected 0x{hook.OriginalBytes[i]:X2}, got 0x{actualBytes[i]:X2}). " +
                    $"This may indicate a different game version or an already-patched executable."
                );
            }
        }

        return PatchResult.Ok("Original bytes match");
    }
}
