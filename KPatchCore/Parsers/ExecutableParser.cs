using KPatchCore.Models;

namespace KPatchCore.Parsers;

/// <summary>
/// Parses PE (Portable Executable) files to extract basic metadata
/// </summary>
/// <remarks>
/// Simplified implementation without external PE libraries.
/// Only extracts essential information needed for patch validation.
/// </remarks>
public static class ExecutableParser
{
    /// <summary>
    /// Information extracted from a PE file
    /// </summary>
    public sealed class ExecutableInfo
    {
        /// <summary>
        /// File size in bytes
        /// </summary>
        public required long FileSize { get; init; }

        /// <summary>
        /// Whether this is a 32-bit (x86) executable
        /// </summary>
        public required bool Is32Bit { get; init; }

        /// <summary>
        /// Whether this is a 64-bit (x86_64) executable
        /// </summary>
        public required bool Is64Bit { get; init; }

        /// <summary>
        /// Machine type from PE header
        /// </summary>
        public required ushort MachineType { get; init; }
    }

    // PE constants
    private const ushort IMAGE_FILE_MACHINE_I386 = 0x014C;  // x86
    private const ushort IMAGE_FILE_MACHINE_AMD64 = 0x8664; // x64

    /// <summary>
    /// Parses a PE executable and extracts metadata
    /// </summary>
    /// <param name="exePath">Path to the executable file</param>
    /// <returns>Result containing ExecutableInfo or error</returns>
    public static PatchResult<ExecutableInfo> ParseExecutable(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<ExecutableInfo>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            var fileInfo = new FileInfo(exePath);

            using var stream = File.OpenRead(exePath);
            using var reader = new BinaryReader(stream);

            // Read DOS header
            var dosSignature = reader.ReadUInt16(); // "MZ"
            if (dosSignature != 0x5A4D) // MZ
            {
                return PatchResult<ExecutableInfo>.Fail(
                    $"Not a valid PE executable (invalid DOS signature): {exePath}");
            }

            // Skip to e_lfanew offset (at 0x3C)
            stream.Seek(0x3C, SeekOrigin.Begin);
            var peHeaderOffset = reader.ReadInt32();

            // Read PE signature
            stream.Seek(peHeaderOffset, SeekOrigin.Begin);
            var peSignature = reader.ReadUInt32(); // "PE\0\0"
            if (peSignature != 0x00004550) // PE\0\0
            {
                return PatchResult<ExecutableInfo>.Fail(
                    $"Not a valid PE executable (invalid PE signature): {exePath}");
            }

            // Read COFF header (IMAGE_FILE_HEADER)
            var machineType = reader.ReadUInt16();

            // Determine architecture
            var is32Bit = machineType == IMAGE_FILE_MACHINE_I386;
            var is64Bit = machineType == IMAGE_FILE_MACHINE_AMD64;

            if (!is32Bit && !is64Bit)
            {
                return PatchResult<ExecutableInfo>.Fail(
                    $"Unsupported architecture (machine type: 0x{machineType:X4}): {exePath}");
            }

            var executableInfo = new ExecutableInfo
            {
                FileSize = fileInfo.Length,
                Is32Bit = is32Bit,
                Is64Bit = is64Bit,
                MachineType = machineType
            };

            return PatchResult<ExecutableInfo>.Ok(
                executableInfo,
                $"Parsed {(is32Bit ? "32-bit" : "64-bit")} executable: {Path.GetFileName(exePath)}"
            );
        }
        catch (Exception ex)
        {
            return PatchResult<ExecutableInfo>.Fail($"Failed to parse executable: {ex.Message}");
        }
    }

    /// <summary>
    /// Checks if a file is a valid PE executable
    /// </summary>
    /// <param name="exePath">Path to the file</param>
    /// <returns>True if the file is a valid PE executable</returns>
    public static bool IsValidPE(string exePath)
    {
        var result = ParseExecutable(exePath);
        return result.Success;
    }
}
