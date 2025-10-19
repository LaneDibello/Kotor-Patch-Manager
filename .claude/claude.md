# Claude Code Session Notes

## Project-Specific Information

### Build System Notes

**Important**: The `dotnet` command is not available directly in Bash on this WSL environment.

To build .NET projects, use the full path to the dotnet executable:
```bash
/mnt/c/Program\ Files/dotnet/dotnet.exe build
```

Or from a project directory:
```bash
cd /mnt/c/Users/laned/source/Repos/KotOR\ Patch\ Manager/KPatchCore
/mnt/c/Program\ Files/dotnet/dotnet.exe build
```

### Project Structure

- **KPatchCore**: C# library for patch management (installation time)
- **KPatchConsole**: C# console application for CLI
- **KPatchLauncher**: C# launcher for DLL injection (RECOMMENDED approach)
- **KotorPatcher**: C++ runtime DLL (loaded by game at runtime)
- **Patches/**: Example patches (EnableScriptAurPostString)

### Current Implementation Status

**C# Components (KPatchCore)**:
- ✅ Phase 1: Models & Common utilities
- ✅ Phase 2: Parsers (ManifestParser, HooksParser, ExecutableParser)
- ✅ Phase 3: Simple Operations (BackupManager, ConfigGenerator, GameDetector)
- ✅ Phase 4: Validators (PatchValidator, HookValidator, DependencyValidator, GameVersionValidator)
- ✅ Phase 5: PE Manipulation (LoaderInjector - experimental, not recommended)
- ✅ Phase 6: Orchestration (PatchApplicator, PatchRemover, PatchRepository, PatchOrchestrator)
- ✅ Phase 7: Console Application (KPatchConsole with install/uninstall/list/status)

**C# Launcher (KPatchLauncher)**:
- ✅ DLL injection via CreateProcess with CREATE_SUSPENDED
- ✅ Thread hijacking for KotorPatcher.dll injection
- ✅ Proper cleanup and error handling

**C++ Components (KotorPatcher)**:
- ✅ Wrapper system with x86 runtime code generation
- ✅ Hook types (INLINE with parameter extraction, REPLACE, WRAP)
- ✅ Configuration parsing (TOML with Tomlyn)
- ✅ Parameter extraction for INLINE hooks
- See: `KotorPatcher/docs/IMPLEMENTATION_COMPLETE.md`

### Testing

- Test KOTOR executable location: `C:\Users\laned\Documents\KotOR Installs\swkotor.exe`
- Always create backups before testing
- Use DebugView for C++ runtime debugging

### Key Decisions

- Using PatchResult pattern instead of exceptions for expected failures
- Launcher-based injection (KPatchLauncher.exe) is RECOMMENDED; PE modification experimental
- No auto-detection of game installations in MVP (using explicit paths)
- Parameter extraction system allows clean C functions instead of inline assembly
- SafeDeleteFile helper for maintainable file cleanup
- Backup files cleaned up automatically after restore

### Recent Improvements

- ✅ Backup cleanup after restore (no orphaned backup files)
- ✅ Refactored RemoveAllPatches with SafeDeleteFile helper
- ✅ Parameter extraction system fully documented and working
- ✅ Updated documentation to reflect current architecture
