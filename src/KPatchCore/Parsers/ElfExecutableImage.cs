using KPatchCore.Models;
using LibObjectFile.Elf;

namespace KPatchCore.Parsers;

/// <summary>
/// <see cref="IExecutableImage"/> over an ELF executable. Address mapping uses LibObjectFile (the same
/// dependency ElfInjector uses) to walk the PT_LOAD segments: a virtual address maps to a file offset via
/// offset = segment.Position + (va - segment.VirtualAddress) for the segment whose file-backed range
/// covers it. The byte read/write itself is a direct, size-preserving file edit, so it never triggers a
/// layout rewrite of the executable.
/// </summary>
internal sealed class ElfExecutableImage : IExecutableImage
{
    private readonly record struct LoadSegment(ulong VirtualAddress, ulong FileOffset, ulong FileSize);

    private readonly string _exePath;
    private readonly List<LoadSegment> _loads;

    private ElfExecutableImage(string exePath, List<LoadSegment> loads)
    {
        _exePath = exePath;
        _loads = loads;
    }

    public static PatchResult<IExecutableImage> Open(string exePath)
    {
        try
        {
            ElfFile elf;
            using (var stream = File.OpenRead(exePath))
                elf = ElfFile.Read(stream);

            // Position and Size are the on-disk offset/filesz; a VA is file-backed only within Size.
            var loads = elf.Segments
                .Where(s => s.Type == ElfSegmentTypeCore.Load && s.Size > 0)
                .Select(s => new LoadSegment(s.VirtualAddress, s.Position, s.Size))
                .ToList();

            if (loads.Count == 0)
                return PatchResult<IExecutableImage>.Fail($"{Path.GetFileName(exePath)} has no loadable PT_LOAD segments.");

            return PatchResult<IExecutableImage>.Ok(new ElfExecutableImage(exePath, loads));
        }
        catch (Exception ex)
        {
            return PatchResult<IExecutableImage>.Fail($"Failed to parse ELF: {ex.Message}");
        }
    }

    private PatchResult<long> ToFileOffset(uint virtualAddress, int length)
    {
        foreach (var seg in _loads)
        {
            if (virtualAddress < seg.VirtualAddress || virtualAddress >= seg.VirtualAddress + seg.FileSize)
                continue;

            ulong offsetInSegment = virtualAddress - seg.VirtualAddress;
            if (offsetInSegment + (uint)length > seg.FileSize)
                return PatchResult<long>.Fail(
                    $"Virtual address 0x{virtualAddress:X8} + {length} bytes runs past its PT_LOAD file range.");

            return PatchResult<long>.Ok((long)(seg.FileOffset + offsetInSegment));
        }

        return PatchResult<long>.Fail($"Virtual address 0x{virtualAddress:X8} is not mapped by any PT_LOAD segment.");
    }

    public PatchResult<byte[]> ReadAtVirtualAddress(uint virtualAddress, int length)
    {
        if (length <= 0)
            return PatchResult<byte[]>.Fail($"Invalid length: {length}");

        var offset = ToFileOffset(virtualAddress, length);
        if (!offset.Success)
            return PatchResult<byte[]>.Fail(offset.Error!);

        try
        {
            using var stream = File.OpenRead(_exePath);
            stream.Seek(offset.Data, SeekOrigin.Begin);
            var bytes = new byte[length];
            if (stream.Read(bytes, 0, length) != length)
                return PatchResult<byte[]>.Fail($"Short read at 0x{virtualAddress:X8}.");
            return PatchResult<byte[]>.Ok(bytes);
        }
        catch (Exception ex)
        {
            return PatchResult<byte[]>.Fail($"Failed to read bytes: {ex.Message}");
        }
    }

    public PatchResult WriteAtVirtualAddress(uint virtualAddress, byte[] bytes)
    {
        if (bytes == null || bytes.Length == 0)
            return PatchResult.Fail("Bytes cannot be null or empty");

        var offset = ToFileOffset(virtualAddress, bytes.Length);
        if (!offset.Success)
            return PatchResult.Fail(offset.Error!);

        try
        {
            using var stream = File.Open(_exePath, FileMode.Open, FileAccess.ReadWrite);
            stream.Seek(offset.Data, SeekOrigin.Begin);
            stream.Write(bytes, 0, bytes.Length);
            stream.Flush();
            return PatchResult.Ok();
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to write bytes: {ex.Message}");
        }
    }
}
