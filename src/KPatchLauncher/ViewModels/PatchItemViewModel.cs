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
        set
        {
            if (SetProperty(ref _isOrphaned, value))
            {
                OnPropertyChanged(nameof(StateColor));
            }
        }
    }

    public bool IsInstalled
    {
        get => _isInstalled;
        set
        {
            if (SetProperty(ref _isInstalled, value))
            {
                OnPropertyChanged(nameof(StateColor));
            }
        }
    }

    public bool IsIncompatible
    {
        get => _isIncompatible;
        set
        {
            if (SetProperty(ref _isIncompatible, value))
            {
                OnPropertyChanged(nameof(StateColor));
            }
        }
    }

    public string DisplayText => $"{Name} v{Version}";

    // Computed property for color - easier for binding to track
    public string StateColor
    {
        get
        {
            if (IsOrphaned) return "#FF0000";      // Red
            if (IsInstalled) return "#00FF00";     // Green
            if (IsIncompatible) return "#FF8800";  // Orange
            return "#00AFFF";                      // Blue (default)
        }
    }

    public void UpdateInstalledState(HashSet<string> installedIds)
    {
        IsInstalled = installedIds.Contains(Id);
    }
}
