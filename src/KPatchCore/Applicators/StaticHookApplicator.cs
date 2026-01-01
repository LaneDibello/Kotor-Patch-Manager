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

        // Parse PE headers once for all hooks
        var peResult = PeHeaderParser.ParsePeHeaders(exePath);
        if (!peResult.Success || peResult.Data == null)
        {
            return PatchResult.Fail($"Failed to parse PE headers: {peResult.Error}");
        }

        var peInfo = peResult.Data;
        var errors = new List<string>();
        var appliedCount = 0;

        foreach (var hook in staticHooks)
        {
            // Convert virtual address to file offset
            var offsetResult = PeHeaderParser.VirtualAddressToFileOffset(peInfo, hook.Address);
            if (!offsetResult.Success)
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: {offsetResult.Error}");
                continue;
            }

            var fileOffset = offsetResult.Data!;

            // Read current bytes at location
            var readResult = PeHeaderParser.ReadBytesAtVirtualAddress(
                exePath,
                peInfo,
                hook.Address,
                hook.OriginalBytes.Length);

            if (!readResult.Success || readResult.Data == null)
            {
                errors.Add($"Hook at 0x{hook.Address:X8}: Failed to read bytes: {readResult.Error}");
                continue;
            }

            // Verify original bytes match
            var actualBytes = readResult.Data;
            if (!hook.OriginalBytes.SequenceEqual(actualBytes))
            {
                var expectedHex = BitConverter.ToString(hook.OriginalBytes).Replace("-", " ");
                var actualHex = BitConverter.ToString(actualBytes).Replace("-", " ");
                errors.Add($"Hook at 0x{hook.Address:X8}: Byte mismatch - expected [{expectedHex}], got [{actualHex}]");
                continue;
            }

            // Write replacement bytes
            var writeResult = PeHeaderParser.WriteBytesToVirtualAddress(
                exePath,
                peInfo,
                hook.Address,
                hook.ReplacementBytes!);

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
