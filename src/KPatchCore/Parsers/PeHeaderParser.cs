using KPatchCore.Models;
using System.Text;

namespace KPatchCore.Parsers;

/// <summary>
/// Parses PE (Portable Executable) file headers and provides virtual address to file offset conversion
/// </summary>
public static class PeHeaderParser
{
    /// <summary>
    /// Represents a PE section with address mapping information
    /// </summary>
    public sealed class PeSection
    {
        /// <summary>
        /// Section name (e.g., ".text", ".data")
        /// </summary>
        public required string Name { get; init; }

        /// <summary>
        /// Virtual address where section is loaded in memory
        /// </summary>
        public required uint VirtualAddress { get; init; }

        /// <summary>
        /// Size of section when loaded in memory
        /// </summary>
        public required uint VirtualSize { get; init; }

        /// <summary>
        /// File offset where section data begins
        /// </summary>
        public required uint PointerToRawData { get; init; }

        /// <summary>
        /// Size of section data in file
        /// </summary>
        public required uint SizeOfRawData { get; init; }

        public override string ToString() =>
            $"{Name}: VA=0x{VirtualAddress:X8}, VSize=0x{VirtualSize:X}, RawPtr=0x{PointerToRawData:X}, RawSize=0x{SizeOfRawData:X}";
    }

    /// <summary>
    /// PE header information including section mappings
    /// </summary>
    public sealed class PeHeaderInfo
    {
        /// <summary>
        /// Offset to PE header in file (e_lfanew)
        /// </summary>
        public required int PeHeaderOffset { get; init; }

        /// <summary>
        /// Machine type from COFF header
        /// </summary>
        public required ushort MachineType { get; init; }

        /// <summary>
        /// List of sections in the PE file
        /// </summary>
        public required List<PeSection> Sections { get; init; }

        /// <summary>
        /// Whether this is a 32-bit executable
        /// </summary>
        public required bool Is32Bit { get; init; }

        /// <summary>
        /// Image base address where the executable is loaded in memory
        /// </summary>
        public required ulong ImageBase { get; init; }
    }

    // PE constants
    private const ushort IMAGE_FILE_MACHINE_I386 = 0x014C;  // x86
    private const ushort IMAGE_FILE_MACHINE_AMD64 = 0x8664; // x64
    private const int SECTION_HEADER_SIZE = 40; // IMAGE_SECTION_HEADER size

