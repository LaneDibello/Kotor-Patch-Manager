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

    public PatchResult<byte[]> ReadAtVirtualAddress(uint virtualAddress, int length) =>
        PeHeaderParser.ReadBytesAtVirtualAddress(_exePath, _info, virtualAddress, length);

    public PatchResult WriteAtVirtualAddress(uint virtualAddress, byte[] bytes) =>
        PeHeaderParser.WriteBytesToVirtualAddress(_exePath, _info, virtualAddress, bytes);
}
