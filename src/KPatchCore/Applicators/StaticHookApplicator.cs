using KPatchCore.Models;
using KPatchCore.Parsers;

namespace KPatchCore.Applicators;

/// <summary>
/// Applies STATIC hooks to executable files at install-time
/// </summary>
public static class StaticHookApplicator
{
    /// <summary>
    /// Applies static hooks to an executable file
    /// </summary>
    /// <param name="exePath">Path to executable to patch</param>
    /// <param name="hooks">Hooks to apply (will filter to only STATIC hooks)</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult ApplyStaticHooks(string exePath, List<Hook> hooks)
    {
        if (string.IsNullOrWhiteSpace(exePath))
        {
            return PatchResult.Fail("Executable path cannot be null or empty");
        }

        if (!File.Exists(exePath))
        {
            return PatchResult.Fail($"Executable not found: {exePath}");
        }

        // Filter to only STATIC hooks
        var staticHooks = hooks.Where(h => h.Type == HookType.Static).ToList();
        if (staticHooks.Count == 0)
        {
            return PatchResult.Ok("No static hooks to apply");
        }

        // Open the executable once. The image maps virtual addresses to file offsets for whichever
        // format this is (Windows PE or native Linux ELF), so the apply loop stays format-agnostic.
        var imageResult = ExecutableImage.Open(exePath);
        if (!imageResult.Success || imageResult.Data == null)
        {
            return PatchResult.Fail($"Failed to read executable: {imageResult.Error}");
        }

        var image = imageResult.Data;
        var errors = new List<string>();
        var appliedCount = 0;

        foreach (var hook in staticHooks)
        {
            // Read current bytes at location
            var readResult = image.ReadAtVirtualAddress(hook.Address, hook.OriginalBytes.Length);
            if (!readResult.Success || readResult.Data == null)
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: Failed to read bytes: {readResult.Error}");
                continue;
            }

            // Verify original bytes match. If the replacement bytes are already present,
            // treat this hook as already applied. This keeps KPM-managed reapply flows from
            // failing solely because a STATIC patch previously changed the executable.
            var actualBytes = readResult.Data;
            if (!hook.OriginalBytes.SequenceEqual(actualBytes))
            {
                if (hook.ReplacementBytes != null && hook.ReplacementBytes.SequenceEqual(actualBytes))
                {
                    appliedCount++;
                    continue;
                }

                var expectedHex = BitConverter.ToString(hook.OriginalBytes).Replace("-", " ");
                var actualHex = BitConverter.ToString(actualBytes).Replace("-", " ");
                errors.Add($"Hook at 0x{hook.Address:X8}: Byte mismatch - expected [{expectedHex}], got [{actualHex}]");
                continue;
            }

            // Write replacement bytes
            var writeResult = image.WriteAtVirtualAddress(hook.Address, hook.ReplacementBytes!);
            if (!writeResult.Success)
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: Failed to write bytes: {writeResult.Error}");
                continue;
            }

            appliedCount++;
        }

        // If any errors occurred, return failure
        if (errors.Count > 0)
        {
            return PatchResult.Fail(
                $"Failed to apply {errors.Count}/{staticHooks.Count} static hook(s):\n  - {string.Join("\n  - ", errors)}");
        }

        return PatchResult.Ok($"Successfully applied {appliedCount} static hook(s) to {Path.GetFileName(exePath)}");
    }
}
