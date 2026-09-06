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

    /// <summary>
    /// The KOTOR PE builds are unsigned, and their header checksum is not enforced for an
    /// executable image, so a byte write leaves nothing to repair.
    /// </summary>
    public PatchResult Complete() => PatchResult.Ok();

    /// <summary>
    /// SteamStub is the only packer these games ship under, and it leaves a .bind section of its
    /// own carrying the entry point. The original .text stays encrypted on disk until the stub
    /// decrypts it at startup. The Steam release of KOTOR 1 is built this way; the GOG release and
    /// the Aspyr builds of KOTOR 2 are not.
    /// </summary>
    public bool IsPacked => _info.Sections.Any(s => s.Name == ".bind");

    /// <summary>
    /// A PE carries no linker-generated build id: these builds ship without a CodeView debug
    /// directory, so there is no PDB signature either. The link timestamp with the image's size and
    /// section count stands in. Two builds that agree on all three have been the same code every
    /// time it has been checked, and the exe patchers seen in the wild leave all three alone.
    /// </summary>
    public string? BuildIdentity =>
        $"pe:{_info.TimeDateStamp:X8}:{_info.SizeOfImage:X}:{_info.Sections.Count}";

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
