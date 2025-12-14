# KPatchLauncher Application

KPatchLauncher is a dual-mode application that serves both as a patch management GUI and a game launcher with DLL injection. Built with Avalonia UI (.NET 8.0), it provides a cross-platform interface for installing patches via KPatchCore and launching games with KotorPatcher.dll injected at runtime.

## Architecture Overview

KPatchLauncher operates in two distinct modes:

1. **GUI Mode** (no arguments): Avalonia-based patch management interface with game version detection, patch compatibility checking, and installation management
2. **CLI Mode** (with arguments): Command-line game launcher that injects KotorPatcher.dll into the game process, bypassing the GUI entirely

The application uses the MVVM pattern with Avalonia's reactive UI framework. It leverages KPatchCore for patch installation logic and implements Windows API-based DLL injection for runtime patching.

## Project Structure

### Entry Point

**Program.cs**: Dual-mode entry point that routes to GUI (Avalonia) or CLI (game launcher) based on command-line arguments. Handles mode detection, Avalonia initialization, game executable discovery, patch detection, and vanilla vs patched launch routing.

### Application

**App.axaml.cs**: Avalonia application class. Initializes MainWindow with MainViewModel. Implements dynamic theme switching based on detected game title (KOTOR1 vs KOTOR2). Loads theme resource dictionaries from Themes folder.

### Process Injection

**ProcessInjector.cs**: Windows API-based DLL injection engine. Provides two injection strategies: direct injection (GOG/Physical distributions using CREATE_SUSPENDED) and delayed injection (Steam distribution with DRM decryption handling). Uses P/Invoke for CreateProcess, VirtualAllocEx, WriteProcessMemory, CreateRemoteThread, LoadLibraryA injection pattern. Validates game processes via PE header checking to distinguish real executables from Steam bootstrap stubs.

### ViewModels

**MainViewModel.cs**: Primary ViewModel for the main window. Manages patch repository, game path/patches path persistence, patch loading/scanning, installation/uninstallation orchestration, game launching with injection routing, version detection and compatibility checking, pending changes tracking, UI commands (Browse, Refresh, Apply, Uninstall, Launch). Uses async/await for background operations with Dispatcher.UIThread for UI updates.

**PatchItemViewModel.cs**: ViewModel for individual patch list items. Tracks patch metadata (ID, name, version, author, description), checked state with CheckedChanged event, orphaned status (installed but not in patches directory), compatibility status with detected game version. Provides DisplayText property for UI binding.

**ViewModelBase.cs**: Base class implementing INotifyPropertyChanged for all ViewModels. Provides SetProperty helper for automatic change notification.

**SimpleCommand.cs**: Lightweight ICommand implementation for UI commands. Supports execute actions and optional canExecute predicates. Provides RaiseCanExecuteChanged for manual command state updates.

### Models

**AppSettings.cs**: Application settings persistence using JSON serialization. Stores GamePath, PatchesPath, CheckedPatchIds. Saves to %AppData%/KPatchLauncher/settings.json. Provides static Load/Save methods with graceful error handling.

### Views

**MainWindow.axaml**: Main Avalonia window XAML defining the UI layout. Displays game path selector, patches directory selector, patch list with checkboxes, patch details panel, action buttons (Apply, Uninstall, Launch), status messages and progress indicators.

### Themes

**Kotor1Theme.axaml**: Color scheme and styling for KOTOR 1 (gold/brown theme).

**Kotor2Theme.axaml**: Color scheme and styling for KOTOR 2 (silver/blue theme).

Themes are dynamically loaded when game version is detected via App.LoadTheme().

## Core Functionality

### Dual-Mode Operation

**GUI Mode**: Launches when executed without arguments. Presents Avalonia UI for patch management. Allows browsing for game executable and patches directory. Displays available patches with compatibility indicators. Supports patch installation, uninstallation, and game launching.

