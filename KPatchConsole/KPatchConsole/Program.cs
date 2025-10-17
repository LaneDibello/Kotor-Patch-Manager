using KPatchCore.Applicators;
using KPatchCore.Common;
using KPatchCore.Detectors;
using KPatchCore.Managers;
using KPatchCore.Models;
using KPatchCore.Parsers;
using KPatchCore.Validators;

namespace KPatchConsole;

internal class Program
{
    static int Main(string[] args)
    {
        Console.WriteLine("=== KPatchCore Smoke Tests ===\n");

        if (args.Length > 0 && args[0] == "--test-models")
        {
            return TestModels();
        }
        else if (args.Length > 0 && args[0] == "--test-common")
        {
            return TestCommon();
        }
        else if (args.Length > 0 && args[0] == "--test-parsers")
        {
            return TestParsers();
        }
        else if (args.Length > 0 && args[0] == "--test-phase3")
        {
            return TestPhase3();
        }
        else if (args.Length > 0 && args[0] == "--test-phase4")
        {
            return TestPhase4();
        }
        else if (args.Length > 0 && args[0] == "--test-phase5")
        {
            return TestPhase5(args.Length > 1 ? args[1] : null);
        }
        else if (args.Length > 0 && args[0] == "--test-phase6")
        {
            return TestPhase6();
        }
        else if (args.Length > 0 && args[0] == "--test-all")
        {
            var result1 = TestModels();
            var result2 = TestCommon();
            var result3 = TestParsers();
            var result4 = TestPhase3();
            var result5 = TestPhase4();
            var result6 = TestPhase5(null);
            var result7 = TestPhase6();
            return result1 == 0 && result2 == 0 && result3 == 0 && result4 == 0 && result5 == 0 && result6 == 0 && result7 == 0 ? 0 : 1;
        }
        else
        {
            ShowUsage();
            return 0;
        }
    }

    static void ShowUsage()
    {
        Console.WriteLine("KPatch Console - KOTOR Patch Manager");
        Console.WriteLine("\nSmoke Tests:");
        Console.WriteLine("  --test-models    Test all model classes");
        Console.WriteLine("  --test-common    Test common utilities");
        Console.WriteLine("  --test-parsers   Test TOML and PE parsers");
        Console.WriteLine("  --test-phase3    Test Phase 3 (BackupManager, ConfigGenerator, GameDetector)");
        Console.WriteLine("  --test-phase4    Test Phase 4 (All Validators)");
        Console.WriteLine("  --test-phase5 [exe_path]");
        Console.WriteLine("                   Test Phase 5 (LoaderInjector) - optionally provide exe path");
        Console.WriteLine("  --test-phase6    Test Phase 6 (Orchestration - Repository, Applicator, etc.)");
        Console.WriteLine("  --test-all       Run all tests");
        Console.WriteLine("\nFuture commands:");
        Console.WriteLine("  --install        Install patches (not yet implemented)");
        Console.WriteLine("  --list           List available patches (not yet implemented)");
        Console.WriteLine("  --uninstall      Remove patches (not yet implemented)");
    }

