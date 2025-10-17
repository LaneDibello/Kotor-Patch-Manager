using KPatchCore.Models;
using PeNet;

namespace KPatchCore.Parsers;

/// <summary>
/// Parses PE (Portable Executable) files to extract metadata
/// </summary>
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
        /// Product version from version info (e.g., "1.03")
        /// </summary>
        public string? ProductVersion { get; init; }

        /// <summary>
        /// File version from version info (e.g., "1.0.3.0")
        /// </summary>
        public string? FileVersion { get; init; }

        /// <summary>
        /// Company name from version info
        /// </summary>
        public string? CompanyName { get; init; }

        /// <summary>
        /// Product name from version info
        /// </summary>
        public string? ProductName { get; init; }

        /// <summary>
        /// Number of imports (imported DLLs)
        /// </summary>
        public int ImportCount { get; init; }

        /// <summary>
        /// List of imported DLL names
        /// </summary>
        public List<string> ImportedDlls { get; init; } = new();
    }

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
            var peFile = new PeFile(exePath);

            // Check if it's a valid PE file by checking if ImageNtHeaders exists
            if (peFile.ImageNtHeaders == null)
            {
                return PatchResult<ExecutableInfo>.Fail($"File is not a valid PE executable: {exePath}");
            }

            // Determine architecture from Machine type
            bool is32Bit = peFile.Is32Bit;
            bool is64Bit = peFile.Is64Bit;

            // Extract version info (if available)
            // Note: PeNet's version info API is complex, skipping for now
            // We'll primarily rely on file hash for version detection
            string? productVersion = null;
            string? fileVersion = null;
            string? companyName = null;
            string? productName = null;

            // Get imports
            var importedDlls = new List<string>();
            int importCount = 0;

            if (peFile.ImportedFunctions != null)
            {
                var uniqueDlls = peFile.ImportedFunctions
                    .Select(import => import.DLL)
                    .Where(dll => !string.IsNullOrWhiteSpace(dll))
                    .Distinct()
                    .ToList();

                importedDlls.AddRange(uniqueDlls);
                importCount = uniqueDlls.Count;
            }

            var executableInfo = new ExecutableInfo
            {
                FileSize = fileInfo.Length,
                Is32Bit = is32Bit,
                Is64Bit = is64Bit,
                ProductVersion = productVersion,
                FileVersion = fileVersion,
                CompanyName = companyName,
                ProductName = productName,
                ImportCount = importCount,
                ImportedDlls = importedDlls
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
    /// Checks if an executable already imports a specific DLL
    /// </summary>
    /// <param name="exePath">Path to the executable</param>
    /// <param name="dllName">DLL name to check (case-insensitive)</param>
    /// <returns>Result indicating whether the DLL is imported</returns>
    public static PatchResult<bool> HasImport(string exePath, string dllName)
    {
        var result = ParseExecutable(exePath);
        if (!result.Success || result.Data == null)
        {
            return PatchResult<bool>.Fail(result.Error ?? "Failed to parse executable");
        }

        var hasImport = result.Data.ImportedDlls
            .Any(dll => dll.Equals(dllName, StringComparison.OrdinalIgnoreCase));

        return PatchResult<bool>.Ok(hasImport);
    }
}
