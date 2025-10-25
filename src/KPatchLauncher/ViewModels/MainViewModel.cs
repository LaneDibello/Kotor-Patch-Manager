using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Platform.Storage;
using Avalonia.Threading;
using KPatchCore.Managers;
using KPatchCore.Models;
using KPatchCore.Applicators;
using KPatchCore.Detectors;
using KPatchLauncher.Models;

namespace KPatchLauncher.ViewModels;

public class MainViewModel : ViewModelBase
{
    private string _gamePath = string.Empty;
    private string _patchesPath = string.Empty;
    private string _statusMessage = "Ready";
    private string _kotorVersion = "Unknown";
    private PatchItemViewModel? _selectedPatch;
    private PatchRepository? _repository;
    private readonly AppSettings _settings;
    private bool _hasInstalledPatches;

    public MainViewModel()
    {
        AllPatches = new ObservableCollection<PatchItemViewModel>();

        // Load settings
        _settings = AppSettings.Load();
        _gamePath = _settings.GamePath;
        _patchesPath = _settings.PatchesPath;

        // Create simple commands
        BrowseGameCommand = new SimpleCommand(async () => await BrowseGame());
        BrowsePatchesCommand = new SimpleCommand(async () => await BrowsePatches());
        RefreshCommand = new SimpleCommand(async () => await Refresh());
        MoveUpCommand = new SimpleCommand(() => MoveUp());
        MoveDownCommand = new SimpleCommand(() => MoveDown());
        ApplyPatchesCommand = new SimpleCommand(async () => await ApplyPatches());
        UninstallAllCommand = new SimpleCommand(async () => await UninstallAll(), () => HasInstalledPatches);
        LaunchGameCommand = new SimpleCommand(async () => await LaunchGame());

        // Load patches if path is set
        if (!string.IsNullOrWhiteSpace(_patchesPath))
        {
            _ = LoadPatchesFromDirectoryAsync(_patchesPath);
        }
    }

    public ObservableCollection<PatchItemViewModel> AllPatches { get; }

    public PatchItemViewModel? SelectedPatch
    {
        get => _selectedPatch;
        set => SetProperty(ref _selectedPatch, value);
    }

    public bool HasInstalledPatches
    {
        get => _hasInstalledPatches;
        private set
        {
            if (SetProperty(ref _hasInstalledPatches, value))
            {
                ((SimpleCommand)UninstallAllCommand).RaiseCanExecuteChanged();
            }
        }
    }

    public string GamePath
    {
        get => _gamePath;
        set
        {
            if (SetProperty(ref _gamePath, value))
            {
                _settings.GamePath = value;
                _settings.Save();

                // Check patch status when game path is set
                if (!string.IsNullOrWhiteSpace(value) && File.Exists(value))
                {
                    _ = CheckPatchStatusAsync(value);
                }
            }
        }
    }

    public string PatchesPath
    {
        get => _patchesPath;
        set
        {
            if (SetProperty(ref _patchesPath, value))
            {
                _settings.PatchesPath = value;
                _settings.Save();

                // Load patches from new directory
                if (!string.IsNullOrWhiteSpace(value))
                {
                    _ = LoadPatchesFromDirectoryAsync(value);
                }
            }
        }
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set => SetProperty(ref _statusMessage, value);
    }

    public string KotorVersion
    {
        get => _kotorVersion;
        set => SetProperty(ref _kotorVersion, value);
    }

    public int PendingChangesCount
    {
        get
        {
            var checkedPatches = AllPatches.Where(p => p.IsChecked).Select(p => p.Id).OrderBy(x => x).ToList();
            var installedPatches = AllPatches.Where(p => !p.IsOrphaned && IsInstalled(p.Id)).Select(p => p.Id).OrderBy(x => x).ToList();

            if (checkedPatches.SequenceEqual(installedPatches))
                return 0;

            return checkedPatches.Union(installedPatches).Except(checkedPatches.Intersect(installedPatches)).Count();
        }
    }

    public string PendingChangesMessage => PendingChangesCount == 0
        ? "No pending changes"
        : $"{PendingChangesCount} patch{(PendingChangesCount == 1 ? "" : "es")} pending";

