using KPatchCore.Models;

namespace KPatchCore.Parsers;

/// <summary>
/// Read/write access to an executable's bytes by virtual address, hiding the on-disk format so
/// install-time byte patching (STATIC hooks) works the same on a Windows PE and a native Linux ELF.
/// </summary>
internal interface IExecutableImage
{
    /// <summary>Reads <paramref name="length"/> bytes located at <paramref name="virtualAddress"/>.</summary>
    PatchResult<byte[]> ReadAtVirtualAddress(uint virtualAddress, int length);

    /// <summary>Writes <paramref name="bytes"/> at <paramref name="virtualAddress"/>.</summary>
    PatchResult WriteAtVirtualAddress(uint virtualAddress, byte[] bytes);
}

/// <summary>
/// Opens an executable as the right <see cref="IExecutableImage"/> for its format. This is the single
/// place the PE-vs-ELF decision is made, so callers (the STATIC hook applicator) stay format-agnostic.
/// </summary>
internal static class ExecutableImage
{
    public static PatchResult<IExecutableImage> Open(string exePath)
    {
        if (!File.Exists(exePath))
            return PatchResult<IExecutableImage>.Fail($"Executable not found: {exePath}");

        var magic = new byte[4];
        try
        {
            using var stream = File.OpenRead(exePath);
            if (stream.Read(magic, 0, magic.Length) < magic.Length)
                return PatchResult<IExecutableImage>.Fail($"{Path.GetFileName(exePath)} is too small to be an executable.");
        }
        catch (Exception ex)
        {
            return PatchResult<IExecutableImage>.Fail($"Failed to read {Path.GetFileName(exePath)}: {ex.Message}");
        }

        // ELF starts with 0x7F 'E' 'L' 'F'; a PE (DOS stub) starts with 'M' 'Z'.
        if (magic[0] == 0x7F && magic[1] == (byte)'E' && magic[2] == (byte)'L' && magic[3] == (byte)'F')
            return ElfExecutableImage.Open(exePath);
        if (magic[0] == (byte)'M' && magic[1] == (byte)'Z')
            return PeExecutableImage.Open(exePath);

        return PatchResult<IExecutableImage>.Fail(
            $"{Path.GetFileName(exePath)} is neither a PE nor an ELF executable.");
    }
}
