using KPatchCore.Models;
using KPatchCore.Parsers;

namespace KPatchCore.Validators;

/// <summary>
/// Checks the hooks of a patch selection against the executable they are about to be applied to.
/// </summary>
/// <remarks>
/// Every hook records the bytes its author found at its address. Reading those bytes back out of
/// the game is the only check that catches an address that is simply wrong: a hook aimed at the
/// wrong instruction is well-formed, does not conflict with anything, and fails silently once the
/// game is running.
/// </remarks>
public static class HookTargetValidator
{
    /// <summary>
    /// Verifies that every hook is aimed at an address the executable has, and that the bytes
    /// there are the ones the hook was written against.
    /// </summary>
    /// <param name="exePath">Path to the game executable the hooks will be applied to</param>
    /// <param name="hooksByPatch">Hooks to check, keyed by the patch they came from</param>
    /// <returns>
    /// Failure listing every hook that does not line up, naming the patch each came from. Success
    /// when they all line up, or when the executable is packed and so cannot be read.
    /// </returns>
    public static PatchResult VerifyAgainstExecutable(
        string exePath,
        IReadOnlyDictionary<string, List<Hook>> hooksByPatch)
    {
        // A selection of DLL-only patches has nothing to check, and asking the parser to open the
        // executable for it would make those installs fail on a build it cannot read.
        if (hooksByPatch.Values.All(hooks => hooks.Count == 0))
        {
            return PatchResult.Ok("No hooks to check against the executable");
        }

        var imageResult = ExecutableImage.Open(exePath);
        if (!imageResult.Success || imageResult.Data == null)
        {
            return PatchResult.Fail($"Failed to read executable: {imageResult.Error}");
        }

        var image = imageResult.Data;
        if (image.IsPacked)
        {
            // Nothing readable here is what the game runs. The patcher checks these same bytes in
            // memory once the executable has unpacked itself, which is the only place they exist.
            return PatchResult.Ok(
                $"{Path.GetFileName(exePath)} is packed, so its hooks can only be checked at runtime");
        }

        var errors = new List<string>();
        var verified = 0;

        foreach (var (patchId, hooks) in hooksByPatch)
        {
            foreach (var hook in hooks)
            {
                var addressResult = HookValidator.ValidateHook(hook);
                if (!addressResult.Success)
                {
                    errors.Add($"{patchId}, {Describe(hook)}: {addressResult.Error}");
                    continue;
                }

                var readResult = image.ReadAtVirtualAddress(hook.Address, hook.OriginalBytes.Length);
                if (!readResult.Success || readResult.Data == null)
                {
                    errors.Add($"{patchId}, {Describe(hook)}: {readResult.Error}");
                    continue;
                }

                if (HookValidator.ClassifyBytes(hook, readResult.Data) == HookByteState.Mismatch)
                {
                    errors.Add(
                        $"{patchId}, {Describe(hook)}: expected [{ToHex(hook.OriginalBytes)}], " +
                        $"found [{ToHex(readResult.Data)}]");
                }

                verified++;
            }
        }

        if (errors.Count > 0)
        {
            return PatchResult.Fail(
                "These hooks do not match this copy of the game, so applying them would do nothing " +
                "or break it. The patch needs updating for this build:\n  - " +
                string.Join("\n  - ", errors));
        }

        return PatchResult.Ok($"Verified {verified} hook(s) against {Path.GetFileName(exePath)}");
    }

    private static string Describe(Hook hook) =>
        string.IsNullOrWhiteSpace(hook.Function)
            ? $"{hook.Type} hook at 0x{hook.Address:X8}"
            : $"{hook.Type} hook {hook.Function} at 0x{hook.Address:X8}";

    private static string ToHex(byte[] bytes) => BitConverter.ToString(bytes).Replace("-", " ");
}
