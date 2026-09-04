using KPatchCore.Models;

namespace KPatchCore.Parsers;

/// <summary>
/// Read/write access to an executable's bytes by virtual address, hiding the on-disk format so
/// install-time byte patching (STATIC hooks) works the same on a Windows PE and a native Linux ELF.
/// </summary>
internal interface IExecutableImage
{
    /// <summary>Reads <paramref name="length"/> bytes located at <paramref name="virtualAddress"/>.</summary>
    PatchResult<byte[]> ReadAtVirtualAddress(ulong virtualAddress, int length);

    /// <summary>Writes <paramref name="bytes"/> at <paramref name="virtualAddress"/>.</summary>
    PatchResult WriteAtVirtualAddress(ulong virtualAddress, byte[] bytes);

    /// <summary>
    /// Finishes the edit. A format whose file carries integrity metadata repairs it here, so this
    /// has to be called once after the last write; a format with nothing to repair succeeds without
    /// touching the file. Writing again afterwards is not supported, because the repair can move
    /// whatever follows the edited bytes.
    /// </summary>
    PatchResult Complete();
}

/// <summary>
/// Opens an executable as the right <see cref="IExecutableImage"/> for its format. This is the single
/// place that decision is made, so callers (the STATIC hook applicator) stay format-agnostic.
/// </summary>
internal static class ExecutableImage
{
    public static PatchResult<IExecutableImage> Open(string exePath)
    {
        var format = ExecutableFormatDetector.Detect(exePath);
        if (!format.Success)
            return PatchResult<IExecutableImage>.Fail(format.Error!);

        return format.Data switch
        {
            ExecutableFormat.Elf => ElfExecutableImage.Open(exePath),
            ExecutableFormat.Pe => PeExecutableImage.Open(exePath),
            ExecutableFormat.MachO => MachOExecutableImage.Open(exePath),
            _ => PatchResult<IExecutableImage>.Fail(
                $"{Path.GetFileName(exePath)} is a {format.Data} executable, which byte patching does not support yet."),
        };
    }
}
