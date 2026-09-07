using System.Buffers.Binary;

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

/// <summary>What an executable is built to run as: its container, and the CPU inside it.</summary>
internal readonly record struct ExecutableTarget(ExecutableFormat Format, Architecture Architecture);

/// <summary>
/// Identifies an executable's container and CPU from its headers. Both the byte-patching and the
/// dependency-editing sides need this, and neither should carry its own copy of the magic numbers.
/// </summary>
/// <remarks>
/// Read by hand rather than through LibObjectFile, which has no PE counterpart to ElfFile.IsElf
/// and never surfaces the COFF header.
/// </remarks>
internal static class ExecutableFormatDetector
{
    // IMAGE_FILE_MACHINE_*, the COFF header's Machine field.
    private const ushort PeMachineI386 = 0x014C;
    private const ushort PeMachineAmd64 = 0x8664;
    private const ushort PeMachineArm64 = 0xAA64;

    // e_machine values from the ELF specification.
    private const ushort ElfMachineI386 = 0x03;
    private const ushort ElfMachineX8664 = 0x3E;
    private const ushort ElfMachineAArch64 = 0xB7;

    // cpu_type_t. The 0x01000000 bit is CPU_ARCH_ABI64, which marks the 64-bit form of a type.
    private const int MachOCpuX86 = 7;
    private const int MachOCpuX8664 = 7 | 0x0100_0000;
    private const int MachOCpuArm64 = 12 | 0x0100_0000;

    // A thin image is stored in its own byte order; a fat header is always big-endian.
    private const uint MachOThin32 = 0xFEEDFACE;
    private const uint MachOThin64 = 0xFEEDFACF;
    private const uint MachOFat32 = 0xCAFEBABE;
    private const uint MachOFat64 = 0xCAFEBABF;

    /// <summary>
    /// The container alone. Fails on everything <see cref="DetectTarget"/> does, including a CPU
    /// the manager cannot patch.
    /// </summary>
    public static PatchResult<ExecutableFormat> Detect(string exePath)
    {
        var target = DetectTarget(exePath);
        return target.Success
            ? PatchResult<ExecutableFormat>.Ok(target.Data.Format)
            : PatchResult<ExecutableFormat>.Fail(target.Error!);
    }

    /// <summary>
    /// Reads the container and the CPU it targets. Fails on anything that is not one of the three
    /// formats, or whose CPU is not one the manager knows how to patch.
    /// </summary>
    public static PatchResult<ExecutableTarget> DetectTarget(string exePath)
    {
        if (!File.Exists(exePath))
            return PatchResult<ExecutableTarget>.Fail($"Executable not found: {exePath}");

        try
        {
            using var stream = File.OpenRead(exePath);
            using var reader = new BinaryReader(stream);

            var magic = reader.ReadBytes(4);
            if (magic.Length < 4)
                return PatchResult<ExecutableTarget>.Fail(
                    $"{Path.GetFileName(exePath)} is too small to be an executable.");

            if (IsElfMagic(magic))
                return Describe(exePath, ExecutableFormat.Elf, ElfCpu(reader));
            if (IsDosMagic(magic))
            {
                // A DOS-only program has an MZ header too.
                if (!HasPeImage(reader))
                    return PatchResult<ExecutableTarget>.Fail(
                        $"{Path.GetFileName(exePath)} has a DOS header but no PE image behind it.");

                return Describe(exePath, ExecutableFormat.Pe, PeCpu(reader));
            }
            if (IsMachOMagic(magic))
                return Describe(exePath, ExecutableFormat.MachO, MachOCpu(reader, magic));

            return PatchResult<ExecutableTarget>.Fail(
                $"{Path.GetFileName(exePath)} is not a PE, ELF or Mach-O executable.");
        }
        catch (Exception ex)
        {
            return PatchResult<ExecutableTarget>.Fail(
                $"Failed to read {Path.GetFileName(exePath)}: {ex.Message}");
        }
    }

    private static PatchResult<ExecutableTarget> Describe(
        string exePath, ExecutableFormat format, Architecture? cpu)
    {
        return cpu is { } architecture
            ? PatchResult<ExecutableTarget>.Ok(new ExecutableTarget(format, architecture))
            : PatchResult<ExecutableTarget>.Fail(
                $"{Path.GetFileName(exePath)} is built for a CPU this manager does not patch.");
    }

