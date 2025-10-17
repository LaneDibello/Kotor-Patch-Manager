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
- **KotorPatcher**: C++ runtime DLL (loaded by game at runtime)

### Current Implementation Status

**C# Components (KPatchCore)**:
- ✅ Phase 1: Models & Common utilities
- ✅ Phase 2: Parsers (ManifestParser, HooksParser, ExecutableParser)
- ✅ Phase 3: Simple Operations (BackupManager, ConfigGenerator, GameDetector)
- ✅ Phase 4: Validators (PatchValidator, HookValidator, DependencyValidator, GameVersionValidator)
- ✅ Phase 5: PE Manipulation (LoaderInjector - experimental, see PE_INJECTION_NOTES.md)
- ⏳ Phase 6: Orchestration (PatchApplicator, PatchRemover, PatchRepository, PatchOrchestrator)
- ⏳ Phase 7: Console Application (KPatchConsole commands)

**C++ Components (KotorPatcher)**:
- ✅ Complete: Wrapper system with x86 runtime code generation
- ✅ Complete: Hook types (INLINE, REPLACE, WRAP)
- ✅ Complete: Configuration parsing (TOML)
- See: `KotorPatcher/docs/IMPLEMENTATION_COMPLETE.md`

### Testing

- Test KOTOR executable location: `C:\Users\laned\Documents\KotOR Installs\swkotor.exe`
- Always create backups before testing
- Use DebugView for C++ runtime debugging

### Key Decisions

- Using PatchResult pattern instead of exceptions for expected failures
- LoaderInjector PE modification is experimental; launcher-based injection recommended for production
- No auto-detection of game installations in MVP (using explicit paths)