    public ICommand BrowseGameCommand { get; }
    public ICommand BrowsePatchesCommand { get; }
    public ICommand RefreshCommand { get; }
    public ICommand MoveUpCommand { get; }
    public ICommand MoveDownCommand { get; }
    public ICommand ApplyPatchesCommand { get; }
    public ICommand UninstallAllCommand { get; }
    public ICommand LaunchGameCommand { get; }

    private bool IsInstalled(string patchId)
    {
        // Check if patch is currently installed in the game
        var info = PatchRemover.GetInstallationInfo(GamePath);
        return info.Success && info.Data != null && info.Data.InstalledPatches.Contains(patchId);
    }

    private async Task BrowseGame()
    {
        try
        {
            var window = GetMainWindow();
            if (window == null)
            {
                StatusMessage = "Error: Could not access window";
                return;
            }

            var result = await window.StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
            {
                Title = "Select Game Executable",
                AllowMultiple = false,
                FileTypeFilter = new[]
                {
                    new FilePickerFileType("Executable Files")
                    {
                        Patterns = new[] { "*.exe" }
                    }
                }
            });

            if (result.Count > 0)
            {
                GamePath = result[0].Path.LocalPath;
                StatusMessage = $"Selected game: {Path.GetFileName(GamePath)}";
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error browsing: {ex.Message}";
        }
    }

    private async Task BrowsePatches()
    {
        try
        {
            var window = GetMainWindow();
            if (window == null)
            {
                StatusMessage = "Error: Could not access window";
                return;
            }

            var result = await window.StorageProvider.OpenFolderPickerAsync(new FolderPickerOpenOptions
            {
                Title = "Select Patches Directory",
                AllowMultiple = false
            });

            if (result.Count > 0)
            {
                PatchesPath = result[0].Path.LocalPath;
                StatusMessage = $"Selected patches directory: {Path.GetFileName(PatchesPath)}";
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error browsing: {ex.Message}";
        }
    }

    private async Task Refresh()
    {
        try
        {
            StatusMessage = "Refreshing...";

            // Reload patches from directory if path is set
            if (!string.IsNullOrWhiteSpace(PatchesPath))
            {
                await LoadPatchesFromDirectoryAsync(PatchesPath);
            }

            StatusMessage = "Refresh complete";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error refreshing: {ex.Message}";
        }
    }

    private Window? GetMainWindow()
    {
        if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            return desktop.MainWindow;
        }
        return null;
    }

    public void OnPatchChecked(PatchItemViewModel patch)
    {
        if (patch.IsChecked)
        {
            // Move to top of list
            var index = AllPatches.IndexOf(patch);
            if (index > 0)
            {
                AllPatches.Move(index, 0);
            }
        }

        SaveCheckedPatches();
        UpdatePendingChanges();
    }

    private void MoveUp()
    {
        if (SelectedPatch == null)
            return;

        var patch = SelectedPatch;
        var index = AllPatches.IndexOf(patch);
        if (index > 0)
        {
            AllPatches.Move(index, index - 1);
            StatusMessage = $"Moved {patch.Name} up";
            UpdatePendingChanges();
        }
    }

    private void MoveDown()
    {
        if (SelectedPatch == null)
            return;

        var patch = SelectedPatch;
        var index = AllPatches.IndexOf(patch);
        if (index < AllPatches.Count - 1)
        {
            AllPatches.Move(index, index + 1);
            StatusMessage = $"Moved {patch.Name} down";
            UpdatePendingChanges();
        }
    }

    private void SaveCheckedPatches()
    {
        _settings.CheckedPatchIds = AllPatches.Where(p => p.IsChecked).Select(p => p.Id).ToList();
        _settings.Save();
    }

    private void UpdatePendingChanges()
    {
        OnPropertyChanged(nameof(PendingChangesCount));
        OnPropertyChanged(nameof(PendingChangesMessage));
        StatusMessage = PendingChangesMessage;
    }

    private async Task ApplyPatches()
    {
        if (string.IsNullOrWhiteSpace(GamePath) || !File.Exists(GamePath))
        {
            StatusMessage = "Error: Invalid game executable path";
            return;
        }

        if (_repository == null)
        {
            StatusMessage = "Error: No patches loaded";
            return;
        }

        try
        {
            var checkedPatches = AllPatches.Where(p => p.IsChecked && !p.IsOrphaned).ToList();

            // If no patches are checked, uninstall all
            if (checkedPatches.Count == 0)
            {
                StatusMessage = "Uninstalling all patches...";

                var uninstallResult = await Task.Run(() =>
                    PatchRemover.RemoveAllPatches(GamePath));

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (uninstallResult.Success)
                    {
                        StatusMessage = "All patches uninstalled successfully";
                        UpdatePendingChanges();
                    }
                    else
                    {
                        StatusMessage = $"Error: {uninstallResult.Error}";
                    }
                });
                return;
            }

