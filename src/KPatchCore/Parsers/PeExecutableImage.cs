using KPatchCore.Models;

namespace KPatchCore.Parsers;

/// <summary>
/// <see cref="IExecutableImage"/> over a Windows PE, delegating the address mapping and byte I/O to the
/// existing <see cref="PeHeaderParser"/> so the Windows STATIC hook behaviour is unchanged.
/// </summary>
internal sealed class PeExecutableImage : IExecutableImage
{
    private readonly string _exePath;
    private readonly PeHeaderParser.PeHeaderInfo _info;

    private PeExecutableImage(string exePath, PeHeaderParser.PeHeaderInfo info)
    {
        _exePath = exePath;
        _info = info;
    }

    public static PatchResult<IExecutableImage> Open(string exePath)
    {
        var parsed = PeHeaderParser.ParsePeHeaders(exePath);
        if (!parsed.Success || parsed.Data == null)
            return PatchResult<IExecutableImage>.Fail($"Failed to parse PE headers: {parsed.Error}");

        return PatchResult<IExecutableImage>.Ok(new PeExecutableImage(exePath, parsed.Data));
    }

    public PatchResult<byte[]> ReadAtVirtualAddress(ulong virtualAddress, int length)
    {
        if (!TryNarrow(virtualAddress, out var va, out var error))
            return PatchResult<byte[]>.Fail(error!);

        return PeHeaderParser.ReadBytesAtVirtualAddress(_exePath, _info, va, length);
    }

    public PatchResult WriteAtVirtualAddress(ulong virtualAddress, byte[] bytes)
    {
        if (!TryNarrow(virtualAddress, out var va, out var error))
            return PatchResult.Fail(error!);

        return PeHeaderParser.WriteBytesToVirtualAddress(_exePath, _info, va, bytes);
    }

    /// <summary>
    /// Addresses are carried as 64-bit because a Mach-O image needs it, but the PE
    /// builds of these games are 32-bit. An address that does not fit one is not an
    /// address in this file, so it is refused rather than narrowed into a
    /// plausible-looking one somewhere else in the image.
    /// </summary>
    private static bool TryNarrow(ulong virtualAddress, out uint narrowed, out string? error)
    {
        if (virtualAddress > uint.MaxValue)
        {
            narrowed = 0;
            error = $"Address 0x{virtualAddress:X} is outside the 32-bit range of a PE image.";
            return false;
        }

        narrowed = (uint)virtualAddress;
        error = null;
        return true;
    }
}
