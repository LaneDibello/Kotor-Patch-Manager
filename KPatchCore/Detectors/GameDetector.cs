using KPatchCore.Common;
using KPatchCore.Models;

namespace KPatchCore.Detectors;

/// <summary>
/// Detects game version by hashing the executable
/// </summary>
public static class GameDetector
{
    /// <summary>
    /// Known game versions database
    /// Maps SHA256 hash to GameVersion
    /// </summary>
    private static readonly Dictionary<string, GameVersion> KnownVersions = new()
    {
        // KOTOR 1 - GOG version 1.03
        ["PLACEHOLDER_HASH_GOG_103"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.GOG,
            Version = "1.03",
            Architecture = Architecture.x86,
            FileSize = 0, // Will be updated when actual hash is added
            Hash = "PLACEHOLDER_HASH_GOG_103"
        },

        // KOTOR 1 - Steam version 1.03
        ["PLACEHOLDER_HASH_STEAM_103"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.Steam,
            Version = "1.03",
            Architecture = Architecture.x86,
            FileSize = 0,
            Hash = "PLACEHOLDER_HASH_STEAM_103"
        },

        // KOTOR 2 - GOG version 1.0b
        ["PLACEHOLDER_HASH_KOTOR2_GOG"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.GOG,
            Version = "1.0b",
            Architecture = Architecture.x86,
            FileSize = 0,
            Hash = "PLACEHOLDER_HASH_KOTOR2_GOG"
        }

        // Additional versions will be added as we test with real executables
    };

    /// <summary>
    /// Detects game version from executable path
    /// </summary>
    /// <param name="exePath">Path to game executable</param>
    /// <returns>Result containing GameVersion or error</returns>
    public static PatchResult<GameVersion> DetectVersion(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<GameVersion>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            // Compute hash and file size
            var (hash, fileSize) = FileHasher.ComputeHashAndSize(exePath);

            // Look up in known versions
            if (KnownVersions.TryGetValue(hash, out var gameVersion))
            {
                return PatchResult<GameVersion>.Ok(
                    gameVersion,
                    $"Detected: {gameVersion.DisplayName}"
                );
            }

            // Version not recognized - create unknown version with hash info
            var unknownVersion = new GameVersion
            {
                Platform = Platform.Windows, // Assume Windows for now
                Distribution = Distribution.Other,
                Version = "Unknown",
                Architecture = Architecture.x86, // Default assumption
                FileSize = fileSize,
                Hash = hash
            };

            return PatchResult<GameVersion>.Ok(
                unknownVersion,
                $"Unknown version (hash: {hash.Substring(0, 16)}...)"
            );
        }
        catch (Exception ex)
        {
            return PatchResult<GameVersion>.Fail($"Failed to detect version: {ex.Message}");
        }
    }

    /// <summary>
    /// Registers a new known version in the database
    /// Useful for testing and adding new versions dynamically
    /// </summary>
    /// <param name="hash">SHA256 hash of the executable</param>
    /// <param name="version">Game version information</param>
    /// <returns>Result indicating success or failure</returns>
    public static PatchResult RegisterKnownVersion(string hash, GameVersion version)
    {
        try
        {
            if (KnownVersions.ContainsKey(hash))
            {
                return PatchResult.Fail($"Hash already registered: {hash.Substring(0, 16)}...");
            }

            KnownVersions[hash] = version;

            return PatchResult.Ok($"Registered version: {version.DisplayName}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to register version: {ex.Message}");
        }
    }

    /// <summary>
    /// Gets all known versions
    /// </summary>
    /// <returns>Dictionary of known versions</returns>
    public static IReadOnlyDictionary<string, GameVersion> GetKnownVersions()
    {
        return KnownVersions;
    }

    /// <summary>
    /// Checks if a specific hash is in the known versions database
    /// </summary>
    /// <param name="hash">SHA256 hash to check</param>
    /// <returns>True if hash is known, false otherwise</returns>
    public static bool IsKnownVersion(string hash)
    {
        return KnownVersions.ContainsKey(hash);
    }

    /// <summary>
    /// Helper method to detect version and get actual hash for registration
    /// Useful for building the known versions database
    /// </summary>
    /// <param name="exePath">Path to executable</param>
    /// <returns>Result containing hash and file size for database entry</returns>
    public static PatchResult<(string Hash, long FileSize)> GetExecutableInfo(string exePath)
    {
        if (!File.Exists(exePath))
        {
            return PatchResult<(string, long)>.Fail($"Executable not found: {exePath}");
        }

        try
        {
            var (hash, fileSize) = FileHasher.ComputeHashAndSize(exePath);
            return PatchResult<(string, long)>.Ok(
                (hash, fileSize),
                $"Hash: {hash}, Size: {fileSize} bytes"
            );
        }
        catch (Exception ex)
        {
            return PatchResult<(string, long)>.Fail($"Failed to get executable info: {ex.Message}");
        }
    }
}