**CLI Mode**: Launches when executed with arguments (e.g., "KPatchLauncher.exe swkotor.exe"). Automatically detects game executable in launcher directory or from first argument. Checks for patch_config.toml to determine patched vs vanilla launch. Injects KotorPatcher.dll if patches detected. Exits after launching game (or waits with --monitor flag).

### DLL Injection Strategies

**Direct Injection** (GOG, Physical, Other): Creates game process in suspended state via CreateProcess with CREATE_SUSPENDED flag. Allocates memory in target process, writes DLL path, creates remote thread calling LoadLibraryA. Resumes main thread after injection completes. Optimal for unencrypted executables.

**Delayed Injection** (Steam): Launches game normally via UseShellExecute to allow Steam DRM decryption. Polls for game process appearance using FindGameProcess with PE header validation. Validates process is real executable (not bootstrap) by reading DOS header MZ signature. Waits for initialization (main window creation). Injects DLL into running process after full initialization.

### Game Version Detection

Uses GameDetector.DetectVersion() from KPatchCore to compute SHA-256 hash of game executable. Looks up hash in known versions database (GOG, Steam, Physical distributions). Updates UI with game title, platform, distribution, version string. Switches application theme based on detected game title (KOTOR1/KOTOR2). Enables compatibility checking against patch requirements.

### Patch Compatibility Checking

After game version detection, MainViewModel.UpdatePatchCompatibility() validates each patch. Uses GameVersionValidator.ValidateGameVersion() to check if patch's supported_versions contains detected game SHA-256. Updates PatchItemViewModel.IsCompatible and CompatibilityStatus. Filters UI to show only compatible patches via VisiblePatches property. Orphaned patches always marked incompatible.

### Patch Installation Pipeline

**Pre-Installation**: Validates game path and patches directory. Scans patches directory via PatchRepository. Detects game version for compatibility checking.

**Installation**: Uninstalls existing patches via PatchRemover.RemoveAllPatches(). Locates KotorPatcher.dll in launcher directory (AppContext.BaseDirectory). Builds InstallOptions with checked patch IDs. Calls PatchApplicator.InstallPatches() on background thread. Updates UI on completion via Dispatcher.UIThread.InvokeAsync().

**Post-Installation**: Refreshes patch status via CheckPatchStatusAsync(). Updates checked state to match installed patches. Detects orphaned patches (installed but missing from patches directory).

### Orphaned Patch Handling

Occurs when patch is installed but .kpatch file removed from patches directory. Detected by comparing installed patches (from patch_config.toml) with scanned patches. Added to UI with "(not found)" suffix, "?" version, "Unknown" author. Marked IsOrphaned = true and IsCompatible = false. Can be uninstalled but not reinstalled without .kpatch file.

### Settings Persistence

Saves GamePath, PatchesPath, CheckedPatchIds to JSON file on every change. Automatically loads settings on startup. Restores checked state when patches are reloaded. Stores in per-user AppData directory (%AppData%/KPatchLauncher/).

### Game Launching

**Vanilla Launch**: Detected when patch_config.toml missing from game directory. Launches game directly via Process.Start with UseShellExecute. No injection performed.

**Patched Launch**: Detected when patch_config.toml and KotorPatcher.dll present. Routes to ProcessInjector.LaunchWithInjection() with detected distribution. Uses direct injection for GOG/Physical, delayed injection for Steam. Reports process ID on successful launch.

## Windows API Functions

**CreateProcess**: Creates new process in suspended state for injection before game initialization.

**OpenProcess**: Opens existing process handle with specific access rights for memory operations.

**VirtualAllocEx**: Allocates executable memory in target process for DLL path string.

**WriteProcessMemory**: Writes DLL path to allocated memory in target process.

**ReadProcessMemory**: Reads DOS header from target process for PE validation (Steam bootstrap detection).

**CreateRemoteThread**: Creates thread in target process that calls LoadLibraryA with DLL path.

**WaitForSingleObject**: Waits for remote thread completion (LoadLibrary call).

**GetModuleHandle/GetProcAddress**: Resolves LoadLibraryA address from kernel32.dll.

**ResumeThread**: Resumes suspended main thread after injection completes.

