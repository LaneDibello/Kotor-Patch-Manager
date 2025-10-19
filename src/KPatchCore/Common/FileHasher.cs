using System.Security.Cryptography;

namespace KPatchCore.Common;

/// <summary>
/// Utility for computing file hashes (SHA256)
/// </summary>
public static class FileHasher
{
    /// <summary>
    /// Computes SHA256 hash of a file
    /// </summary>
    /// <param name="filePath">Path to file</param>
    /// <returns>Hex string of hash (uppercase)</returns>
    public static string ComputeSha256(string filePath)
    {
        if (!File.Exists(filePath))
            throw new FileNotFoundException($"File not found: {filePath}");

        using var stream = File.OpenRead(filePath);
        using var sha256 = SHA256.Create();

        var hashBytes = sha256.ComputeHash(stream);
        return Convert.ToHexString(hashBytes);
    }

    /// <summary>
    /// Computes SHA256 hash of a file asynchronously
    /// </summary>
    public static async Task<string> ComputeSha256Async(string filePath, CancellationToken cancellationToken = default)
    {
        if (!File.Exists(filePath))
            throw new FileNotFoundException($"File not found: {filePath}");

        using var stream = File.OpenRead(filePath);
        using var sha256 = SHA256.Create();

        var hashBytes = await sha256.ComputeHashAsync(stream, cancellationToken);
        return Convert.ToHexString(hashBytes);
    }

    /// <summary>
    /// Verifies that a file's hash matches an expected value
    /// </summary>
    /// <param name="filePath">Path to file</param>
    /// <param name="expectedHash">Expected hash (case-insensitive)</param>
    /// <returns>True if hashes match</returns>
    public static bool VerifyHash(string filePath, string expectedHash)
    {
        try
        {
            var actualHash = ComputeSha256(filePath);
            return actualHash.Equals(expectedHash, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Computes hash and file size together (common operation)
    /// </summary>
    public static (string Hash, long FileSize) ComputeHashAndSize(string filePath)
    {
        if (!File.Exists(filePath))
            throw new FileNotFoundException($"File not found: {filePath}");

        var fileInfo = new FileInfo(filePath);
        var hash = ComputeSha256(filePath);

        return (hash, fileInfo.Length);
    }
}
