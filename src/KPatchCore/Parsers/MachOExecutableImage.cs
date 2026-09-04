using KPatchCore.Models;
using LibObjectFile.MachO;

namespace KPatchCore.Parsers;

/// <summary>
/// <see cref="IExecutableImage"/> over a Mach-O executable, thin or universal. A virtual address maps
/// to a file offset via offset = slice.FileOffset + segment.FileOffset + (va - segment.VmAddress),
/// where the slice offset is zero for a thin file. The byte read/write is a direct, size-preserving
/// file edit, so it never triggers a layout rewrite of the executable.
/// </summary>
internal sealed class MachOExecutableImage : IExecutableImage
{
    /// <summary>A segment's file-backed bytes, with the offset already relative to the container.</summary>
    private readonly record struct MappedRange(ulong VmAddress, ulong ContainerOffset, ulong FileSize);

    private readonly string _exePath;
    private readonly List<MappedRange> _ranges;
    private readonly bool _isSigned;

    private MachOExecutableImage(string exePath, List<MappedRange> ranges, bool isSigned)
    {
        _exePath = exePath;
        _ranges = ranges;
        _isSigned = isSigned;
    }

    public static PatchResult<IExecutableImage> Open(string exePath)
    {
        try
        {
            var ranges = new List<MappedRange>();
            var isSigned = false;

            using var stream = File.OpenRead(exePath);
            if (MachOFatFile.IsFat(stream))
            {
                stream.Position = 0;
                foreach (var slice in MachOFatFile.Read(stream).Slices)
                {
                    if (slice.File is null)
                        continue;

                    Collect(slice.File, slice.FileOffset, ranges);
                    isSigned |= slice.File.CodeSignature is not null;
                }
            }
            else
            {
                stream.Position = 0;
                var file = MachOFile.Read(stream);
                Collect(file, 0, ranges);
                isSigned = file.CodeSignature is not null;
            }

            if (ranges.Count == 0)
                return PatchResult<IExecutableImage>.Fail($"{Path.GetFileName(exePath)} has no file-backed segments.");

            return PatchResult<IExecutableImage>.Ok(new MachOExecutableImage(exePath, ranges, isSigned));
        }
        catch (Exception ex)
        {
            return PatchResult<IExecutableImage>.Fail($"Failed to parse Mach-O: {ex.Message}");
        }
    }

    /// <summary>
    /// Adds the segments that actually occupy bytes in the file. A segment with no file content maps
    /// nothing, which drops __PAGEZERO: it claims the whole 4 GB below the image and would otherwise
    /// swallow every address handed to it.
    /// </summary>
    private static void Collect(MachOFile file, ulong sliceOffset, List<MappedRange> into)
    {
        foreach (var segment in file.Segments)
        {
            if (segment.FileSize == 0)
                continue;

            into.Add(new MappedRange(segment.VmAddress, sliceOffset + segment.FileOffset, segment.FileSize));
        }
    }

    private PatchResult<long> ToFileOffset(ulong virtualAddress, int length)
    {
        MappedRange? match = null;

        foreach (var range in _ranges)
        {
            if (virtualAddress < range.VmAddress || virtualAddress >= range.VmAddress + range.FileSize)
                continue;

            // The slices of a universal binary do not share an address space: a 64-bit slice sits
            // above its 4 GB __PAGEZERO while a 32-bit one starts just above a 4 KB one, so an
            // address names exactly one of them and no architecture has to be given. Should a build
            // ever break that, the mapping is refused rather than guessed, because writing to the
            // wrong slice would put one instruction set's bytes over another's.
            if (match is not null)
                return PatchResult<long>.Fail(
                    $"Virtual address 0x{virtualAddress:X8} is claimed by more than one slice of " +
                    $"{Path.GetFileName(_exePath)}, so which one to patch is ambiguous.");

            match = range;
        }

        if (match is null)
            return PatchResult<long>.Fail($"Virtual address 0x{virtualAddress:X8} is not mapped by any segment.");

        var found = match.Value;
        ulong offsetInSegment = virtualAddress - found.VmAddress;
        if (offsetInSegment + (uint)length > found.FileSize)
            return PatchResult<long>.Fail(
                $"Virtual address 0x{virtualAddress:X8} + {length} bytes runs past its segment's file range.");

        return PatchResult<long>.Ok((long)(found.ContainerOffset + offsetInSegment));
    }

    public PatchResult<byte[]> ReadAtVirtualAddress(ulong virtualAddress, int length)
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

    public PatchResult WriteAtVirtualAddress(ulong virtualAddress, byte[] bytes)
    {
        if (bytes == null || bytes.Length == 0)
            return PatchResult.Fail("Bytes cannot be null or empty");

        // The kernel checks a signed binary's signature when it execs it, not just when Gatekeeper
        // assesses it, so editing one produces a game that will not start. K1's KOTOR_Exe is signed;
        // K2's KOTOR2sub is not.
        if (_isSigned)
            return PatchResult.Fail(
                $"{Path.GetFileName(_exePath)} carries a code signature, and editing its bytes would " +
                "leave a binary the kernel refuses to run.");

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
