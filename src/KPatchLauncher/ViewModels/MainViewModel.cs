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
using KPatchLauncher.Models;

namespace KPatchLauncher.ViewModels;

public class MainViewModel : ViewModelBase
{
    private string _gamePath = string.Empty;
    private string _patchesPath = string.Empty;
    private string _statusMessage = "Ready";
    private PatchItemViewModel? _selectedAvailablePatch;
    private PatchItemViewModel? _selectedActivePatch;
    private PatchRepository? _repository;
    private readonly AppSettings _settings;
    private bool _hasActivePatches;

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
        AddPatchCommand = new SimpleCommand(() => AddPatch());
        RemovePatchCommand = new SimpleCommand(() => RemovePatch());
        MoveUpCommand = new SimpleCommand(() => MoveUp());
        MoveDownCommand = new SimpleCommand(() => MoveDown());
        ApplyPatchesCommand = new SimpleCommand(async () => await ApplyPatches());
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
    public ICommand AddPatchCommand { get; }
    public ICommand RemovePatchCommand { get; }
    public ICommand MoveUpCommand { get; }
    public ICommand MoveDownCommand { get; }
    public ICommand ApplyPatchesCommand { get; }
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

    private void RemovePatch()
    {
        if (SelectedActivePatch == null)
            return;

        var patch = SelectedActivePatch;
        AvailablePatches.Add(patch);
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

        var index = ActivePatches.IndexOf(SelectedActivePatch);
        if (index > 0)
        {
            ActivePatches.Move(index, index - 1);
            StatusMessage = $"Moved {SelectedActivePatch.Name} up";
            SaveActivePatches();
        }
    }

    private void MoveDown()
    {
        if (SelectedActivePatch == null)
            return;

        var index = ActivePatches.IndexOf(SelectedActivePatch);
        if (index < ActivePatches.Count - 1)
        {
            ActivePatches.Move(index, index + 1);
            StatusMessage = $"Moved {SelectedActivePatch.Name} down";
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

        if (ActivePatches.Count == 0)
        {
            StatusMessage = "Error: No patches selected";
            return;
        }

        if (_repository == null)
        {
            StatusMessage = "Error: No patches loaded";
            return;
        }

        try
        {
            StatusMessage = "Applying patches...";

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
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                StatusMessage = $"Error loading patches: {ex.Message}";
            });
        }
    }
}