    private static bool HasPeImage(BinaryReader reader)
    {
        // e_lfanew, the offset to the PE signature.
        reader.BaseStream.Position = 0x3C;
        reader.BaseStream.Position = reader.ReadUInt32();

        var signature = reader.ReadBytes(4);
        return signature.Length == 4 && signature[0] == (byte)'P' && signature[1] == (byte)'E' &&
               signature[2] == 0 && signature[3] == 0;
    }

    private static Architecture? PeCpu(BinaryReader reader)
    {
        // HasPeImage left the stream on the COFF header, and Machine comes first.
        return reader.ReadUInt16() switch
        {
            PeMachineI386 => Architecture.x86,
            PeMachineAmd64 => Architecture.x86_64,
            PeMachineArm64 => Architecture.ARM64,
            _ => null,
        };
    }

    private static Architecture? ElfCpu(BinaryReader reader)
    {
        // EI_DATA, and BinaryReader only reads little-endian.
        reader.BaseStream.Position = 5;
        var littleEndian = reader.ReadByte() == 1;

        reader.BaseStream.Position = 0x12;  // e_machine
        var machine = reader.ReadUInt16();
        if (!littleEndian)
            machine = BinaryPrimitives.ReverseEndianness(machine);

        return machine switch
        {
            ElfMachineI386 => Architecture.x86,
            ElfMachineX8664 => Architecture.x86_64,
            ElfMachineAArch64 => Architecture.ARM64,
            _ => null,
        };
    }

    private static Architecture? MachOCpu(BinaryReader reader, byte[] magic)
    {
        var be = BinaryPrimitives.ReadUInt32BigEndian(magic);

        if (be is MachOFat32 or MachOFat64)
        {
            var sliceCount = BinaryPrimitives.ReverseEndianness(reader.ReadUInt32());

            // fat_arch is 20 bytes and fat_arch_64 is 32, and cpu_type_t leads both.
            var stride = be == MachOFat64 ? 32 : 20;
            var slices = new List<Architecture>();

            for (long i = 0; i < sliceCount; i++)
            {
                reader.BaseStream.Position = 8 + (i * stride);
                if (MachOArchitecture(BinaryPrimitives.ReverseEndianness(reader.ReadInt32())) is { } slice)
                    slices.Add(slice);
            }

            // A Mac runs the newest slice it can, which is not always the first one.
            if (slices.Contains(Architecture.ARM64))
                return Architecture.ARM64;
            if (slices.Contains(Architecture.x86_64))
                return Architecture.x86_64;
            return slices.Contains(Architecture.x86) ? Architecture.x86 : null;
        }

        // A thin image stores cpu_type_t right after the magic, in the image's own byte order.
        var cpu = reader.ReadInt32();
        return MachOArchitecture(BinaryPrimitives.ReadUInt32LittleEndian(magic) is MachOThin32 or MachOThin64
            ? cpu
            : BinaryPrimitives.ReverseEndianness(cpu));
    }

    private static Architecture? MachOArchitecture(int cpuType) => cpuType switch
    {
        MachOCpuX86 => Architecture.x86,
        MachOCpuX8664 => Architecture.x86_64,
        MachOCpuArm64 => Architecture.ARM64,
        _ => null,
    };

    private static bool IsElfMagic(byte[] magic) =>
        magic[0] == 0x7F && magic[1] == (byte)'E' && magic[2] == (byte)'L' && magic[3] == (byte)'F';

    // Necessary for a PE, not sufficient: see HasPeImage.
    private static bool IsDosMagic(byte[] magic) =>
        magic[0] == (byte)'M' && magic[1] == (byte)'Z';

    private static bool IsMachOMagic(byte[] magic) =>
        BinaryPrimitives.ReadUInt32LittleEndian(magic) is MachOThin32 or MachOThin64
        || BinaryPrimitives.ReadUInt32BigEndian(magic) is MachOThin32 or MachOThin64 or MachOFat32 or MachOFat64;
}
