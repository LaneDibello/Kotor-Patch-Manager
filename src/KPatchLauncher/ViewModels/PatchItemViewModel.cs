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
        set => SetProperty(ref _isChecked, value);
    }

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

    public string DisplayText => $"{Name} v{Version}";
}