    /// <summary>
    /// Parses PE headers including section information
    /// </summary>
    /// <param name="exePath">Path to executable file</param>
    /// <returns>PE header information or error</returns>
    public static PatchResult<PeHeaderInfo> ParsePeHeaders(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<PeHeaderInfo>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            using var stream = File.OpenRead(exePath);
            using var reader = new BinaryReader(stream);

            // Read DOS header
            var dosSignature = reader.ReadUInt16();
            if (dosSignature != 0x5A4D) // "MZ"
            {
                return PatchResult<PeHeaderInfo>.Fail(
                    $"Invalid DOS signature: expected MZ, got 0x{dosSignature:X4}");
            }

            // Jump to e_lfanew (PE header offset at 0x3C)
            stream.Seek(0x3C, SeekOrigin.Begin);
            var peHeaderOffset = reader.ReadInt32();

            if (peHeaderOffset < 0 || peHeaderOffset > stream.Length - 4)
            {
                return PatchResult<PeHeaderInfo>.Fail(
                    $"Invalid PE header offset: 0x{peHeaderOffset:X}");
            }

            // Read PE signature
            stream.Seek(peHeaderOffset, SeekOrigin.Begin);
            var peSignature = reader.ReadUInt32();
            if (peSignature != 0x00004550) // "PE\0\0"
            {
                return PatchResult<PeHeaderInfo>.Fail(
                    $"Invalid PE signature: expected PE\\0\\0, got 0x{peSignature:X8}");
            }

            // Read COFF header (IMAGE_FILE_HEADER)
            var machineType = reader.ReadUInt16();
            var numberOfSections = reader.ReadUInt16();

            // Skip: TimeDateStamp (4), PointerToSymbolTable (4), NumberOfSymbols (4)
            stream.Seek(12, SeekOrigin.Current);

            var sizeOfOptionalHeader = reader.ReadUInt16();
            var characteristics = reader.ReadUInt16();

            // Determine architecture
            var is32Bit = machineType == IMAGE_FILE_MACHINE_I386;
            var is64Bit = machineType == IMAGE_FILE_MACHINE_AMD64;

            if (!is32Bit && !is64Bit)
            {
                return PatchResult<PeHeaderInfo>.Fail(
                    $"Unsupported architecture: machine type 0x{machineType:X4}");
            }

            // Read ImageBase from Optional Header
            // We're currently at the start of the Optional Header
            var optionalHeaderStart = stream.Position;

            // Read magic to verify PE32 vs PE32+
            var magic = reader.ReadUInt16();
            ulong imageBase;

            if (magic == 0x10B) // PE32 (32-bit)
            {
                // ImageBase is at offset 28 in PE32 Optional Header
                stream.Seek(optionalHeaderStart + 28, SeekOrigin.Begin);
                imageBase = reader.ReadUInt32();
            }
            else if (magic == 0x20B) // PE32+ (64-bit)
            {
                // ImageBase is at offset 24 in PE32+ Optional Header
                stream.Seek(optionalHeaderStart + 24, SeekOrigin.Begin);
                imageBase = reader.ReadUInt64();
            }
            else
            {
                return PatchResult<PeHeaderInfo>.Fail(
                    $"Unknown Optional Header magic: 0x{magic:X4}");
            }

            // Skip to end of optional header (after section headers)
            stream.Seek(optionalHeaderStart + sizeOfOptionalHeader, SeekOrigin.Begin);

            // Read section headers
            var sections = new List<PeSection>();
            for (int i = 0; i < numberOfSections; i++)
            {
                var sectionResult = ReadSectionHeader(reader);
                if (!sectionResult.Success || sectionResult.Data == null)
                {
                    return PatchResult<PeHeaderInfo>.Fail(
                        $"Failed to read section {i}: {sectionResult.Error}");
                }

                sections.Add(sectionResult.Data);
            }

            var peInfo = new PeHeaderInfo
            {
                PeHeaderOffset = peHeaderOffset,
                MachineType = machineType,
                Sections = sections,
                Is32Bit = is32Bit,
                ImageBase = imageBase
            };

            return PatchResult<PeHeaderInfo>.Ok(
                peInfo,
                $"Parsed PE with {sections.Count} section(s)");
        }
        catch (Exception ex)
        {
            return PatchResult<PeHeaderInfo>.Fail(
                $"Failed to parse PE headers: {ex.Message}");
        }
    }

    /// <summary>
    /// Reads a single section header (IMAGE_SECTION_HEADER)
    /// </summary>
    private static PatchResult<PeSection> ReadSectionHeader(BinaryReader reader)
    {
        try
        {
            // Name (8 bytes, null-terminated)
            var nameBytes = reader.ReadBytes(8);
            var name = Encoding.ASCII.GetString(nameBytes).TrimEnd('\0');

            // VirtualSize (4 bytes) - part of Misc union at offset +8
            var virtualSize = reader.ReadUInt32();

            // VirtualAddress (4 bytes) at offset +12
            var virtualAddress = reader.ReadUInt32();

            // SizeOfRawData (4 bytes) at offset +16
            var sizeOfRawData = reader.ReadUInt32();

            // PointerToRawData (4 bytes) at offset +20
            var pointerToRawData = reader.ReadUInt32();

            // Skip remaining fields (we don't need them):
            // PointerToRelocations (4), PointerToLinenumbers (4),
            // NumberOfRelocations (2), NumberOfLinenumbers (2),
            // Characteristics (4)
            // Total: 16 bytes
            reader.BaseStream.Seek(16, SeekOrigin.Current);

            var section = new PeSection
            {
                Name = name,
                VirtualAddress = virtualAddress,
                VirtualSize = virtualSize,
                PointerToRawData = pointerToRawData,
                SizeOfRawData = sizeOfRawData
            };

            return PatchResult<PeSection>.Ok(section, $"Read section {name}");
        }
        catch (Exception ex)
        {
            return PatchResult<PeSection>.Fail($"Failed to read section header: {ex.Message}");
        }
    }

    /// <summary>
    /// Converts a virtual address to a file offset using section mappings
    /// </summary>
    /// <param name="peInfo">PE header information</param>
    /// <param name="virtualAddress">Virtual address to convert</param>
    /// <returns>File offset or error</returns>
    public static PatchResult<uint> VirtualAddressToFileOffset(
        PeHeaderInfo peInfo,
        uint virtualAddress)
    {
        if (peInfo == null)
        {
            return PatchResult<uint>.Fail("PE header info cannot be null");
        }

        // Find the first section (sections are typically ordered by VirtualAddress)
        var firstSection = peInfo.Sections.OrderBy(s => s.VirtualAddress).FirstOrDefault();

        // Check if virtual address is in the header region (before first section)
        // Headers are mapped 1:1 from file to memory
        if (firstSection != null && virtualAddress < peInfo.ImageBase + firstSection.VirtualAddress)
        {
            // Address is in PE header region
            if (virtualAddress < peInfo.ImageBase)
            {
                return PatchResult<uint>.Fail(
                    $"Virtual address 0x{virtualAddress:X8} is below image base 0x{peInfo.ImageBase:X}");
            }

            var fileOffset = (uint)(virtualAddress - peInfo.ImageBase);
            return PatchResult<uint>.Ok(
                fileOffset,
                $"VA 0x{virtualAddress:X8} -> File offset 0x{fileOffset:X} (PE header)");
        }

        // Search for the section containing this virtual address
        foreach (var section in peInfo.Sections)
        {
            var sectionStart = peInfo.ImageBase + section.VirtualAddress;
            var sectionEnd = sectionStart + section.VirtualSize;

            if (virtualAddress >= sectionStart && virtualAddress < sectionEnd)
            {
                // Calculate offset within section
                var offsetInSection = virtualAddress - sectionStart;

                // Verify offset is within raw data bounds
                if (offsetInSection < section.SizeOfRawData)
                {
                    var fileOffset = section.PointerToRawData + (uint)offsetInSection;
                    return PatchResult<uint>.Ok(
                        fileOffset,
                        $"VA 0x{virtualAddress:X8} -> File offset 0x{fileOffset:X} (section {section.Name})");
                }
                else
                {
                    return PatchResult<uint>.Fail(
                        $"Virtual address 0x{virtualAddress:X8} is in virtual-only portion of section {section.Name} " +
                        $"(offset 0x{offsetInSection:X} >= raw data size 0x{section.SizeOfRawData:X})");
                }
            }
        }

        // Not found in any section
        return PatchResult<uint>.Fail(
            $"Virtual address 0x{virtualAddress:X8} not found in any section");
    }

    /// <summary>
    /// Reads bytes from file at a virtual address
    /// </summary>
    /// <param name="exePath">Path to executable file</param>
    /// <param name="peInfo">PE header information</param>
    /// <param name="virtualAddress">Virtual address to read from</param>
    /// <param name="length">Number of bytes to read</param>
    /// <returns>Bytes read or error</returns>
    public static PatchResult<byte[]> ReadBytesAtVirtualAddress(
        string exePath,
        PeHeaderInfo peInfo,
        uint virtualAddress,
        int length)
    {
        if (length <= 0)
        {
            return PatchResult<byte[]>.Fail($"Invalid length: {length}");
        }

        // Convert virtual address to file offset
        var offsetResult = VirtualAddressToFileOffset(peInfo, virtualAddress);
        if (!offsetResult.Success)
        {
            return PatchResult<byte[]>.Fail(
                $"Failed to convert VA to offset: {offsetResult.Error}");
        }

        var fileOffset = offsetResult.Data!;

        try
        {
            using var stream = File.OpenRead(exePath);

            if (fileOffset + length > stream.Length)
            {
                return PatchResult<byte[]>.Fail(
                    $"Read would exceed file bounds: offset 0x{fileOffset:X} + length {length} > file size {stream.Length}");
            }

            stream.Seek(fileOffset, SeekOrigin.Begin);
            var bytes = new byte[length];
            var bytesRead = stream.Read(bytes, 0, length);

            if (bytesRead != length)
            {
                return PatchResult<byte[]>.Fail(
                    $"Failed to read all bytes: expected {length}, got {bytesRead}");
            }

            return PatchResult<byte[]>.Ok(
                bytes,
                $"Read {length} byte(s) from VA 0x{virtualAddress:X8}");
        }
        catch (Exception ex)
        {
            return PatchResult<byte[]>.Fail(
                $"Failed to read bytes: {ex.Message}");
        }
    }

    /// <summary>
    /// Writes bytes to file at a virtual address
    /// </summary>
    /// <param name="exePath">Path to executable file</param>
    /// <param name="peInfo">PE header information</param>
    /// <param name="virtualAddress">Virtual address to write to</param>
    /// <param name="bytes">Bytes to write</param>
    /// <returns>Success or error</returns>
    public static PatchResult WriteBytesToVirtualAddress(
        string exePath,
        PeHeaderInfo peInfo,
        uint virtualAddress,
        byte[] bytes)
    {
        if (bytes == null || bytes.Length == 0)
        {
            return PatchResult.Fail("Bytes cannot be null or empty");
        }

        // Convert virtual address to file offset
        var offsetResult = VirtualAddressToFileOffset(peInfo, virtualAddress);
        if (!offsetResult.Success)
        {
            return PatchResult.Fail(
                $"Failed to convert VA to offset: {offsetResult.Error}");
        }

        var fileOffset = offsetResult.Data!;

        try
        {
            using var stream = File.Open(exePath, FileMode.Open, FileAccess.ReadWrite);

            if (fileOffset + bytes.Length > stream.Length)
            {
                return PatchResult.Fail(
                    $"Write would exceed file bounds: offset 0x{fileOffset:X} + length {bytes.Length} > file size {stream.Length}");
            }

            stream.Seek(fileOffset, SeekOrigin.Begin);
            stream.Write(bytes, 0, bytes.Length);
            stream.Flush();

            return PatchResult.Ok(
                $"Wrote {bytes.Length} byte(s) to VA 0x{virtualAddress:X8}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail(
                $"Failed to write bytes: {ex.Message}");
        }
    }
}
