using System.Collections.Generic;

namespace KPatchLauncher.ViewModels;

public class PatchItemViewModel : ViewModelBase
{
    private string _id = string.Empty;
    private string _name = string.Empty;
    private string _version = string.Empty;
    private string _author = string.Empty;
    private string _description = string.Empty;
    private bool _isOrphaned = false;
    private bool _isInstalled = false;
    private bool _isIncompatible = false;

    public string Id
    {
        get => _id;
        set => SetProperty(ref _id, value);
    }

    public string Name
    {
        get => _name;
        set
        {
            if (SetProperty(ref _name, value))
            {
                OnPropertyChanged(nameof(DisplayText));
            }
        }
    }

    public string Version
    {
        get => _version;
        set
        {
            if (SetProperty(ref _version, value))
            {
                OnPropertyChanged(nameof(DisplayText));
            }
        }
    }

    public string Author
    {
        get => _author;
        set => SetProperty(ref _author, value);
    }

    public string Description
    {
        get => _description;
        set => SetProperty(ref _description, value);
    }

    public bool IsOrphaned
    {
        get => _isOrphaned;
        set => SetProperty(ref _isOrphaned, value);
    }

    public bool IsInstalled
    {
        get => _isInstalled;
        set => SetProperty(ref _isInstalled, value);
    }

    public bool IsIncompatible
    {
        get => _isIncompatible;
        set => SetProperty(ref _isIncompatible, value);
    }

    public string DisplayText => $"{Name} v{Version}";

    public void UpdateInstalledState(HashSet<string> installedIds)
    {
        IsInstalled = installedIds.Contains(Id);
    }
}
