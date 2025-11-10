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
        // KOTOR 1 - GOG version 1.0.3
        ["9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.GOG,
            Version = "1.0.3",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR1,
            FileSize = 0x3db00,
            Hash = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
        },

        // KOTOR 1 - Steam version 1.0.3
        ["34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.Steam,
            Version = "1.0.3",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR1,
            FileSize = 0x431000,
            Hash = "34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88"
        },

        // KOTOR 2 - GOG version Aspyr
        ["777BEE235A9E8BDD9863F6741BC3AC54BB6A113B62B1D2E4D12BBE6DB963A914"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.GOG,
            Version = "2 1.0.2 (Aspyr)",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR2,
            FileSize = 0x648f98,
            Hash = "777BEE235A9E8BDD9863F6741BC3AC54BB6A113B62B1D2E4D12BBE6DB963A914"
        },

        // KOTOR 2 - Steam version Aspyr
        ["6A522E71631DCEE93467BD2010F3B23D9145326E1E2E89305F13AB104DBBFFEF"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.Steam,
            Version = "2 1.0.2 (Aspyr)",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR2,
            FileSize = 0x648800,
            Hash = "6A522E71631DCEE93467BD2010F3B23D9145326E1E2E89305F13AB104DBBFFEF"
        },

        // KOTOR 2 - Legacy 1.0
        ["92D7800687A0119A1A81527DB875673228C891A3EA241EE130F22567BF34A501"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.Physical,
            Version = "2 1.0 (Legacy)",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR2,
            FileSize = 0x45de00,
            Hash = "92D7800687A0119A1A81527DB875673228C891A3EA241EE130F22567BF34A501"
        },

        // KOTOR 2 - Legacy 1.0b
        ["0912D1942DE4EE849F06588CB738A0E78B6D5FFE92960B9567196D54B7E808D0"] = new GameVersion
        {
            Platform = Platform.Windows,
            Distribution = Distribution.GOG,
            Version = "2 1.0b (Legacy)",
            Architecture = Architecture.x86,
            Title = GameTitle.KOTOR2,
            FileSize = 0x45de00,
            Hash = "0912D1942DE4EE849F06588CB738A0E78B6D5FFE92960B9567196D54B7E808D0"
        }
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
                Title = GameTitle.Unknown,
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
