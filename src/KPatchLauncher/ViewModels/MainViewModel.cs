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
    private PatchItemViewModel? _selectedAvailablePatch;
    private PatchItemViewModel? _selectedActivePatch;
    private PatchRepository? _repository;
    private readonly AppSettings _settings;
    private bool _hasActivePatches;
    private bool _hasInstalledPatches;
    private HashSet<string> _installedPatchIds = new();

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

    public HashSet<string> InstalledPatchIds
    {
        get => _installedPatchIds;
        private set
        {
            _installedPatchIds = value;
            HasInstalledPatches = value.Count > 0;
            // Notify all patches to update their IsInstalled state
            foreach (var patch in AvailablePatches.Concat(ActivePatches))
            {
                patch.UpdateInstalledState(value);
            }
        }
    }

    public MainViewModel()
    {
        AvailablePatches = new ObservableCollection<PatchItemViewModel>();
        ActivePatches = new ObservableCollection<PatchItemViewModel>();

        // Load settings
        _settings = AppSettings.Load();
        _gamePath = _settings.GamePath;
        _patchesPath = _settings.PatchesPath;

        // Create simple commands
        BrowseGameCommand = new SimpleCommand(async () => await BrowseGame());
        BrowsePatchesCommand = new SimpleCommand(async () => await BrowsePatches());
        RefreshCommand = new SimpleCommand(async () => await Refresh());
        AddPatchCommand = new SimpleCommand(() => AddPatch());
        RemovePatchCommand = new SimpleCommand(() => RemovePatch());
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

    public ObservableCollection<PatchItemViewModel> AvailablePatches { get; }
    public ObservableCollection<PatchItemViewModel> ActivePatches { get; }

    public bool HasActivePatches
    {
        get => _hasActivePatches;
        private set => SetProperty(ref _hasActivePatches, value);
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

    public PatchItemViewModel? SelectedAvailablePatch
    {
        get => _selectedAvailablePatch;
        set => SetProperty(ref _selectedAvailablePatch, value);
    }

    public PatchItemViewModel? SelectedActivePatch
    {
        get => _selectedActivePatch;
        set => SetProperty(ref _selectedActivePatch, value);
    }

    public ICommand BrowseGameCommand { get; }
    public ICommand BrowsePatchesCommand { get; }
    public ICommand RefreshCommand { get; }
    public ICommand AddPatchCommand { get; }
    public ICommand RemovePatchCommand { get; }
    public ICommand MoveUpCommand { get; }
    public ICommand MoveDownCommand { get; }
    public ICommand ApplyPatchesCommand { get; }
    public ICommand UninstallAllCommand { get; }
    public ICommand LaunchGameCommand { get; }

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
                    },
                    new FilePickerFileType("All Files")
                    {
                        Patterns = new[] { "*" }
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

    private void AddPatch()
    {
        if (SelectedAvailablePatch == null)
            return;

        var patch = SelectedAvailablePatch;
        ActivePatches.Add(patch);
        AvailablePatches.Remove(patch);
        HasActivePatches = ActivePatches.Count > 0;
        StatusMessage = $"Added patch: {patch.Name}";
        SaveActivePatches();
    }

    private async void RemovePatch()
    {
        if (SelectedActivePatch == null)
            return;

        var patch = SelectedActivePatch;

        // Warn if removing an orphaned patch
        if (patch.IsOrphaned)
        {
            var window = GetMainWindow();
            if (window != null)
            {
                var dialog = new Window
                {
                    Title = "Remove Orphaned Patch",
                    Width = 450,
                    Height = 200,
                    WindowStartupLocation = WindowStartupLocation.CenterOwner,
                    Background = new Avalonia.Media.SolidColorBrush(Avalonia.Media.Color.Parse("#000016")),
                    Foreground = new Avalonia.Media.SolidColorBrush(Avalonia.Media.Color.Parse("#00AFFF"))
                };

                var panel = new StackPanel
                {
                    Margin = new Avalonia.Thickness(20),
                    Spacing = 15
                };

                panel.Children.Add(new TextBlock
                {
                    Text = "Warning: Orphaned Patch",
                    FontWeight = Avalonia.Media.FontWeight.Bold,
                    FontSize = 16,
                    Foreground = new Avalonia.Media.SolidColorBrush(Avalonia.Media.Color.Parse("#FF0000"))
                });

                panel.Children.Add(new TextBlock
                {
                    Text = $"The patch '{patch.Name}' is installed but not found in your patches directory.\n\nRemoving it will uninstall it from the game, but it will NOT appear in the Available Patches list.",
                    TextWrapping = Avalonia.Media.TextWrapping.Wrap
                });

                var buttonPanel = new StackPanel
                {
                    Orientation = Avalonia.Layout.Orientation.Horizontal,
                    HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Right,
                    Spacing = 10
                };

                var okButton = new Button
                {
                    Content = "Remove Anyway",
                    Width = 120,
                    Height = 35
                };
                okButton.Click += (s, e) => dialog.Close(true);

                var cancelButton = new Button
                {
                    Content = "Cancel",
                    Width = 120,
                    Height = 35
                };
                cancelButton.Click += (s, e) => dialog.Close(false);

                buttonPanel.Children.Add(cancelButton);
                buttonPanel.Children.Add(okButton);
                panel.Children.Add(buttonPanel);

                dialog.Content = panel;

                var result = await dialog.ShowDialog<bool>(window);
                if (!result)
                {
                    StatusMessage = "Cancelled removal";
                    return;
                }
            }
        }

        // Don't add orphaned patches back to Available list
        if (!patch.IsOrphaned)
        {
            AvailablePatches.Add(patch);
        }

        ActivePatches.Remove(patch);
        HasActivePatches = ActivePatches.Count > 0;
        StatusMessage = $"Removed patch: {patch.Name}";
        SaveActivePatches();
    }

    private void SaveActivePatches()
    {
        _settings.ActivePatchIds = ActivePatches.Select(p => p.Id).ToList();
        _settings.Save();
    }

    private void MoveUp()
    {
        if (SelectedActivePatch == null)
            return;

        var patch = SelectedActivePatch;
        var index = ActivePatches.IndexOf(patch);
        if (index > 0)
        {
            ActivePatches.Move(index, index - 1);
            StatusMessage = $"Moved {patch.Name} up";
            SaveActivePatches();
        }
    }

    private void MoveDown()
    {
        if (SelectedActivePatch == null)
            return;

        var patch = SelectedActivePatch;
        var index = ActivePatches.IndexOf(patch);
        if (index < ActivePatches.Count - 1)
        {
            ActivePatches.Move(index, index + 1);
            StatusMessage = $"Moved {patch.Name} down";
            SaveActivePatches();
        }
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
            // If no patches are selected, uninstall all
            if (ActivePatches.Count == 0)
            {
                StatusMessage = "Uninstalling all patches...";

                var uninstallResult = await Task.Run(() =>
                    PatchRemover.RemoveAllPatches(GamePath));

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (uninstallResult.Success)
                    {
                        StatusMessage = "All patches uninstalled successfully";
                    }
                    else
                    {
                        StatusMessage = $"Error: {uninstallResult.Error}";
                    }
                });
                return;
            }

            // Otherwise, uninstall existing patches and install selected ones
            StatusMessage = "Applying patches...";

            // First, uninstall any existing patches
            await Task.Run(() => PatchRemover.RemoveAllPatches(GamePath));

            // Get launcher exe path (current application)
            var launcherPath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            launcherPath = launcherPath.Replace(".dll", ".exe");

            // Get patcher DLL path (should be in same directory as launcher)
            var launcherDir = Path.GetDirectoryName(launcherPath);
            var patcherDllPath = Path.Combine(launcherDir ?? "", "KotorPatcher.dll");

            var applicator = new PatchApplicator(_repository);
            var options = new PatchApplicator.InstallOptions
            {
                GameExePath = GamePath,
                PatchIds = ActivePatches.Select(p => p.Id).ToList(),
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
                    // Update installed patch IDs
                    InstalledPatchIds = result.InstalledPatches.ToHashSet();
                    StatusMessage = $"Patches applied successfully ({result.InstalledPatches.Count} patches)";
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
            // Move all active patches back to available
            var patchesToMove = ActivePatches.ToList();
            foreach (var patch in patchesToMove)
            {
                ActivePatches.Remove(patch);
                AvailablePatches.Add(patch);
            }

            HasActivePatches = false;
            SaveActivePatches();

            // Now apply (which will uninstall since Active is empty)
            await ApplyPatches();

            // Clear installed patch IDs after successful uninstall
            InstalledPatchIds = new HashSet<string>();
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
            AvailablePatches.Clear();
            ActivePatches.Clear();
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

                // Restore active patches from settings
                var activePatchIds = _settings.ActivePatchIds.ToHashSet();
                var orderedActivePatches = new List<PatchItemViewModel>();

                // Add patches in the order they were saved
                foreach (var patchId in _settings.ActivePatchIds)
                {
                    var patch = patchViewModels.FirstOrDefault(p => p.Id == patchId);
                    if (patch != null)
                    {
                        orderedActivePatches.Add(patch);
                    }
                }

                // Add remaining patches to available
                foreach (var patch in patchViewModels)
                {
                    if (activePatchIds.Contains(patch.Id))
                    {
                        // Already in active list
                        continue;
                    }
                    AvailablePatches.Add(patch);
                }

                // Add active patches in order
                foreach (var patch in orderedActivePatches)
                {
                    ActivePatches.Add(patch);
                }

                HasActivePatches = ActivePatches.Count > 0;
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
                    KotorVersion = v.DisplayName;
                }
                else
                {
                    KotorVersion = "Unknown";
                }
            });

            if (!installInfo.Success || installInfo.Data == null)
                return;

            var info = installInfo.Data;

            // Update UI on UI thread
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (info.InstalledPatches.Count == 0)
                {
                    StatusMessage = "No patches currently installed";
                    return;
                }

                // Store installed patch IDs for later comparison
                InstalledPatchIds = info.InstalledPatches.ToHashSet();

                // Move installed patches to Active list in the correct order
                var installedIds = info.InstalledPatches.ToHashSet();

                // Collect all patches (from both Available and Active lists)
                var allPatches = AvailablePatches.Concat(ActivePatches).ToList();
                var patchesToMove = allPatches.Where(p => installedIds.Contains(p.Id)).ToList();

                // Clear both lists
                AvailablePatches.Clear();
                ActivePatches.Clear();

                // Add patches in the order they're installed
                foreach (var patchId in info.InstalledPatches)
                {
                    var patch = patchesToMove.FirstOrDefault(p => p.Id == patchId);
                    if (patch != null)
                    {
                        ActivePatches.Add(patch);
                    }
                    else
                    {
                        // Orphaned patch - create a placeholder
                        var orphanedPatch = new PatchItemViewModel
                        {
                            Id = patchId,
                            Name = $"{patchId} (not found)",
                            Version = "?",
                            Author = "Unknown",
                            Description = "This patch is installed but not found in patches directory",
                            IsOrphaned = true
                        };
                        ActivePatches.Add(orphanedPatch);
                    }
                }

                // Add remaining patches back to Available
                foreach (var patch in allPatches)
                {
                    if (!installedIds.Contains(patch.Id))
                    {
                        AvailablePatches.Add(patch);
                    }
                }

                HasActivePatches = ActivePatches.Count > 0;
                SaveActivePatches();
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