            // Otherwise, uninstall existing patches and install checked ones
            StatusMessage = "Applying patches...";

            // First, uninstall any existing patches
            await Task.Run(() => PatchRemover.RemoveAllPatches(GamePath));

            // Get launcher exe path (current application)
            var launcherPath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            launcherPath = launcherPath.Replace(".dll", ".exe");

            // Get patcher DLL path (should be in same directory as launcher)
            var launcherDir = Path.GetDirectoryName(launcherPath);
            var binDir = Path.GetDirectoryName(Path.GetDirectoryName(launcherDir));
            var patcherDir = Path.Combine(binDir ?? "", "Patcher\\Debug");
            var patcherDllPath = Path.Combine(patcherDir ?? "", "KotorPatcher.dll");

            var applicator = new PatchApplicator(_repository);
            var options = new PatchApplicator.InstallOptions
            {
                GameExePath = GamePath,
                PatchIds = checkedPatches.Select(p => p.Id).ToList(),
                CreateBackup = true,
                PatcherDllPath = File.Exists(patcherDllPath) ? patcherDllPath : null,
                LauncherExePath = File.Exists(launcherPath) ? launcherPath : null,
                CopyLauncher = true
            };

            // Run on background thread
            var result = await Task.Run(() => applicator.InstallPatches(options));

