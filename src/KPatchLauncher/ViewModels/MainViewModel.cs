using System;
using System.Collections.Generic;
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
using KPatchCore.Common;
using KPatchCore.Detectors;
using KPatchCore.Launcher;
using KPatchCore.Validators;
using KPatchLauncher.Models;

namespace KPatchLauncher.ViewModels;

public class MainViewModel : ViewModelBase
{
    private string _gamePath = string.Empty;
    private string _patchesPath = string.Empty;
    private string _statusMessage = "Ready";
    private string _kotorVersion = "Unknown";
    private GameVersion? _detectedGameVersion;
    private PatchItemViewModel? _selectedPatch;
    private PatchRepository? _repository;
    private readonly HashSet<string> _installedPatchIds = new(StringComparer.OrdinalIgnoreCase);
    private readonly AppSettings _settings;
    private bool _hasInstalledPatches;
    private bool _isOperationInProgress;
    private double _progressValue = 0;
    private bool? _selectAllPatches = false;
    private bool _isUpdatingSelectAllState;
    private bool _isBulkUpdatingPatchChecks;
    private int _patchStatusRequestVersion;
    private int _patchLoadRequestVersion;
    private bool _useCustomLaunch;
    private string _customLaunchCommand = string.Empty;

    public MainViewModel()
    {
        AllPatches = new ObservableCollection<PatchItemViewModel>();

        // Every structural change to AllPatches (load, orphan insert/remove, move-to-top,
        // manual reorder) has to reach the list control, so mirror them into VisiblePatches.
        // Compatibility changes are not collection changes, so UpdatePatchCompatibility()
        // syncs explicitly as well.
        AllPatches.CollectionChanged += (_, _) => SyncVisiblePatches();

        // Load settings. Pending patch selections are intentionally not restored at startup;
        // installed patch state is reloaded from the selected game's patch_config.toml.
        _settings = AppSettings.Load();
        _gamePath = _settings.GamePath;
        _patchesPath = _settings.PatchesPath;
        _useCustomLaunch = _settings.LaunchMethod == LaunchMethod.Custom;
        _customLaunchCommand = _settings.CustomLaunchCommand;
        DeploymentPolicy.PreferLibraryProxy = _settings.PreferLibraryProxy;
        ClearPersistedPatchSelection();

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

    /// <summary>
    /// The compatible subset of <see cref="AllPatches"/>, in the same order, as bound by the
    /// patch list. This is a single long-lived collection that is reconciled in place: the
    /// list control must never have its ItemsSource swapped for a differently ordered
    /// snapshot, which leaves recycled rows showing another patch's name and checkbox.
    /// </summary>
    public ObservableCollection<PatchItemViewModel> VisiblePatches { get; } = new();

    public bool HasVisiblePatches => VisiblePatches.Count > 0;

    public bool? SelectAllPatches
    {
        get => _selectAllPatches;
        set
        {
            if (_isUpdatingSelectAllState)
            {
                SetProperty(ref _selectAllPatches, value);
                return;
            }

            var requestedState = value == true;
            if (SetProperty(ref _selectAllPatches, (bool?)requestedState))
            {
                SetPatchSelectionFromSelectAll(requestedState);
            }
        }
    }

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
                OnPropertyChanged(nameof(CanChangeDeployment));
                ((SimpleCommand)UninstallAllCommand).RaiseCanExecuteChanged();
            }
        }
    }

    public bool IsOperationInProgress
    {
        get => _isOperationInProgress;
        private set => SetProperty(ref _isOperationInProgress, value);
    }

    public double ProgressValue
    {
        get => _progressValue;
        private set => SetProperty(ref _progressValue, value);
    }

    public string GamePath
    {
        get => _gamePath;
        set
        {
            // A macOS install is picked as a bundle or as the folder holding one, and neither is
            // the file to patch. Resolving here means everything downstream keeps seeing an
            // executable path, and the box shows what will actually be touched.
            value = PathHelpers.ResolveGameExecutable(value);

            if (SetProperty(ref _gamePath, value))
            {
                _settings.GamePath = value;
                UpdateGameBrowseDirectory(value);
                _settings.Save();

                InvalidatePatchStateForGamePathChange();

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
                UpdatePatchesBrowseDirectory(value);
                _settings.Save();

                ClearAllPatchSelections(clearInstalledState: true, removeOrphanedPatches: true);

                // Load patches from new directory
                if (!string.IsNullOrWhiteSpace(value))
                {
                    _ = LoadPatchesFromDirectoryAsync(value);
                }
                else
                {
                    _repository = null;
                    _patchLoadRequestVersion++;
                    ClearPatchViewModels();
                    SelectedPatch = null;
                    UpdateSelectAllState();
                    UpdatePendingChanges();
                }
            }
        }
    }

    /// <summary>
    /// Whether the launch-method controls apply. They matter only for a Windows build on a host
    /// that cannot run it, where a compatibility layer starts the game and the manager has no way
    /// to guess which. A native build goes through Steam, and a Windows host runs the executable
    /// directly, so in both cases there is nothing to ask.
    /// </summary>
    public bool ShowLaunchSettings => DeploymentPolicy.NeedsLaunchConfiguration(_detectedGameVersion);

    /// <summary>
    /// Whether the game is reached through the library proxy instead of injection. Only Windows
    /// can choose; Linux is on the proxy either way, which is why the menu item is disabled there.
    /// </summary>
    public bool PreferLibraryProxy
    {
        get => DeploymentPolicy.PreferLibraryProxy;
        set
        {
            if (DeploymentPolicy.PreferLibraryProxy == value)
            {
                return;
            }

            DeploymentPolicy.PreferLibraryProxy = value;
            _settings.PreferLibraryProxy = value;
            _settings.Save();
            OnPropertyChanged();
        }
    }

    /// <summary>
    /// Whether to offer the deployment choice at all. Hidden where there is nothing to choose:
    /// a host that cannot inject, or a game whose format settles the method itself.
    /// </summary>
    public bool ShowDeploymentOption => DeploymentPolicy.HasDeploymentChoice(_detectedGameVersion);

    /// <summary>
    /// Whether the deployment method can be changed right now. Switching it with patches installed
    /// would leave the game staged for the previous method, so it is fixed until they come out.
    /// This is a temporary state, unlike <see cref="ShowDeploymentOption"/>, so the control stays
    /// visible and goes disabled.
    /// </summary>
    public bool CanChangeDeployment => !HasInstalledPatches;

    /// <summary>
    /// When true, the game is launched with <see cref="CustomLaunchCommand"/>
    /// instead of through Steam. Applies to the proxy deployment method.
    /// </summary>
    public bool UseCustomLaunch
    {
        get => _useCustomLaunch;
        set
        {
            if (SetProperty(ref _useCustomLaunch, value))
            {
                _settings.LaunchMethod = value ? LaunchMethod.Custom : LaunchMethod.Steam;
                _settings.Save();
                OnPropertyChanged(nameof(CanLaunchGame));
                OnPropertyChanged(nameof(LaunchButtonTip));
            }
        }
    }

    /// <summary>
    /// Command used when <see cref="UseCustomLaunch"/> is set. "{exe}" is replaced
    /// with the game executable path.
    /// </summary>
    public string CustomLaunchCommand
    {
        get => _customLaunchCommand;
        set
        {
            if (SetProperty(ref _customLaunchCommand, value))
            {
                _settings.CustomLaunchCommand = value;
                _settings.Save();
                OnPropertyChanged(nameof(CanLaunchGame));
                OnPropertyChanged(nameof(LaunchButtonTip));
            }
        }
    }

    /// <summary>
    /// Whether pressing Launch can do anything. The configured method decides, not the platform:
    /// a Windows host starts the game itself, Steam is left available because Proton makes it a
    /// real option for a Windows build on Linux, and a custom command is only usable once one has
    /// been typed. That last case is the only one we can be certain about, so it is the only one
    /// that disables the button.
    /// </summary>
    public bool CanLaunchGame =>
        !ShowLaunchSettings
        || !UseCustomLaunch
        || !string.IsNullOrWhiteSpace(CustomLaunchCommand);

    /// <summary>What the Launch button says on hover, including why it is unavailable.</summary>
    public string LaunchButtonTip =>
        CanLaunchGame
            ? "Start the game with the installed patches."
            : "Enter a custom launch command above, or turn the option off to launch through Steam.";

    /// <summary>Raises the launch controls, all of which follow the detected game.</summary>
    private void NotifyLaunchControlsChanged()
    {
        OnPropertyChanged(nameof(ShowLaunchSettings));
        OnPropertyChanged(nameof(CanLaunchGame));
        OnPropertyChanged(nameof(LaunchButtonTip));
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
            var checkedPatches = AllPatches
                .Where(p => p.IsChecked && !p.IsOrphaned)
                .Select(p => p.Id)
                .OrderBy(x => x)
                .ToList();

            var installedPatches = AllPatches
                .Where(p => !p.IsOrphaned && IsInstalled(p.Id))
                .Select(p => p.Id)
                .OrderBy(x => x)
                .ToList();

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

    private bool IsInstalled(string patchId) => _installedPatchIds.Contains(patchId);

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
                SuggestedStartLocation = await TryGetSuggestedStartFolderAsync(window, GetGameBrowseStartDirectory()),
                FileTypeFilter = new[]
                {
                    new FilePickerFileType("Game Executables")
                    {
                        // swkotor.exe / swkotor2.exe on Windows and under Wine/Proton; the
                        // native Linux KOTOR II build is an extensionless "KOTOR2" ELF. On macOS
                        // the game lives inside a bundle, so both the bundle and the executable
                        // within it are offered: the panel presents an .app as one item, and
                        // whichever is chosen resolves to the same file.
                        Patterns = new[] { "*.exe", "KOTOR2", "*.app", "KOTOR_Exe", "KOTOR2sub" }
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
                AllowMultiple = false,
                SuggestedStartLocation = await TryGetSuggestedStartFolderAsync(window, GetPatchesBrowseStartDirectory())
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
            SetOperationInProgress(true, "Refreshing...");

            // Reload patches from directory if path is set
            if (!string.IsNullOrWhiteSpace(PatchesPath))
            {
                await LoadPatchesFromDirectoryAsync(PatchesPath);
            }

            SetOperationInProgress(false, "Refresh complete");
        }
        catch (Exception ex)
        {
            SetOperationInProgress(false, $"Error refreshing: {ex.Message}");
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

    private string? GetGameBrowseStartDirectory()
    {
        return GetDirectoryForExistingFile(GamePath)
            ?? GetExistingDirectory(_settings.LastGameBrowseDirectory);
    }

    private string? GetPatchesBrowseStartDirectory()
    {
        return GetExistingDirectory(PatchesPath)
            ?? GetExistingDirectory(_settings.LastPatchesBrowseDirectory);
    }

    private async Task<IStorageFolder?> TryGetSuggestedStartFolderAsync(Window window, string? directory)
    {
        if (string.IsNullOrWhiteSpace(directory))
            return null;

        try
        {
            return await window.StorageProvider.TryGetFolderFromPathAsync(directory);
        }
        catch
        {
            return null;
        }
    }

    private static string? GetDirectoryForExistingFile(string? filePath)
    {
        if (string.IsNullOrWhiteSpace(filePath) || !File.Exists(filePath))
            return null;

        return GetExistingDirectory(Path.GetDirectoryName(filePath));
    }

    private static string? GetExistingDirectory(string? directory)
    {
        return !string.IsNullOrWhiteSpace(directory) && Directory.Exists(directory)
            ? directory
            : null;
    }

    private void UpdateGameBrowseDirectory(string gamePath)
    {
        var directory = GetDirectoryForExistingFile(gamePath);
        if (directory != null)
        {
            _settings.LastGameBrowseDirectory = directory;
        }
    }

    private void UpdatePatchesBrowseDirectory(string patchesPath)
    {
        var directory = GetExistingDirectory(patchesPath);
        if (directory != null)
        {
            _settings.LastPatchesBrowseDirectory = directory;
        }
    }

    private void OnPatchCheckedChanged(object? sender, EventArgs e)
    {
        if (sender is PatchItemViewModel patch)
        {
            if (!_isBulkUpdatingPatchChecks && patch.IsChecked)
            {
                // Move to top of list
                var index = AllPatches.IndexOf(patch);
                if (index > 0)
                {
                    AllPatches.Move(index, 0);
                }
            }

            if (!_isBulkUpdatingPatchChecks)
            {
                SaveCheckedPatches();
                UpdatePendingChanges();
                UpdateSelectAllState();
            }
        }
    }

    private void SetPatchSelectionFromSelectAll(bool isChecked)
    {
        var targetPatches = isChecked
            ? VisiblePatches.Where(p => !p.IsOrphaned).ToList()
            : AllPatches.ToList();

        if (targetPatches.Count == 0)
        {
            UpdateSelectAllState();
            return;
        }

        _isBulkUpdatingPatchChecks = true;
        try
        {
            foreach (var patch in targetPatches)
            {
                patch.IsChecked = isChecked;
            }
        }
        finally
        {
            _isBulkUpdatingPatchChecks = false;
        }

        SaveCheckedPatches();
        UpdatePendingChanges();
        UpdateSelectAllState();
        StatusMessage = isChecked
            ? $"Selected {targetPatches.Count} visible patch{(targetPatches.Count == 1 ? "" : "es")}"
            : "Cleared all patch selections";
    }

    private void UpdateSelectAllState()
    {
        var visiblePatches = VisiblePatches.Where(p => !p.IsOrphaned).ToList();
        bool? newState;

        if (visiblePatches.Count == 0)
        {
            newState = false;
        }
        else
        {
            var checkedCount = visiblePatches.Count(p => p.IsChecked);
            newState = checkedCount == 0
                ? false
                : checkedCount == visiblePatches.Count
                    ? true
                    : null;
        }

        _isUpdatingSelectAllState = true;
        try
        {
            SelectAllPatches = newState;
        }
        finally
        {
            _isUpdatingSelectAllState = false;
        }

        OnPropertyChanged(nameof(HasVisiblePatches));
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

    private void InvalidatePatchStateForGamePathChange()
    {
        ClearAllPatchSelections(clearInstalledState: true, removeOrphanedPatches: true);
        _detectedGameVersion = null;
        KotorVersion = "Unknown";
        OnPropertyChanged(nameof(ShowDeploymentOption));
        NotifyLaunchControlsChanged();
        UpdatePatchCompatibility();
    }

    private void ClearAllPatchSelections(bool clearInstalledState, bool removeOrphanedPatches)
    {
        _patchStatusRequestVersion++;

        if (clearInstalledState)
        {
            _installedPatchIds.Clear();
            HasInstalledPatches = false;
        }

        SelectedPatch = null;

        if (removeOrphanedPatches)
        {
            RemoveOrphanedPatches();
        }

        _isBulkUpdatingPatchChecks = true;
        try
        {
            foreach (var patch in AllPatches)
            {
                patch.IsChecked = false;
            }
        }
        finally
        {
            _isBulkUpdatingPatchChecks = false;
        }

        ClearPersistedPatchSelection();
        UpdateSelectAllState();
        UpdatePendingChanges();
    }

    private void SyncPatchSelectionWithInstalledPatches(IEnumerable<string> installedPatchIds)
    {
        var normalizedInstalledIds = installedPatchIds
            .Where(id => !string.IsNullOrWhiteSpace(id))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        _installedPatchIds.Clear();
        foreach (var patchId in normalizedInstalledIds)
        {
            _installedPatchIds.Add(patchId);
        }

        HasInstalledPatches = _installedPatchIds.Count > 0;
        RemoveOrphanedPatches();

        _isBulkUpdatingPatchChecks = true;
        try
        {
            foreach (var patch in AllPatches)
            {
                patch.IsChecked = _installedPatchIds.Contains(patch.Id);
            }
        }
        finally
        {
            _isBulkUpdatingPatchChecks = false;
        }

        foreach (var patchId in normalizedInstalledIds)
        {
            if (AllPatches.Any(p => string.Equals(p.Id, patchId, StringComparison.OrdinalIgnoreCase)))
                continue;

            var orphanedPatch = new PatchItemViewModel
            {
                Id = patchId,
                Name = $"{patchId} (not found)",
                Version = "?",
                Author = "Unknown",
                Description = "This patch is installed but not found in patches directory",
                IsOrphaned = true,
                IsChecked = true,
                IsCompatible = false,
                CompatibilityStatus = "Patch files not found"
            };
            orphanedPatch.CheckedChanged += OnPatchCheckedChanged;
            AllPatches.Insert(0, orphanedPatch);
        }

        SaveCheckedPatches();
        UpdateSelectAllState();
        UpdatePendingChanges();
    }

    private void RemoveOrphanedPatches()
    {
        for (var i = AllPatches.Count - 1; i >= 0; i--)
        {
            if (!AllPatches[i].IsOrphaned)
                continue;

            AllPatches[i].CheckedChanged -= OnPatchCheckedChanged;
            AllPatches.RemoveAt(i);
        }
    }

    /// <summary>
    /// Drops every patch view model, detaching the check handlers first so discarded
    /// view models cannot keep driving this one.
    /// </summary>
    private void ClearPatchViewModels()
    {
        foreach (var patch in AllPatches)
        {
            patch.CheckedChanged -= OnPatchCheckedChanged;
        }

        AllPatches.Clear();
    }

    /// <summary>
    /// Reconciles <see cref="VisiblePatches"/> with the compatible subset of
    /// <see cref="AllPatches"/> using in-place inserts, moves and removes. Rebuilding the
    /// collection with Clear/Add instead would push a Reset at the list control and cost
    /// row identity, so a reorder could leave a row rendering the previous patch.
    /// </summary>
    private void SyncVisiblePatches()
    {
        var desired = AllPatches.Where(p => p.IsCompatible).ToList();
        var desiredSet = new HashSet<PatchItemViewModel>(desired);

        for (var i = VisiblePatches.Count - 1; i >= 0; i--)
        {
            if (!desiredSet.Contains(VisiblePatches[i]))
            {
                VisiblePatches.RemoveAt(i);
            }
        }

        // Everything left is also in desired, so each remaining item is at or after its
        // target index and this walk places them all without disturbing settled rows.
        for (var i = 0; i < desired.Count; i++)
        {
            var patch = desired[i];
            var current = VisiblePatches.IndexOf(patch);

            if (current < 0)
            {
                VisiblePatches.Insert(i, patch);
            }
            else if (current != i)
            {
                VisiblePatches.Move(current, i);
            }
        }

        OnPropertyChanged(nameof(HasVisiblePatches));
    }

    private void ClearPersistedPatchSelection()
    {
        if (_settings.CheckedPatchIds.Count == 0)
            return;

        _settings.CheckedPatchIds.Clear();
        _settings.Save();
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
    }

    private void SetOperationInProgress(bool inProgress, string? message = null, bool isAutoRefresh = false)
    {
        IsOperationInProgress = inProgress;
        ProgressValue = inProgress ? 100 : 0;

        if (message != null && !isAutoRefresh)
        {
            StatusMessage = message;
        }

        if (!inProgress)
        {
            UpdatePendingChanges();
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
            var checkedPatches = AllPatches.Where(p => p.IsChecked && !p.IsOrphaned && p.IsCompatible).ToList();

            // If no patches are checked, uninstall all
            if (checkedPatches.Count == 0)
            {
                SetOperationInProgress(true, "Uninstalling all patches...");

                var uninstallResult = await Task.Run(() =>
                    PatchRemover.RemoveAllPatches(GamePath));

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (uninstallResult.Success)
                    {
                        SyncPatchSelectionWithInstalledPatches(Array.Empty<string>());
                        SetOperationInProgress(false, "All patches uninstalled successfully");
                    }
                    else
                    {
                        SetOperationInProgress(false, $"Error: {uninstallResult.Error}");
                    }
                });
                return;
            }

            // Otherwise, uninstall existing patches and install checked ones
            SetOperationInProgress(true, "Applying patches...");

            // First, uninstall any existing patches, but preserve KPM's managed
            // identity file so a statically-patched EXE can still be recognized
            // during the reinstall that follows.
            await Task.Run(() => PatchRemover.RemoveAllPatches(GamePath, removeManagedState: false));

            // The patcher modules and the KProxy all ship next to the launcher. Which one gets
            // staged is the deployment method's call, so only the directory is passed.
            // AppContext.BaseDirectory works reliably with both regular and single-file builds.
            var applicator = new PatchApplicator(_repository);
            var options = new PatchApplicator.InstallOptions
            {
                GameExePath = GamePath,
                PatchIds = checkedPatches.Select(p => p.Id).ToList(),
                CreateBackup = true,
                PatcherDirectory = AppContext.BaseDirectory
            };

            // Run on background thread
            var result = await Task.Run(() => applicator.InstallPatches(options));

            // Update UI on UI thread
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (result.Success)
                {
                    SetOperationInProgress(false, $"Patches applied successfully ({result.InstalledPatches.Count} patches)");
                }
                else
                {
                    SetOperationInProgress(false, $"Error: {result.Error}");
                }
            });

            // Refresh installed status
            await CheckPatchStatusAsync(GamePath);
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                SetOperationInProgress(false, $"Error applying patches: {ex.Message}");
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
            SetOperationInProgress(true, "Uninstalling all patches...");

            // Uncheck all patches
            _isBulkUpdatingPatchChecks = true;
            try
            {
                foreach (var patch in AllPatches)
                {
                    patch.IsChecked = false;
                }
            }
            finally
            {
                _isBulkUpdatingPatchChecks = false;
            }

            SaveCheckedPatches();
            UpdatePendingChanges();
            UpdateSelectAllState();

            // Now apply (which will uninstall since nothing is checked)
            await ApplyPatches();
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                SetOperationInProgress(false, $"Error: {ex.Message}");
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
            SetOperationInProgress(true, "Launching game...");

            // A game with no launch controls showing is started through Steam whatever the
            // saved preference says. The box is hidden for it, so a command left behind by
            // another install must not decide how this one starts.
            var useCustom = ShowLaunchSettings && _useCustomLaunch;
            var launchConfig = new LaunchConfig
            {
                Method = useCustom ? LaunchMethod.Custom : LaunchMethod.Steam,
                CustomCommand = _customLaunchCommand
            };

            // Use KPatchCore's game launcher (handles patch detection and injection automatically)
            var result = await Task.Run(() =>
            {
                var orchestrator = new PatchOrchestrator(_patchesPath ?? string.Empty);
                return orchestrator.LaunchGame(GamePath, commandLineArgs: null, launchConfig);
            });

            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (result.Success)
                {
                    // The launcher's own message says how the game was started and how the
                    // patches got in, which differs per deployment method. A launch the manager
                    // started itself also has a pid; one handed to Steam or a custom command does
                    // not, since the manager never owns that process.
                    var detail = result.Messages.FirstOrDefault()
                        ?? (result.VanillaLaunch ? "Game launched, no patches" : "Game launched with patches");
                    var pid = result.ProcessId.HasValue ? $" (PID: {result.ProcessId})" : string.Empty;
                    SetOperationInProgress(false, detail + pid);
                }
                else
                {
                    SetOperationInProgress(false, $"Error: {result.Error}");
                }
            });
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                SetOperationInProgress(false, $"Error launching game: {ex.Message}");
            });
        }
    }

    private async Task LoadPatchesFromDirectoryAsync(string directory)
    {
        // Loads overlap whenever Refresh is clicked while one is running, or the startup
        // load races a directory change. Only the newest load may touch AllPatches;
        // otherwise each one appends its own copy of every patch.
        var requestVersion = ++_patchLoadRequestVersion;

        try
        {
            SetOperationInProgress(true, "Loading patches...");

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
                if (requestVersion != _patchLoadRequestVersion)
                    return;

                if (!scanResult.Success)
                {
                    SetOperationInProgress(false, $"Error loading patches: {scanResult.Error}");
                    return;
                }

                _repository = repository;

                if (allPatches == null)
                {
                    SetOperationInProgress(false, "Error: No patches found");
                    return;
                }

                // Swap the old view models out here rather than before the scan, so a load
                // that gets superseded never empties the list it is no longer filling.
                ClearPatchViewModels();

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
                    patch.CheckedChanged += OnPatchCheckedChanged;
                    AllPatches.Add(patch);
                }

                // Update compatibility status for loaded patches
                UpdatePatchCompatibility();
                UpdateSelectAllState();

                SetOperationInProgress(false, $"Loaded {patchViewModels.Count} patches from {Path.GetFileName(directory)}");
            });

            // Check patch status if we have a game path set
            if (requestVersion == _patchLoadRequestVersion
                && !string.IsNullOrWhiteSpace(GamePath)
                && File.Exists(GamePath))
            {
                await CheckPatchStatusAsync(GamePath);
            }
        }
        catch (Exception ex)
        {
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (requestVersion != _patchLoadRequestVersion)
                    return;

                SetOperationInProgress(false, $"Error loading patches: {ex.Message}");
            });
        }
    }

    private async Task CheckPatchStatusAsync(string gameExePath, bool isAutoRefresh = true)
    {
        if (_repository == null)
            return;

        var requestVersion = ++_patchStatusRequestVersion;

        try
        {
            SetOperationInProgress(true, "Checking patch status...", isAutoRefresh);

            // Get installation info and game version on background thread.
            var (installInfo, versionInfo) = await Task.Run(() =>
            {
                var install = PatchRemover.GetInstallationInfo(gameExePath);
                var version = GameDetector.DetectVersion(
                    gameExePath,
                    allowManagedInstallState: true);
                return (install, version);
            });

            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!IsPatchStatusRequestCurrent(requestVersion, gameExePath))
                    return;

                ApplyDetectedGameVersion(versionInfo);
                UpdatePatchCompatibility();

                if (!installInfo.Success || installInfo.Data == null)
                {
                    SyncPatchSelectionWithInstalledPatches(Array.Empty<string>());
                    SetOperationInProgress(false, isAutoRefresh ? null : "No patches detected", isAutoRefresh);
                    return;
                }

                var info = installInfo.Data;
                SyncPatchSelectionWithInstalledPatches(info.InstalledPatches);

                var installedCount = _installedPatchIds.Count;
                SetOperationInProgress(
                    false,
                    isAutoRefresh
                        ? null
                        : installedCount == 0
                            ? "No patches currently installed"
                            : $"Found {installedCount} installed patch{(installedCount == 1 ? "" : "es")}",
                    isAutoRefresh);
            });
        }
        catch (Exception ex)
        {
            // Silent failure - not critical
            await Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!IsPatchStatusRequestCurrent(requestVersion, gameExePath))
                    return;

                SyncPatchSelectionWithInstalledPatches(Array.Empty<string>());
                SetOperationInProgress(false, isAutoRefresh ? null : $"Could not check patch status: {ex.Message}", isAutoRefresh);
            });
        }
    }

    private bool IsPatchStatusRequestCurrent(int requestVersion, string gameExePath)
    {
        return requestVersion == _patchStatusRequestVersion
            && PathsEqual(GamePath, gameExePath);
    }

    private static bool PathsEqual(string? left, string? right)
    {
        if (string.IsNullOrWhiteSpace(left) || string.IsNullOrWhiteSpace(right))
            return false;

        try
        {
            return string.Equals(
                Path.GetFullPath(left),
                Path.GetFullPath(right),
                StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return string.Equals(left, right, StringComparison.OrdinalIgnoreCase);
        }
    }

    private void ApplyDetectedGameVersion(PatchResult<GameVersion> versionInfo)
    {
        if (versionInfo.Success && versionInfo.Data != null)
        {
            var v = versionInfo.Data;
            _detectedGameVersion = v;
            KotorVersion = v.DisplayName;
            OnPropertyChanged(nameof(ShowDeploymentOption));
            NotifyLaunchControlsChanged();

            // Switch theme based on detected game title
            if (Application.Current is App app)
            {
                app.LoadTheme(v.Title);
            }
        }
        else
        {
            ApplyUnknownGameVersion();
        }
    }

    private void ApplyUnknownGameVersion()
    {
        _detectedGameVersion = null;
        KotorVersion = "Unknown";
        OnPropertyChanged(nameof(ShowDeploymentOption));
        NotifyLaunchControlsChanged();

        // Load default theme (KOTOR 1) for unknown games
        if (Application.Current is App app)
        {
            app.LoadTheme(KPatchCore.Models.GameTitle.KOTOR1);
        }
    }

    private void UpdatePatchCompatibility()
    {
        if (_repository == null)
            return;

        // Get all patch entries from repository
        var allPatchEntries = _repository.GetAllPatches();

        foreach (var patchViewModel in AllPatches)
        {
            // Skip orphaned patches - they can't be checked for compatibility
            if (patchViewModel.IsOrphaned)
            {
                patchViewModel.IsCompatible = false;
                patchViewModel.CompatibilityStatus = "Patch files not found";
                continue;
            }

            // If game version is unknown, show all patches as compatible
            if (_detectedGameVersion == null || _detectedGameVersion.Version == "Unknown")
            {
                patchViewModel.IsCompatible = true;
                patchViewModel.CompatibilityStatus = "Unknown game version - compatibility not verified";
                continue;
            }

            // Find the patch entry in the repository
            if (allPatchEntries.TryGetValue(patchViewModel.Id, out var patchEntry))
            {
                // Use GameVersionValidator to check compatibility
                var validationResult = GameVersionValidator.ValidateGameVersion(
                    patchEntry.Manifest,
                    _detectedGameVersion
                );

                patchViewModel.IsCompatible = validationResult.Success;
                patchViewModel.CompatibilityStatus = validationResult.Success
                    ? $"Compatible with {_detectedGameVersion.DisplayName}"
                    : $"Incompatible: {validationResult.Error}";
            }
            else
            {
                // Shouldn't happen, but handle it gracefully
                patchViewModel.IsCompatible = false;
                patchViewModel.CompatibilityStatus = "Patch not found in repository";
            }
        }

        // IsCompatible changes are not collection changes, so mirror them across by hand
        SyncVisiblePatches();
        UpdateSelectAllState();
    }
}
