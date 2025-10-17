using KPatchCore.Common;
using KPatchCore.Models;
using PeNet;
using PeNet.Header.Pe;

namespace KPatchCore.Applicators;

/// <summary>
/// Injects the KotorPatcher.dll into a game executable by modifying the PE import table
/// </summary>
/// <remarks>
/// ⚠️ EXPERIMENTAL: PE import table modification is complex and has limitations.
/// The current implementation:
/// - Can read and analyze import tables
/// - Can detect if patcher DLL is already injected
/// - Has basic import injection (may not work with all executables)
///
/// For production use, consider alternative injection methods:
/// - Launcher application with CreateRemoteThread injection (recommended)
/// - DLL proxy/hijacking
/// - External PE manipulation tools
///
/// See PE_INJECTION_NOTES.md for detailed discussion of approaches.
/// </remarks>
public static class LoaderInjector
{
    /// <summary>
    /// The name of the patcher DLL that will be injected
    /// </summary>
    public const string PatcherDllName = "KotorPatcher.dll";

    /// <summary>
    /// Injects the patcher DLL into an executable's import table
    /// </summary>
    /// <param name="exePath">Path to the executable to modify</param>
    /// <param name="verifyBackup">Whether to verify a backup exists before modifying</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult InjectLoader(string exePath, bool verifyBackup = true)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult.Fail($"Executable not found: {exePath}");
        }

        if (verifyBackup)
        {
            var backupPath = PathHelpers.GetBackupPath(exePath);
            if (!File.Exists(backupPath))
            {
                return PatchResult.Fail(
                    "No backup found. Create a backup before modifying the executable."
                );
            }
        }

        try
        {
            // Check if already injected
            var checkResult = IsLoaderInjected(exePath);
            if (!checkResult.Success)
            {
                return PatchResult.Fail($"Failed to check injection status: {checkResult.Error}");
            }

            if (checkResult.Data)
            {
                return PatchResult.Ok($"{PatcherDllName} is already in import table");
            }

            // Load the PE file
            var peFile = new PeFile(exePath);

            if (peFile.ImageNtHeaders == null)
            {
                return PatchResult.Fail($"Not a valid PE executable: {exePath}");
            }

            if (!peFile.Is32Bit)
            {
                return PatchResult.Fail("Only 32-bit executables are supported in this version");
            }

            // Perform the injection
            var result = InjectImport(peFile, exePath);
            if (!result.Success)
            {
                return result;
            }

            // Verify the injection
            var verifyResult = IsLoaderInjected(exePath);
            if (!verifyResult.Success || !verifyResult.Data)
            {
                return PatchResult.Fail(
                    "Injection completed but verification failed - executable may be corrupted"
                );
            }

            return PatchResult.Ok($"Successfully injected {PatcherDllName} into import table");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to inject loader: {ex.Message}");
        }
    }

    /// <summary>
    /// Removes the patcher DLL from an executable's import table
    /// </summary>
    /// <param name="exePath">Path to the executable to modify</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult RemoveLoader(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult.Fail($"Executable not found: {exePath}");
        }

        try
        {
            // Check if it's actually injected
            var checkResult = IsLoaderInjected(exePath);
            if (!checkResult.Success)
            {
                return PatchResult.Fail($"Failed to check injection status: {checkResult.Error}");
            }

            if (!checkResult.Data)
            {
                return PatchResult.Ok($"{PatcherDllName} is not in import table");
            }

            // For removal, we'll restore from backup
            // Direct removal from import table is complex and risky
            // The proper way is to use BackupManager.RestoreBackup()
            return PatchResult.Fail(
                "Loader removal not implemented - use BackupManager.RestoreBackup() instead. " +
                "Direct import table manipulation for removal is unsafe."
            );
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to remove loader: {ex.Message}");
        }
    }

    /// <summary>
    /// Checks if the patcher DLL is already in the import table
    /// </summary>
    /// <param name="exePath">Path to the executable to check</param>
    /// <returns>Result containing true if injected, false otherwise</returns>
    public static PatchResult<bool> IsLoaderInjected(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<bool>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            var peFile = new PeFile(exePath);

            if (peFile.ImageNtHeaders == null)
            {
                return PatchResult<bool>.Fail($"Not a valid PE executable: {exePath}");
            }

            // Check imported DLLs
            if (peFile.ImportedFunctions != null)
            {
                var hasImport = peFile.ImportedFunctions
                    .Any(import => import.DLL?.Equals(PatcherDllName, StringComparison.OrdinalIgnoreCase) == true);

                return PatchResult<bool>.Ok(hasImport);
            }

            return PatchResult<bool>.Ok(false);
        }
        catch (Exception ex)
        {
            return PatchResult<bool>.Fail($"Failed to check imports: {ex.Message}");
        }
    }

    /// <summary>
    /// Gets detailed information about the current import table
    /// </summary>
    /// <param name="exePath">Path to the executable</param>
    /// <returns>Result containing import table information</returns>
    public static PatchResult<ImportTableInfo> GetImportTableInfo(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<ImportTableInfo>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            var peFile = new PeFile(exePath);

            if (peFile.ImageNtHeaders == null)
            {
                return PatchResult<ImportTableInfo>.Fail($"Not a valid PE executable: {exePath}");
            }

            var imports = new List<string>();
            if (peFile.ImportedFunctions != null)
            {
                imports = peFile.ImportedFunctions
                    .Select(import => import.DLL)
                    .Where(dll => !string.IsNullOrWhiteSpace(dll))
                    .Distinct()
                    .ToList();
            }

            var info = new ImportTableInfo
            {
                ImportCount = imports.Count,
                ImportedDlls = imports,
                HasPatcherDll = imports.Any(dll =>
                    dll.Equals(PatcherDllName, StringComparison.OrdinalIgnoreCase))
            };

            return PatchResult<ImportTableInfo>.Ok(info);
        }
        catch (Exception ex)
        {
            return PatchResult<ImportTableInfo>.Fail($"Failed to get import info: {ex.Message}");
        }
    }

    /// <summary>
    /// Performs the actual import injection using PeNet
    /// </summary>
    private static PatchResult InjectImport(PeFile peFile, string exePath)
    {
        try
        {
            // WARNING: PeNet v5.1.0 has limited support for modifying imports
            // This is a complex operation that requires:
            // 1. Expanding the import directory
            // 2. Adding new import descriptors
            // 3. Updating the import address table (IAT)
            // 4. Updating the import lookup table (ILT)
            // 5. Updating PE headers with new sizes and RVAs

            // For MVP, we'll use a different approach:
            // Read the PE file as binary and manually manipulate the import table
            // This is safer than relying on PeNet's modification capabilities

            return ManualImportInjection(exePath, peFile);
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Import injection failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Manually injects an import by manipulating PE binary data
    /// </summary>
    private static PatchResult ManualImportInjection(string exePath, PeFile peFile)
    {
        try
        {
            // Read the entire PE file
            var peBytes = File.ReadAllBytes(exePath);

            // Get import directory info
            var importDir = peFile.ImageNtHeaders?.OptionalHeader?.DataDirectory?[1]; // Import Directory
            if (importDir == null || importDir.VirtualAddress == 0)
            {
                return PatchResult.Fail("Executable has no import directory");
            }

            // Convert RVA to file offset
            var importDirOffset = RvaToFileOffset(peFile, importDir.VirtualAddress);
            if (importDirOffset == -1)
            {
                return PatchResult.Fail("Failed to locate import directory in file");
            }

            // Count existing import descriptors
            var importDescriptorCount = CountImportDescriptors(peBytes, importDirOffset);

            // Calculate space needed
            // We need:
            // - 1 new IMAGE_IMPORT_DESCRIPTOR (20 bytes)
            // - DLL name string (strlen + 1)
            // - At least one import entry (4 bytes for name RVA, 4 bytes for thunk)
            // - Null terminator descriptor (20 bytes)

            var dllNameLength = PatcherDllName.Length + 1; // +1 for null terminator
            var spaceNeeded = 20 + dllNameLength + 8 + 20; // descriptor + name + entry + null

            // Try to find code cave or expand section
            var caveResult = FindCodeCave(peFile, peBytes, spaceNeeded);
            if (!caveResult.Success || caveResult.Data == null)
            {
                return PatchResult.Fail(
                    "No space available for import injection. " +
                    $"Need {spaceNeeded} bytes. " +
                    "Consider using a PE editor tool or creating a new section."
                );
            }

            var (caveRva, caveOffset) = caveResult.Data.Value;

            // Build new import descriptor
            var newDescriptor = BuildImportDescriptor(caveRva, PatcherDllName);

            // Write new import descriptor before the null terminator
            var newDescriptorOffset = importDirOffset + (importDescriptorCount * 20);
            Array.Copy(newDescriptor, 0, peBytes, newDescriptorOffset, 20);

            // Write DLL name at cave
            var nameBytes = System.Text.Encoding.ASCII.GetBytes(PatcherDllName + "\0");
            Array.Copy(nameBytes, 0, peBytes, caveOffset, nameBytes.Length);

            // Write null terminator descriptor (20 zero bytes)
            var nullDescriptor = new byte[20];
            Array.Copy(nullDescriptor, 0, peBytes, newDescriptorOffset + 20, 20);

            // Update import directory size if needed
            // Note: Some executables don't require this, but it's good practice
            // We'd need to update the data directory entry in the PE header

            // Write modified PE back to disk
            File.WriteAllBytes(exePath, peBytes);

            return PatchResult.Ok("Import injection completed");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Manual injection failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Converts an RVA (Relative Virtual Address) to a file offset
    /// </summary>
    private static long RvaToFileOffset(PeFile peFile, uint rva)
    {
        var sections = peFile.ImageSectionHeaders;
        if (sections == null) return -1;

        foreach (var section in sections)
        {
            if (rva >= section.VirtualAddress &&
                rva < section.VirtualAddress + section.VirtualSize)
            {
                return section.PointerToRawData + (rva - section.VirtualAddress);
            }
        }

        return -1;
    }

    /// <summary>
    /// Counts the number of import descriptors (excluding null terminator)
    /// </summary>
    private static int CountImportDescriptors(byte[] peBytes, long importDirOffset)
    {
        int count = 0;
        var offset = importDirOffset;

        while (true)
        {
            // IMAGE_IMPORT_DESCRIPTOR is 20 bytes
            // Check if it's all zeros (null terminator)
            bool isNull = true;
            for (int i = 0; i < 20; i++)
            {
                if (peBytes[offset + i] != 0)
                {
                    isNull = false;
                    break;
                }
            }

            if (isNull) break;

            count++;
            offset += 20;
        }

        return count;
    }

    /// <summary>
    /// Finds a code cave or suitable space for injection
    /// </summary>
    private static PatchResult<(uint rva, long offset)?> FindCodeCave(
        PeFile peFile,
        byte[] peBytes,
        int spaceNeeded)
    {
        // Look for padding at the end of sections
        var sections = peFile.ImageSectionHeaders;
        if (sections == null)
        {
            return PatchResult<(uint, long)?>.Fail("No sections found");
        }

        foreach (var section in sections)
        {
            // Check sections that are readable (typically .rdata or .data)
            if ((section.Characteristics & ScnCharacteristicsType.MemRead) != 0) // IMAGE_SCN_MEM_READ
            {
                var rawEnd = section.PointerToRawData + section.SizeOfRawData;
                var virtualEnd = section.VirtualAddress + section.VirtualSize;

                // Check if there's padding space
                if (section.SizeOfRawData > section.VirtualSize)
                {
                    var availableSpace = section.SizeOfRawData - section.VirtualSize;
                    if (availableSpace >= spaceNeeded)
                    {
                        var offset = section.PointerToRawData + section.VirtualSize;
                        var rva = section.VirtualAddress + section.VirtualSize;

                        // Verify it's actually zero/unused
                        bool isUnused = true;
                        for (int i = 0; i < spaceNeeded && i < availableSpace; i++)
                        {
                            if (peBytes[offset + i] != 0)
                            {
                                isUnused = false;
                                break;
                            }
                        }

                        if (isUnused)
                        {
                            return PatchResult<(uint, long)?>.Ok((rva, offset));
                        }
                    }
                }
            }
        }

        return PatchResult<(uint, long)?>.Ok(null, "No suitable code cave found");
    }

    /// <summary>
    /// Builds an IMAGE_IMPORT_DESCRIPTOR structure
    /// </summary>
    private static byte[] BuildImportDescriptor(uint nameRva, string dllName)
    {
        var descriptor = new byte[20];

        // IMAGE_IMPORT_DESCRIPTOR structure:
        // +0x00: OriginalFirstThunk (RVA to ILT) - can be 0
        // +0x04: TimeDateStamp - usually 0
        // +0x08: ForwarderChain - usually 0
        // +0x0C: Name (RVA to DLL name string)
        // +0x10: FirstThunk (RVA to IAT)

        // For a simple injection, we can use a minimal descriptor
        // OriginalFirstThunk = 0 (some loaders don't require it)
        BitConverter.GetBytes((uint)0).CopyTo(descriptor, 0);

        // TimeDateStamp = 0
        BitConverter.GetBytes((uint)0).CopyTo(descriptor, 4);

        // ForwarderChain = 0
        BitConverter.GetBytes((uint)0).CopyTo(descriptor, 8);

        // Name RVA
        BitConverter.GetBytes(nameRva).CopyTo(descriptor, 12);

        // FirstThunk = 0 (we'd need to set this up properly for real imports)
        // For now, this is a placeholder - the DLL will still load
        BitConverter.GetBytes((uint)0).CopyTo(descriptor, 16);

        return descriptor;
    }

    /// <summary>
    /// Information about an executable's import table
    /// </summary>
    public sealed class ImportTableInfo
    {
        /// <summary>
        /// Number of imported DLLs
        /// </summary>
        public required int ImportCount { get; init; }

        /// <summary>
        /// List of imported DLL names
        /// </summary>
        public required List<string> ImportedDlls { get; init; }

        /// <summary>
        /// Whether KotorPatcher.dll is already imported
        /// </summary>
        public required bool HasPatcherDll { get; init; }

        /// <summary>
        /// Human-readable summary
        /// </summary>
        public string Summary =>
            $"{ImportCount} imports, Patcher: {(HasPatcherDll ? "YES" : "NO")}";
    }
}