            // Update UI on UI thread
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (result.Success)
                {
                    StatusMessage = $"Patches applied successfully ({result.InstalledPatches.Count} patches)";
                    UpdatePendingChanges();
                }
                else
                {
                    StatusMessage = $"Error: {result.Error}";
                }
            });

            // Refresh installed status
            await CheckPatchStatusAsync(GamePath);
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Error applying patches: {ex.Message}";
            });
        }
    }

    private async Task UninstallAll()
    {
        if (string.IsNullOrWhiteSpace(GamePath) || !File.Exists(GamePath))
        {
            StatusMessage = "Error: Invalid game executable path";
            return;
        }

        try
        {
            // Uncheck all patches
            foreach (var patch in AllPatches)
            {
                patch.IsChecked = false;
            }

            SaveCheckedPatches();

            // Now apply (which will uninstall since nothing is checked)
            await ApplyPatches();
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Error: {ex.Message}";
            });
        }
    }

    private async Task LaunchGame()
    {
        if (string.IsNullOrWhiteSpace(GamePath) || !File.Exists(GamePath))
        {
            StatusMessage = "Error: Invalid game executable path";
            return;
        }

        try
        {
            var gameDir = Path.GetDirectoryName(GamePath);
            if (gameDir == null)
            {
                StatusMessage = "Error: Invalid game directory";
                return;
            }

            var patcherDllPath = Path.Combine(gameDir, "KotorPatcher.dll");
            var patchConfigPath = Path.Combine(gameDir, "patch_config.toml");

            // Check if patches are installed
            if (!File.Exists(patchConfigPath))
            {
                // No patches - launch vanilla
                StatusMessage = "Launching game (no patches detected)...";

                var vanillaProcess = await Task.Run(() => Process.Start(new ProcessStartInfo
                {
                    FileName = GamePath,
                    WorkingDirectory = gameDir,
                    UseShellExecute = true
                }));

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (vanillaProcess != null)
                    {
                        StatusMessage = $"Game launched (PID: {vanillaProcess.Id})";
                    }
                    else
                    {
                        StatusMessage = "Error: Failed to launch game";
                    }
                });
                return;
            }

            // Patches detected - launch with injection
            if (!File.Exists(patcherDllPath))
            {
                StatusMessage = "Error: KotorPatcher.dll not found (patches are configured but DLL is missing)";
                return;
            }

            StatusMessage = "Launching game with patches...";

            var result = await Task.Run(() =>
                ProcessInjector.LaunchWithInjection(GamePath, patcherDllPath));

            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (result.Success && result.Data != null)
                {
                    StatusMessage = $"Game launched with patches (PID: {result.Data.Id})";
                }
                else
                {
                    StatusMessage = $"Error: {result.Error}";
                }
            });
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Error launching game: {ex.Message}";
            });
        }
    }

    private async Task LoadPatchesFromDirectoryAsync(string directory)
    {
        try
        {
            // Clear existing patches on UI thread
            AllPatches.Clear();
            StatusMessage = "Loading patches...";

            // Do heavy work on background thread
            var (repository, scanResult, allPatches) = await Task.Run(() =>
            {
                var repo = new PatchRepository(directory);
                var result = repo.ScanPatches();
                var patches = result.Success ? repo.GetAllPatches() : null;
                return (repo, result, patches);
            });

            // Update UI on UI thread
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Back on UI thread for result handling
                if (!scanResult.Success)
                {
                    StatusMessage = $"Error loading patches: {scanResult.Error}";
                    return;
                }

                _repository = repository;

                if (allPatches == null)
                {
                    StatusMessage = "Error: No patches found";
                    return;
                }

                var patchViewModels = allPatches.Values.Select(entry => new PatchItemViewModel
                {
                    Id = entry.Manifest.Id,
                    Name = entry.Manifest.Name,
                    Version = entry.Manifest.Version,
                    Author = entry.Manifest.Author,
                    Description = entry.Manifest.Description
                }).ToList();

                // Restore checked state from settings
                var checkedIds = _settings.CheckedPatchIds.ToHashSet();

                foreach (var patch in patchViewModels)
                {
                    patch.IsChecked = checkedIds.Contains(patch.Id);
                    AllPatches.Add(patch);
                }

                StatusMessage = $"Loaded {patchViewModels.Count} patches from {Path.GetFileName(directory)}";
            });

            // Check patch status if we have a game path set
            if (!string.IsNullOrWhiteSpace(GamePath) && File.Exists(GamePath))
            {
                await CheckPatchStatusAsync(GamePath);
            }
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Error loading patches: {ex.Message}";
            });
        }
    }

    private async Task CheckPatchStatusAsync(string gameExePath)
    {
        if (_repository == null)
            return;

        try
        {
            // Get installation info and game version on background thread
            var (installInfo, versionInfo) = await Task.Run(() =>
            {
                var install = PatchRemover.GetInstallationInfo(gameExePath);
                var version = GameDetector.DetectVersion(gameExePath);
                return (install, version);
            });

            // Update game version display
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (versionInfo.Success && versionInfo.Data != null)
                {
                    var v = versionInfo.Data;
                    KotorVersion = $"{v.DisplayName})";
                }
                else
                {
                    KotorVersion = "Unknown";
                }
            });

            if (!installInfo.Success || installInfo.Data == null)
            {
                HasInstalledPatches = false;
                return;
            }

            var info = installInfo.Data;

            // Update UI on UI thread
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                HasInstalledPatches = info.InstalledPatches.Count > 0;

                if (info.InstalledPatches.Count == 0)
                {
                    StatusMessage = "No patches currently installed";
                    UpdatePendingChanges();
                    return;
                }

                var installedIds = info.InstalledPatches.ToHashSet();

                // Check boxes for installed patches
                foreach (var patch in AllPatches)
                {
                    if (installedIds.Contains(patch.Id))
                    {
                        patch.IsChecked = true;
                    }
                }

                // Add orphaned patches (installed but not in directory)
                foreach (var patchId in info.InstalledPatches)
                {
                    if (!AllPatches.Any(p => p.Id == patchId))
                    {
                        var orphanedPatch = new PatchItemViewModel
                        {
                            Id = patchId,
                            Name = $"{patchId} (not found)",
                            Version = "?",
                            Author = "Unknown",
                            Description = "This patch is installed but not found in patches directory",
                            IsOrphaned = true,
                            IsChecked = true
                        };
                        AllPatches.Insert(0, orphanedPatch);
                    }
                }

                SaveCheckedPatches();
                UpdatePendingChanges();
                StatusMessage = $"Found {info.InstalledPatches.Count} installed patches";
            });
        }
        catch (Exception ex)
        {
            // Silent failure - not critical
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Could not check patch status: {ex.Message}";
            });
        }
    }
}