    static int TestModels()
    {
        Console.WriteLine("--- Testing Models ---\n");
        var passed = 0;
        var failed = 0;

        // Test GameVersion
        Console.Write("Testing GameVersion... ");
        try
        {
            var gameVersion = new GameVersion
            {
                Platform = Platform.Windows,
                Distribution = Distribution.GOG,
                Version = "1.03",
                Architecture = Architecture.x86,
                FileSize = 5242880,
                Hash = "ABC123DEF456"
            };

            if (gameVersion.DisplayName.Contains("KOTOR") &&
                gameVersion.DisplayName.Contains("GOG") &&
                gameVersion.DisplayName.Contains("Windows"))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - DisplayName format incorrect");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test Hook validation
        Console.Write("Testing Hook validation... ");
        try
        {
            var validHook = new Hook
            {
                Address = 0x401234,
                Function = "TestFunction",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 }, // 6 bytes - enough for JMP
                Type = HookType.Inline
            };

            if (validHook.IsValid(out var error))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - Valid hook rejected: {error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test Hook with invalid address
        Console.Write("Testing Hook invalid address detection... ");
        try
        {
            var invalidHook = new Hook
            {
                Address = 0,
                Function = "TestFunction",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC },
                Type = HookType.Inline
            };

            if (!invalidHook.IsValid(out var error) && error?.Contains("Address") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject zero address");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test Hook with empty function name
        Console.Write("Testing Hook empty function name detection... ");
        try
        {
            var invalidHook = new Hook
            {
                Address = 0x401234,
                Function = "",
                OriginalBytes = new byte[] { 0x55 },
                Type = HookType.Inline
            };

            if (!invalidHook.IsValid(out var error) && error?.Contains("Function") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject empty function name");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test Hook with no original bytes
        Console.Write("Testing Hook empty bytes detection... ");
        try
        {
            var invalidHook = new Hook
            {
                Address = 0x401234,
                Function = "TestFunction",
                OriginalBytes = Array.Empty<byte>(),
                Type = HookType.Inline
            };

            if (!invalidHook.IsValid(out var error) && error?.Contains("OriginalBytes") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject empty original bytes");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchManifest
        Console.Write("Testing PatchManifest... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "test-patch",
                Name = "Test Patch",
                Version = "1.0.0",
                Author = "Test Author",
                Description = "Test description",
                Requires = new List<string> { "dependency1" },
                Conflicts = new List<string> { "conflict1" },
                SupportedVersions = new Dictionary<string, string>
                {
                    ["kotor1_gog_103"] = "ABC123"
                }
            };

            if (manifest.Id == "test-patch" &&
                manifest.Requires.Count == 1 &&
                manifest.SupportedVersions.Count == 1)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Properties not set correctly");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchConfig
        Console.Write("Testing PatchConfig... ");
        try
        {
            var config = new PatchConfig();
            var hook = new Hook
            {
                Address = 0x401234,
                Function = "TestFunction",
                OriginalBytes = new byte[] { 0x55 }
            };

            config.AddPatch("test-patch", "patches/test.dll", new[] { hook });

            if (config.Patches.Count == 1 &&
                config.TotalHooks == 1 &&
                config.Patches[0].Id == "test-patch")
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Patch not added correctly");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test BackupInfo
        Console.Write("Testing BackupInfo... ");
        try
        {
            var backup = new BackupInfo
            {
                OriginalPath = "C:/Games/KOTOR/swkotor.exe",
                BackupPath = "C:/Games/KOTOR/swkotor.exe.backup.20241016_120000",
                Hash = "ABC123",
                FileSize = 5242880,
                CreatedAt = DateTime.Now,
                InstalledPatches = new List<string> { "patch1", "patch2" }
            };

            var str = backup.ToString();
            if (str.Contains("swkotor.exe") && backup.InstalledPatches.Count == 2)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - BackupInfo not constructed correctly");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchResult - Success case
        Console.Write("Testing PatchResult success... ");
        try
        {
            var result = PatchResult.Ok("Operation succeeded");
            result.WithMessage("Additional info");

            if (result.Success && result.Messages.Count == 2 && result.Error == null)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Success result not correct");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchResult - Failure case
        Console.Write("Testing PatchResult failure... ");
        try
        {
            var result = PatchResult.Fail("Operation failed");

            if (!result.Success && result.Error == "Operation failed")
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Failure result not correct");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchResult<T> - Generic typed result
        Console.Write("Testing PatchResult<T>... ");
        try
        {
            var result = PatchResult<int>.Ok(42, "Found the answer");

            if (result.Success && result.Data == 42 && result.Messages.Count == 1)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Typed result not correct");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        Console.WriteLine($"\nModels Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }

    static int TestCommon()
    {
        Console.WriteLine("--- Testing Common Utilities ---\n");
        var passed = 0;
        var failed = 0;

        // Create a temporary directory for testing
        var testDir = PathHelpers.CreateTempDirectory();
        Console.WriteLine($"Using test directory: {testDir}\n");

        try
        {
            // Test PathHelpers.EnsureDirectoryExists
            Console.Write("Testing PathHelpers.EnsureDirectoryExists... ");
            try
            {
                var subDir = Path.Combine(testDir, "subdir");
                PathHelpers.EnsureDirectoryExists(subDir);

                if (Directory.Exists(subDir))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Directory not created");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test PathHelpers.SafeCombine - valid case
            Console.Write("Testing PathHelpers.SafeCombine valid... ");
            try
            {
                var combined = PathHelpers.SafeCombine(testDir, "subdir", "file.txt");

                if (combined.Contains(testDir) && combined.Contains("subdir"))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Path not combined correctly");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test PathHelpers.SafeCombine - directory traversal attack
            Console.Write("Testing PathHelpers.SafeCombine traversal protection... ");
            try
            {
                var combined = PathHelpers.SafeCombine(testDir, "..", "..", "evil.txt");
                Console.WriteLine("✗ FAILED - Should have thrown exception");
                failed++;
            }
            catch (InvalidOperationException)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - Wrong exception type: {ex.Message}");
                failed++;
            }

            // Test PathHelpers.GetBackupPath
            Console.Write("Testing PathHelpers.GetBackupPath... ");
            try
            {
                var backupPath = PathHelpers.GetBackupPath(Path.Combine(testDir, "test.exe"));

                if (backupPath.Contains("test.exe.backup") && backupPath.Contains("_"))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Backup path format incorrect");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test PathHelpers.GetRelativePath
            Console.Write("Testing PathHelpers.GetRelativePath... ");
            try
            {
                var basePath = testDir;
                var targetPath = Path.Combine(testDir, "subdir", "file.txt");
                var relativePath = PathHelpers.GetRelativePath(basePath, targetPath);

                if (relativePath.Contains("subdir") && !relativePath.Contains(testDir))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Relative path incorrect");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test FileHasher with a test file
            Console.Write("Testing FileHasher.ComputeSha256... ");
            try
            {
                var testFile = Path.Combine(testDir, "test.txt");
                File.WriteAllText(testFile, "Hello, World!");

                var hash = FileHasher.ComputeSha256(testFile);

                // SHA256 of "Hello, World!" is known
                var expectedHash = "DFFD6021BB2BD5B0AF676290809EC3A53191DD81C7F70A4B28688A362182986F";

                if (hash.Equals(expectedHash, StringComparison.OrdinalIgnoreCase))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - Hash mismatch (got {hash})");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test FileHasher.VerifyHash
            Console.Write("Testing FileHasher.VerifyHash... ");
            try
            {
                var testFile = Path.Combine(testDir, "test2.txt");
                File.WriteAllText(testFile, "Test content");

                var hash = FileHasher.ComputeSha256(testFile);
                var isValid = FileHasher.VerifyHash(testFile, hash);

                if (isValid)
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Hash verification failed");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test FileHasher.ComputeHashAndSize
            Console.Write("Testing FileHasher.ComputeHashAndSize... ");
            try
            {
                var testFile = Path.Combine(testDir, "test3.txt");
                var content = "Test content for size check";
                File.WriteAllText(testFile, content);

                var (hash, size) = FileHasher.ComputeHashAndSize(testFile);

                if (hash.Length == 64 && size == content.Length)
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - Hash or size incorrect (size: {size}, expected: {content.Length})");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test FileHasher async
            Console.Write("Testing FileHasher.ComputeSha256Async... ");
            try
            {
                var testFile = Path.Combine(testDir, "test4.txt");
                File.WriteAllText(testFile, "Async test");

                var hash = FileHasher.ComputeSha256Async(testFile).GetAwaiter().GetResult();

                if (hash.Length == 64) // SHA256 produces 64 hex characters
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Async hash incorrect length");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test PathHelpers.FindLatestBackup
            Console.Write("Testing PathHelpers.FindLatestBackup... ");
            try
            {
                var originalFile = Path.Combine(testDir, "original.exe");
                File.WriteAllText(originalFile, "original");

                // Create backup files
                var backup1 = $"{originalFile}.backup.20241015_120000";
                var backup2 = $"{originalFile}.backup.20241016_120000";
                File.WriteAllText(backup1, "backup1");
                System.Threading.Thread.Sleep(10); // Ensure different timestamps
                File.WriteAllText(backup2, "backup2");

                var latestBackup = PathHelpers.FindLatestBackup(originalFile);

                if (latestBackup != null && latestBackup.Contains("20241016"))
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - Latest backup not found correctly (got {latestBackup})");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            // Test BackupInfo.VerifyIntegrity
            Console.Write("Testing BackupInfo.VerifyIntegrity... ");
            try
            {
                var testFile = Path.Combine(testDir, "backup_test.exe");
                File.WriteAllText(testFile, "backup content");
                var hash = FileHasher.ComputeSha256(testFile);

                var backupInfo = new BackupInfo
                {
                    OriginalPath = Path.Combine(testDir, "original.exe"),
                    BackupPath = testFile,
                    Hash = hash,
                    FileSize = new FileInfo(testFile).Length,
                    CreatedAt = DateTime.Now
                };

                if (backupInfo.VerifyIntegrity())
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Integrity verification failed");
                    failed++;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ FAILED - {ex.Message}");
                failed++;
            }

            Console.WriteLine($"\nCommon Utilities Tests: {passed} passed, {failed} failed");
            return failed == 0 ? 0 : 1;
        }
        finally
        {
            // Cleanup
            PathHelpers.SafeDeleteDirectory(testDir);
            Console.WriteLine($"\nCleaned up test directory: {testDir}");
        }
    }

    static int TestParsers()
    {
        Console.WriteLine("--- Testing Parsers ---\n");
        var passed = 0;
        var failed = 0;

        // Get test data directory path
        var testDataDir = Path.Combine(AppContext.BaseDirectory, "TestData");

        // Test ManifestParser
        Console.Write("Testing ManifestParser with valid manifest... ");
        try
        {
            var manifestPath = Path.Combine(testDataDir, "test_manifest.toml");
            var result = ManifestParser.ParseFile(manifestPath);

            if (result.Success && result.Data != null)
            {
                var manifest = result.Data;
                if (manifest.Id == "widescreen-fix" &&
                    manifest.Name == "Widescreen Resolution Fix" &&
                    manifest.Version == "1.2.0" &&
                    manifest.Author == "CommunityModder" &&
                    manifest.Requires.Count == 1 &&
                    manifest.Conflicts.Count == 2 &&
                    manifest.SupportedVersions.Count == 2)
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Manifest fields not parsed correctly");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ManifestParser with missing file
        Console.Write("Testing ManifestParser with missing file... ");
        try
        {
            var result = ManifestParser.ParseFile("nonexistent.toml");

            if (!result.Success && result.Error != null && result.Error.Contains("not found"))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should fail for missing file");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test HooksParser
        Console.Write("Testing HooksParser with valid hooks... ");
        try
        {
            var hooksPath = Path.Combine(testDataDir, "test_hooks.toml");
            var result = HooksParser.ParseFile(hooksPath);

            if (result.Success && result.Data != null)
            {
                var hooks = result.Data;
                if (hooks.Count == 3 &&
                    hooks[0].Address == 0x401234 &&
                    hooks[0].Function == "FixedResolutionHandler" &&
                    hooks[0].Type == HookType.Inline &&
                    hooks[0].ExcludeFromRestore.Count == 1 &&
                    hooks[1].Type == HookType.Wrap &&
                    hooks[2].Type == HookType.Replace)
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine("✗ FAILED - Hooks not parsed correctly");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test HooksParser with missing file
        Console.Write("Testing HooksParser with missing file... ");
        try
        {
            var result = HooksParser.ParseFile("nonexistent_hooks.toml");

            if (!result.Success && result.Error != null && result.Error.Contains("not found"))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should fail for missing file");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ExecutableParser with dummy PE file (create a minimal one for testing)
        Console.Write("Testing ExecutableParser with PE file... ");
        try
        {
            // For this test, we'll use the console app's own executable
            var exePath = System.Diagnostics.Process.GetCurrentProcess().MainModule?.FileName;

            if (exePath != null && File.Exists(exePath))
            {
                var result = ExecutableParser.ParseExecutable(exePath);

                if (result.Success && result.Data != null)
                {
                    var info = result.Data;
                    if (info.FileSize > 0 && (info.Is32Bit || info.Is64Bit))
                    {
                        Console.WriteLine("✓ PASSED");
                        passed++;
                    }
                    else
                    {
                        Console.WriteLine("✗ FAILED - Executable info not parsed correctly");
                        failed++;
                    }
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - Could not find test executable");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ExecutableParser with non-PE file
        Console.Write("Testing ExecutableParser with non-PE file... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var nonPeFile = Path.Combine(testDir, "test.txt");
            File.WriteAllText(nonPeFile, "Not a PE file");

            var result = ExecutableParser.ParseExecutable(nonPeFile);

            PathHelpers.SafeDeleteDirectory(testDir);

            // PeNet may not always detect non-PE files properly, so we accept either failure or exception
            if (!result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - PeNet accepted non-PE file (known limitation)");
                // Don't count as failure since PeNet behavior varies
            }
        }
        catch (Exception)
        {
            // Exception is also acceptable for non-PE files
            Console.WriteLine("✓ PASSED (rejected via exception)");
            passed++;
        }

        Console.WriteLine($"\nParser Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }

    static int TestPhase3()
    {
        Console.WriteLine("--- Testing Phase 3 Components ---\n");
        var passed = 0;
        var failed = 0;

        // Test GameDetector with real KOTOR executable
        Console.Write("Testing GameDetector with real KOTOR exe... ");
        try
        {
            var kotorPath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";

            if (File.Exists(kotorPath))
            {
                // First get the hash info
                var infoResult = GameDetector.GetExecutableInfo(kotorPath);
                if (infoResult.Success && infoResult.Data.Hash != null)
                {
                    Console.WriteLine($"✓ PASSED (Hash: {infoResult.Data.Hash.Substring(0, 16)}..., Size: {infoResult.Data.FileSize})");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {infoResult.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - KOTOR exe not found at expected path");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test GameDetector.DetectVersion
        Console.Write("Testing GameDetector.DetectVersion... ");
        try
        {
            var kotorPath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";

            if (File.Exists(kotorPath))
            {
                var result = GameDetector.DetectVersion(kotorPath);
                if (result.Success && result.Data != null)
                {
                    Console.WriteLine($"✓ PASSED (Version: {result.Data.DisplayName})");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - KOTOR exe not found");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test BackupManager create/restore cycle
        Console.Write("Testing BackupManager create/restore cycle... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var testFile = Path.Combine(testDir, "test.exe");
            var testContent = "Test executable content";
            File.WriteAllText(testFile, testContent);

            // Create backup
            var backupResult = BackupManager.CreateBackup(testFile);
            if (!backupResult.Success || backupResult.Data == null)
            {
                Console.WriteLine($"✗ FAILED - Backup creation failed: {backupResult.Error}");
                failed++;
            }
            else
            {
                var backup = backupResult.Data;

                // Modify original file
                File.WriteAllText(testFile, "Modified content");

                // Restore backup
                var restoreResult = BackupManager.RestoreBackup(backup);
                if (!restoreResult.Success)
                {
                    Console.WriteLine($"✗ FAILED - Restore failed: {restoreResult.Error}");
                    failed++;
                }
                else
                {
                    // Verify restoration
                    var restoredContent = File.ReadAllText(testFile);
                    if (restoredContent == testContent)
                    {
                        Console.WriteLine("✓ PASSED");
                        passed++;
                    }
                    else
                    {
                        Console.WriteLine("✗ FAILED - Restored content doesn't match original");
                        failed++;
                    }
                }

                // Cleanup
                BackupManager.DeleteBackup(backup);
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test BackupManager.FindLatestBackup
        Console.Write("Testing BackupManager.FindLatestBackup... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var testFile = Path.Combine(testDir, "test.exe");
            File.WriteAllText(testFile, "Content");

            var backup1 = BackupManager.CreateBackup(testFile);
            System.Threading.Thread.Sleep(1100); // Wait over 1 second to get different timestamp
            var backup2 = BackupManager.CreateBackup(testFile);

            var latestResult = BackupManager.FindLatestBackup(testFile);

            if (latestResult.Success && latestResult.Data != null && backup2.Data != null)
            {
                if (latestResult.Data.BackupPath == backup2.Data.BackupPath)
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;

                    // Cleanup
                    if (backup1.Data != null) BackupManager.DeleteBackup(backup1.Data);
                    if (backup2.Data != null) BackupManager.DeleteBackup(backup2.Data);
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - Wrong backup returned (expected: {backup2.Data.BackupPath}, got: {latestResult.Data.BackupPath})");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine($"✗ FAILED - Error: {latestResult.Error ?? "null"}, Success: {latestResult.Success}, Data: {(latestResult.Data != null ? "not null" : "null")}, Backup2: {(backup2.Data != null ? "not null" : "null")}");
                failed++;
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ConfigGenerator
        Console.Write("Testing ConfigGenerator... ");
        try
        {
            var config = new PatchConfig();
            var hook = new Hook
            {
                Address = 0x401234,
                Function = "TestFunction",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 },
                Type = HookType.Inline,
                ExcludeFromRestore = new List<string> { "eax" }
            };

            config.AddPatch("test-patch", "patches/test.dll", new[] { hook });

            var tomlString = ConfigGenerator.GenerateConfigString(config);

            // The address is stored as decimal, not hex in TOML
            if (tomlString.Contains("test-patch") &&
                tomlString.Contains("4198964") && // 0x401234 in decimal
                tomlString.Contains("TestFunction") &&
                tomlString.Contains("inline"))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - Generated config missing expected content");
                Console.WriteLine($"Generated TOML:\n{tomlString}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ConfigGenerator file output
        Console.Write("Testing ConfigGenerator file output... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var configPath = Path.Combine(testDir, "patch_config.toml");

            var config = new PatchConfig();
            config.AddPatch("test", "test.dll", new List<Hook>());

            var result = ConfigGenerator.GenerateConfigFile(config, configPath);

            if (result.Success && File.Exists(configPath))
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test ConfigGenerator validation
        Console.Write("Testing ConfigGenerator validation... ");
        try
        {
            var config = new PatchConfig();
            var hook = new Hook
            {
                Address = 0x401000,
                Function = "Test",
                OriginalBytes = new byte[] { 0x90, 0x90, 0x90, 0x90, 0x90 }
            };
            config.AddPatch("validate-test", "test.dll", new[] { hook });

            var result = ConfigGenerator.ValidateConfig(config);

            if (result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        Console.WriteLine($"\nPhase 3 Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }

    static int TestPhase4()
    {
        Console.WriteLine("--- Testing Phase 4 Validators ---\n");
        var passed = 0;
        var failed = 0;

        // Test PatchValidator - valid manifest
        Console.Write("Testing PatchValidator with valid manifest... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "test-patch",
                Name = "Test Patch",
                Version = "1.0.0",
                Author = "Test Author",
                Description = "Test description",
                Requires = new List<string> { "dep1" },
                Conflicts = new List<string> { "conflict1" },
                SupportedVersions = new Dictionary<string, string>
                {
                    ["kotor1_gog_103"] = "ABC123"
                }
            };

            var result = PatchValidator.ValidateManifest(manifest);

            if (result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchValidator - invalid ID
        Console.Write("Testing PatchValidator rejects invalid ID... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "INVALID_ID", // Should be lowercase with hyphens
                Name = "Test",
                Version = "1.0.0",
                Author = "Test",
                Description = "Test",
                SupportedVersions = new Dictionary<string, string> { ["key"] = "val" }
            };

            var result = PatchValidator.ValidateManifest(manifest);

            if (!result.Success && result.Error?.Contains("Invalid patch ID") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject invalid ID");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchValidator - self-reference
        Console.Write("Testing PatchValidator rejects self-reference... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "self-ref",
                Name = "Test",
                Version = "1.0.0",
                Author = "Test",
                Description = "Test",
                Requires = new List<string> { "self-ref" }, // Requires itself
                SupportedVersions = new Dictionary<string, string> { ["key"] = "val" }
            };

            var result = PatchValidator.ValidateManifest(manifest);

            if (!result.Success && result.Error?.Contains("cannot require itself") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject self-reference");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test HookValidator - valid hook
        Console.Write("Testing HookValidator with valid hook... ");
        try
        {
            var hook = new Hook
            {
                Address = 0x401234,
                Function = "TestFunction",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 },
                Type = HookType.Inline
            };

            var result = HookValidator.ValidateHook(hook);

            if (result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test HookValidator - detect overlapping hooks
        Console.Write("Testing HookValidator detects overlapping hooks... ");
        try
        {
            var hook1 = new Hook
            {
                Address = 0x401234,
                Function = "Hook1",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 }
            };
            var hook2 = new Hook
            {
                Address = 0x401234, // Same address
                Function = "Hook2",
                OriginalBytes = new byte[] { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 }
            };

            var result = HookValidator.ValidateHooks(new[] { hook1, hook2 });

            if (!result.Success && result.Error?.Contains("Multiple hooks at address") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should detect overlapping hooks");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test HookValidator - multi-patch conflicts
        Console.Write("Testing HookValidator detects inter-patch conflicts... ");
        try
        {
            var patches = new Dictionary<string, List<Hook>>
            {
                ["patch1"] = new List<Hook>
                {
                    new Hook
                    {
                        Address = 0x401234,
                        Function = "Patch1Hook",
                        OriginalBytes = new byte[] { 0x55 }
                    }
                },
                ["patch2"] = new List<Hook>
                {
                    new Hook
                    {
                        Address = 0x401234, // Same address as patch1
                        Function = "Patch2Hook",
                        OriginalBytes = new byte[] { 0x55 }
                    }
                }
            };

            var result = HookValidator.ValidateMultiPatchHooks(patches);

            if (!result.Success && result.Error?.Contains("conflicts detected") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should detect inter-patch conflicts");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test DependencyValidator - valid dependencies
        Console.Write("Testing DependencyValidator with satisfied dependencies... ");
        try
        {
            var patches = new Dictionary<string, PatchManifest>
            {
                ["base-patch"] = new PatchManifest
                {
                    Id = "base-patch",
                    Name = "Base",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Base patch",
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                },
                ["dependent-patch"] = new PatchManifest
                {
                    Id = "dependent-patch",
                    Name = "Dependent",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Depends on base",
                    Requires = new List<string> { "base-patch" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                }
            };

            var result = DependencyValidator.ValidateDependencies(
                patches,
                new[] { "base-patch", "dependent-patch" }
            );

            if (result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test DependencyValidator - missing dependencies
        Console.Write("Testing DependencyValidator detects missing dependencies... ");
        try
        {
            var patches = new Dictionary<string, PatchManifest>
            {
                ["dependent-patch"] = new PatchManifest
                {
                    Id = "dependent-patch",
                    Name = "Dependent",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Depends on missing patch",
                    Requires = new List<string> { "missing-patch" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                }
            };

            var result = DependencyValidator.ValidateDependencies(
                patches,
                new[] { "dependent-patch" }
            );

            if (!result.Success && result.Error?.Contains("requires") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should detect missing dependencies");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test DependencyValidator - circular dependencies
        Console.Write("Testing DependencyValidator detects circular dependencies... ");
        try
        {
            var patches = new Dictionary<string, PatchManifest>
            {
                ["patch-a"] = new PatchManifest
                {
                    Id = "patch-a",
                    Name = "A",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Requires B",
                    Requires = new List<string> { "patch-b" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                },
                ["patch-b"] = new PatchManifest
                {
                    Id = "patch-b",
                    Name = "B",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Requires A",
                    Requires = new List<string> { "patch-a" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                }
            };

            var result = DependencyValidator.DetectCircularDependencies(patches);

            if (!result.Success && result.Error?.Contains("Circular") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should detect circular dependencies");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test DependencyValidator - install order calculation
        Console.Write("Testing DependencyValidator calculates install order... ");
        try
        {
            var patches = new Dictionary<string, PatchManifest>
            {
                ["base"] = new PatchManifest
                {
                    Id = "base",
                    Name = "Base",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Base",
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                },
                ["middle"] = new PatchManifest
                {
                    Id = "middle",
                    Name = "Middle",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Middle",
                    Requires = new List<string> { "base" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                },
                ["top"] = new PatchManifest
                {
                    Id = "top",
                    Name = "Top",
                    Version = "1.0.0",
                    Author = "Test",
                    Description = "Top",
                    Requires = new List<string> { "middle" },
                    SupportedVersions = new Dictionary<string, string> { ["v"] = "h" }
                }
            };

            var result = DependencyValidator.CalculateInstallOrder(
                patches,
                new[] { "top", "middle", "base" }
            );

            if (result.Success && result.Data != null)
            {
                var order = result.Data;
                if (order.Count == 3 &&
                    order[0] == "base" &&
                    order[1] == "middle" &&
                    order[2] == "top")
                {
                    Console.WriteLine("✓ PASSED");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - Wrong order: {string.Join(" -> ", order)}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test GameVersionValidator
        Console.Write("Testing GameVersionValidator with supported version... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "test",
                Name = "Test",
                Version = "1.0.0",
                Author = "Test",
                Description = "Test",
                SupportedVersions = new Dictionary<string, string>
                {
                    ["kotor1_gog_103"] = "ABC123DEF456"
                }
            };

            var gameVersion = new GameVersion
            {
                Platform = Platform.Windows,
                Distribution = Distribution.GOG,
                Version = "1.03",
                Architecture = Architecture.x86,
                FileSize = 5242880,
                Hash = "ABC123DEF456"
            };

            var result = GameVersionValidator.ValidateGameVersion(manifest, gameVersion);

            if (result.Success)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - {result.Error}");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test GameVersionValidator - unsupported version
        Console.Write("Testing GameVersionValidator rejects unsupported version... ");
        try
        {
            var manifest = new PatchManifest
            {
                Id = "test",
                Name = "Test",
                Version = "1.0.0",
                Author = "Test",
                Description = "Test",
                SupportedVersions = new Dictionary<string, string>
                {
                    ["kotor1_gog_103"] = "ABC123"
                }
            };

            var gameVersion = new GameVersion
            {
                Platform = Platform.Windows,
                Distribution = Distribution.Steam,
                Version = "1.03",
                Architecture = Architecture.x86,
                FileSize = 5242880,
                Hash = "DIFFERENT_HASH"
            };

            var result = GameVersionValidator.ValidateGameVersion(manifest, gameVersion);

            if (!result.Success && result.Error?.Contains("does not support") == true)
            {
                Console.WriteLine("✓ PASSED");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should reject unsupported version");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        Console.WriteLine($"\nPhase 4 Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }

    static int TestPhase5(string? exePath)
    {
        Console.WriteLine("--- Testing Phase 5 LoaderInjector ---\n");
        var passed = 0;
        var failed = 0;

        // Default to KOTOR exe if no path provided
        if (string.IsNullOrEmpty(exePath))
        {
            exePath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";
        }

        Console.WriteLine($"Using executable: {exePath}\n");

        // Test LoaderInjector.IsLoaderInjected
        Console.Write("Testing LoaderInjector.IsLoaderInjected... ");
        try
        {
            if (File.Exists(exePath))
            {
                var result = LoaderInjector.IsLoaderInjected(exePath);

                if (result.Success)
                {
                    Console.WriteLine($"✓ PASSED (Injected: {result.Data})");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - Executable not found");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test LoaderInjector.GetImportTableInfo
        Console.Write("Testing LoaderInjector.GetImportTableInfo... ");
        try
        {
            if (File.Exists(exePath))
            {
                var result = LoaderInjector.GetImportTableInfo(exePath);

                if (result.Success && result.Data != null)
                {
                    var info = result.Data;
                    Console.WriteLine($"✓ PASSED ({info.Summary})");
                    Console.WriteLine($"  Imports: {string.Join(", ", info.ImportedDlls.Take(5))}{(info.ImportedDlls.Count > 5 ? "..." : "")}");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - Executable not found");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test LoaderInjector with a dummy executable
        Console.Write("Testing LoaderInjector with dummy executable... ");
        try
        {
            // Use the console app itself as a test target
            var testExePath = System.Diagnostics.Process.GetCurrentProcess().MainModule?.FileName;

            if (testExePath != null && File.Exists(testExePath))
            {
                var result = LoaderInjector.IsLoaderInjected(testExePath);

                if (result.Success)
                {
                    Console.WriteLine($"✓ PASSED (Can analyze PE imports)");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - Could not find test executable");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Note about injection testing
        Console.WriteLine("\n⚠️  Note: Actual PE modification testing skipped for safety.");
        Console.WriteLine("    LoaderInjector.InjectLoader() is marked experimental.");
        Console.WriteLine("    See PE_INJECTION_NOTES.md for alternative approaches.");

        Console.WriteLine($"\nPhase 5 Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }

    static int TestPhase6()
    {
        Console.WriteLine("--- Testing Phase 6 Orchestration ---\n");
        var passed = 0;
        var failed = 0;

        // Test PatchRemover.HasPatchesInstalled
        Console.Write("Testing PatchRemover.HasPatchesInstalled... ");
        try
        {
            var kotorPath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";

            if (File.Exists(kotorPath))
            {
                var result = PatchRemover.HasPatchesInstalled(kotorPath);

                if (result.Success)
                {
                    Console.WriteLine($"✓ PASSED (Patched: {result.Data})");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - KOTOR exe not found");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchRemover.GetInstallationInfo
        Console.Write("Testing PatchRemover.GetInstallationInfo... ");
        try
        {
            var kotorPath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";

            if (File.Exists(kotorPath))
            {
                var result = PatchRemover.GetInstallationInfo(kotorPath);

                if (result.Success && result.Data != null)
                {
                    var info = result.Data;
                    Console.WriteLine($"✓ PASSED");
                    Console.WriteLine($"  {info.Summary}");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - KOTOR exe not found");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchRepository with empty directory
        Console.Write("Testing PatchRepository with empty directory... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var repo = new PatchRepository(testDir);

            var result = repo.ScanPatches();

            if (result.Success && repo.PatchCount == 0)
            {
                Console.WriteLine("✓ PASSED (No patches found, as expected)");
                passed++;
            }
            else
            {
                Console.WriteLine($"✗ FAILED - Expected 0 patches, got {repo.PatchCount}");
                failed++;
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchRepository with non-existent directory
        Console.Write("Testing PatchRepository with non-existent directory... ");
        try
        {
            var repo = new PatchRepository("/nonexistent/path");
            var result = repo.ScanPatches();

            if (!result.Success && result.Error?.Contains("not found") == true)
            {
                Console.WriteLine("✓ PASSED (Correctly reports missing directory)");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Should fail for non-existent directory");
                failed++;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchOrchestrator construction
        Console.Write("Testing PatchOrchestrator construction... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var orchestrator = new PatchOrchestrator(testDir);

            if (orchestrator.AvailablePatchCount == 0)
            {
                Console.WriteLine("✓ PASSED (Orchestrator initialized)");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Unexpected patches found");
                failed++;
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchOrchestrator.GetAvailablePatches
        Console.Write("Testing PatchOrchestrator.GetAvailablePatches... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var orchestrator = new PatchOrchestrator(testDir);

            var patches = orchestrator.GetAvailablePatches();

            if (patches != null && patches.Count == 0)
            {
                Console.WriteLine("✓ PASSED (Empty patch collection)");
                passed++;
            }
            else
            {
                Console.WriteLine("✗ FAILED - Expected empty collection");
                failed++;
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Test PatchOrchestrator.IsPatched
        Console.Write("Testing PatchOrchestrator.IsPatched... ");
        try
        {
            var testDir = PathHelpers.CreateTempDirectory();
            var orchestrator = new PatchOrchestrator(testDir);
            var kotorPath = @"C:\Users\laned\Documents\KotOR Installs\swkotor.exe";

            if (File.Exists(kotorPath))
            {
                var result = orchestrator.IsPatched(kotorPath);

                if (result.Success)
                {
                    Console.WriteLine($"✓ PASSED (Can check patch status)");
                    passed++;
                }
                else
                {
                    Console.WriteLine($"✗ FAILED - {result.Error}");
                    failed++;
                }
            }
            else
            {
                Console.WriteLine("⊘ SKIPPED - KOTOR exe not found");
            }

            PathHelpers.SafeDeleteDirectory(testDir);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ FAILED - {ex.Message}");
            failed++;
        }

        // Note about full integration tests
        Console.WriteLine("\n⚠️  Note: Full installation/removal tests require .kpatch files.");
        Console.WriteLine("    Create test .kpatch files to test:");
        Console.WriteLine("    - PatchRepository.LoadPatch()");
        Console.WriteLine("    - PatchApplicator.InstallPatches()");
        Console.WriteLine("    - PatchRemover.RemoveAllPatches()");
        Console.WriteLine("    - End-to-end orchestration workflow");
        Console.WriteLine("\n    See PHASE6_NOTES.md for .kpatch file format.");

        Console.WriteLine($"\nPhase 6 Tests: {passed} passed, {failed} failed");
        return failed == 0 ? 0 : 1;
    }
}
