using KPatchCore.Models;

namespace KPatchCore.Parsers;

/// <summary>The on-disk container an executable uses.</summary>
internal enum ExecutableFormat
{
    /// <summary>Windows Portable Executable.</summary>
    Pe,

    /// <summary>Linux ELF.</summary>
    Elf,

    /// <summary>macOS Mach-O, thin or universal.</summary>
    MachO,
}

/// <summary>
/// Identifies an executable's container from its first four bytes. Both the byte-patching and the
/// dependency-editing sides need this, and neither should carry its own copy of the magic numbers.
/// </summary>
internal static class ExecutableFormatDetector
{
    public static PatchResult<ExecutableFormat> Detect(string exePath)
    {
        if (!File.Exists(exePath))
            return PatchResult<ExecutableFormat>.Fail($"Executable not found: {exePath}");

        var magic = new byte[4];
        try
        {
            using var stream = File.OpenRead(exePath);
            if (stream.Read(magic, 0, magic.Length) < magic.Length)
                return PatchResult<ExecutableFormat>.Fail(
                    $"{Path.GetFileName(exePath)} is too small to be an executable.");
        }
        catch (Exception ex)
        {
            return PatchResult<ExecutableFormat>.Fail(
                $"Failed to read {Path.GetFileName(exePath)}: {ex.Message}");
        }

        // ELF starts with 0x7F 'E' 'L' 'F'; a PE starts with the DOS stub's 'M' 'Z'.
        if (magic[0] == 0x7F && magic[1] == (byte)'E' && magic[2] == (byte)'L' && magic[3] == (byte)'F')
            return PatchResult<ExecutableFormat>.Ok(ExecutableFormat.Elf);
        if (magic[0] == (byte)'M' && magic[1] == (byte)'Z')
            return PatchResult<ExecutableFormat>.Ok(ExecutableFormat.Pe);
        if (IsMachOMagic(magic))
            return PatchResult<ExecutableFormat>.Ok(ExecutableFormat.MachO);

        return PatchResult<ExecutableFormat>.Fail(
            $"{Path.GetFileName(exePath)} is not a PE, ELF or Mach-O executable.");
    }

    // Mach-O comes in four flavours here: a thin image in either byte order (0xFEEDFACE 32-bit,
    // 0xFEEDFACF 64-bit) and a universal binary (0xCAFEBABE, or 0xCAFEBABF for 64-bit offsets).
    // The fat header is always big-endian, whatever the images inside it are.
    private static bool IsMachOMagic(byte[] magic)
    {
        uint le = (uint)(magic[0] | magic[1] << 8 | magic[2] << 16 | magic[3] << 24);
        uint be = (uint)(magic[3] | magic[2] << 8 | magic[1] << 16 | magic[0] << 24);
        return le is 0xFEEDFACE or 0xFEEDFACF
            || be is 0xFEEDFACE or 0xFEEDFACF
            || be is 0xCAFEBABE or 0xCAFEBABF;
    }
}