All API calls use error checking via Marshal.GetLastWin32Error() with detailed logging to console.

## Steam-Specific Injection Challenges

**Challenge**: Steam DRM encrypts executable; early injection fails due to invalid PE headers.

**Solution**: Delayed injection strategy. Launch normally, let Steam decrypt, then inject into running process.

**Bootstrap Detection**: Steam creates temporary bootstrap process that exits quickly. Uses PE header validation (DOS MZ signature at 0x400000) to distinguish real game from bootstrap. Tracks checked PIDs to avoid re-validating same process. Waits 500ms after validation for stability check.

**Initialization Wait**: After finding valid process, waits for MainWindowHandle != IntPtr.Zero to ensure game fully initialized before injection.

## UI Architecture (MVVM)

**Model**: PatchManifest, GameVersion, PatchResult (from KPatchCore), AppSettings (local).

**ViewModel**: MainViewModel (application state, commands), PatchItemViewModel (individual patch state). ViewModelBase provides INotifyPropertyChanged infrastructure. SimpleCommand implements ICommand for button bindings.

**View**: MainWindow.axaml defines XAML layout. Data binding to ViewModel properties. Command binding to ViewModel commands. Theme resources from Themes folder.

**Reactive Updates**: Uses OnPropertyChanged notifications for UI updates. Dispatcher.UIThread.InvokeAsync() for background thread → UI thread transitions. ObservableCollection&lt;PatchItemViewModel&gt; for automatic list updates.

## Error Handling

All operations use PatchResult pattern from KPatchCore for expected failures. Installation/uninstallation errors reported via StatusMessage in UI. CLI mode prints errors to console and returns non-zero exit codes. DLL injection failures terminate suspended process to prevent orphaned processes. Silent fallbacks for non-critical operations (settings persistence, theme loading).

## Debug Features

**Debug Mode** (disabled by default): Hardcoded false flag in LaunchDirectWithInjection. When enabled, pauses after injection before resuming thread. Allows debugger attachment (Cheat Engine, x32dbg). Prints process ID and waits for ENTER key press.

**Console Logging**: CLI mode uses Console.WriteLine for detailed operation logging. DLL injection logs all API calls with addresses and results. Steam injection logs process validation details (PID, PE header bytes, stability checks).

## Dependencies

**Avalonia UI**: Cross-platform UI framework (.NET 8.0). Provides MVVM infrastructure, data binding, theming, file pickers.

**KPatchCore**: Installation-time framework for patch management. Used for PatchRepository, PatchApplicator, PatchRemover, GameDetector, validators.

**System.Text.Json**: Settings serialization/deserialization.

**Windows API** (P/Invoke): Process creation, memory manipulation, thread creation for DLL injection. Windows-only functionality (Linux/macOS would require different injection method).

## Technical Constraints

**Platform**: GUI is cross-platform (Avalonia), but DLL injection is Windows-only (x86 32-bit targets).

**Injection Method**: Classic LoadLibraryA injection via remote thread. Requires target process same or lower privilege level. Cannot inject across privilege boundaries without elevation.

**Steam DRM**: Delayed injection required for Steam distributions. Timing-sensitive (must wait for decryption and initialization). PE header validation prevents false positives on bootstrap processes.

**Single-File Deployment**: Uses AppContext.BaseDirectory instead of Assembly.Location for single-file publish support. KotorPatcher.dll must be in same directory as launcher executable.

## Deployment

**Standard Deployment**: KPatchLauncher.exe, KotorPatcher.dll, and KPatchCore.dll in same directory. Patches directory with .kpatch files (can be in separate location, specified via UI).

**Single-File Deployment**: .NET 8.0 supports single-file publish for KPatchLauncher.exe. KotorPatcher.dll must remain separate (cannot be embedded, must be injectable). Uses AppContext.BaseDirectory to locate KotorPatcher.dll after single-file extraction.

**AppData Storage**: Settings stored per-user in %AppData%/KPatchLauncher/settings.json. No registry or system-wide configuration.
