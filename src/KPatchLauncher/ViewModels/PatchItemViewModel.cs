namespace KPatchLauncher.ViewModels;

public class PatchItemViewModel : ViewModelBase
{
    private string _id = string.Empty;
    private string _name = string.Empty;
    private string _version = string.Empty;
    private string _author = string.Empty;
    private string _description = string.Empty;
    private bool _isChecked = false;
    private bool _isOrphaned = false;
    private int _displayOrder = 0;
    private bool _isCompatible = true;
    private string _compatibilityStatus = string.Empty;

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

    public bool IsChecked
    {
        get => _isChecked;
        set
        {
            if (SetProperty(ref _isChecked, value))
            {
                foreach (var option in Options)
                {
                    option.IsAvailable = value;
                }
                CheckedChanged?.Invoke(this, EventArgs.Empty);
            }
        }
    }

    public event EventHandler? CheckedChanged;

    public bool IsOrphaned
    {
        get => _isOrphaned;
        set => SetProperty(ref _isOrphaned, value);
    }

    public int DisplayOrder
    {
        get => _displayOrder;
        set => SetProperty(ref _displayOrder, value);
    }

    public bool IsCompatible
    {
        get => _isCompatible;
        set => SetProperty(ref _isCompatible, value);
    }

    public string CompatibilityStatus
    {
        get => _compatibilityStatus;
        set => SetProperty(ref _compatibilityStatus, value);
    }

    public string DisplayText => $"{Name} v{Version}";

    /// <summary>
    /// The patch's options, in the manifest's order. Empty for a patch without any.
    /// </summary>
    public List<PatchOptionViewModel> Options { get; } = new();

    public bool HasOptions => Options.Count > 0;

    /// <summary>
    /// Gives the patch its options. They start usable exactly when the patch is ticked.
    /// </summary>
    public void SetOptions(IEnumerable<KPatchCore.Models.PatchOption> options)
    {
        Options.Clear();
        foreach (var option in options)
        {
            Options.Add(new PatchOptionViewModel(this, option) { IsAvailable = IsChecked });
        }
        OnPropertyChanged(nameof(HasOptions));
    }

    /// <summary>
    /// The values an install would use for this patch, by option id.
    /// </summary>
    public Dictionary<string, string> OptionValues() => Options.ToDictionary(o => o.Id, o => o.Value);

    /// <summary>
    /// Whether applying would change one of this patch's options.
    /// </summary>
    public bool HasPendingOptions => Options.Any(o => o.IsPending);
}
