using KPatchCore.Models;

namespace KPatchLauncher.ViewModels;

/// <summary>
/// One entry of a choice option, as a radio button. Picking one unpicks the others.
/// </summary>
public class PatchChoiceViewModel : ViewModelBase
{
    private readonly PatchOptionViewModel _option;

    public PatchChoiceViewModel(PatchOptionViewModel option, PatchOptionChoice choice)
    {
        _option = option;
        Id = choice.Id;
        Name = choice.Name;
    }

    public string Id { get; }
    public string Name { get; }

    /// <summary>The option this is an entry of, for the entry's own bindings.</summary>
    public PatchOptionViewModel Option => _option;

    public bool IsSelected
    {
        get => _option.Value == Id;
        set
        {
            // Only picking does anything: a choice always holds one of its entries, so
            // unpicking the current one would leave it holding none.
            if (value)
            {
                _option.Value = Id;
            }
            else
            {
                OnPropertyChanged();
            }
        }
    }

    internal void NotifySelectionChanged() => OnPropertyChanged(nameof(IsSelected));
}

/// <summary>
/// One option of a patch, as the details panel shows it under the patch's description:
/// a tick box for a toggle, one radio button per entry for a choice, with a triangle that
/// drops down what the option does.
/// </summary>
/// <remarks>
/// Holds the value the player has picked and the value the game was last installed with.
/// A difference between the two is a pending change, like a tick that has not been applied.
/// </remarks>
public class PatchOptionViewModel : ViewModelBase
{
    private string _value;
    private string _installedValue;
    private bool _isAvailable;

    public PatchOptionViewModel(PatchItemViewModel patch, PatchOption option)
    {
        Patch = patch;
        Id = option.Id;
        Name = option.Name;
        Description = string.IsNullOrWhiteSpace(option.Description)
            ? "No description provided."
            : option.Description;
        IsToggle = option.Type == PatchOptionType.Toggle;
        Default = option.Default;
        _value = option.Default;
        _installedValue = option.Default;
        Choices = option.Choices.Select(c => new PatchChoiceViewModel(this, c)).ToList();
        ToggleDetailsCommand = new SimpleCommand(() => IsDetailsExpanded = !IsDetailsExpanded);
        DefaultText = IsToggle
            ? (option.Default == PatchOption.On ? "On" : "Off")
            : option.Choices.FirstOrDefault(c => c.Id == option.Default)?.Name ?? option.Default;
    }

    /// <summary>The patch this option belongs to.</summary>
    public PatchItemViewModel Patch { get; }

    public string Id { get; }
    public string Name { get; }
    public string Description { get; }
    public bool IsToggle { get; }
    public bool IsChoice => !IsToggle;
    public IReadOnlyList<PatchChoiceViewModel> Choices { get; }
    public string Default { get; }

    /// <summary>The default, as the details panel words it.</summary>
    public string DefaultText { get; }

    /// <summary>The value an install would use: "true"/"false" for a toggle, a choice id otherwise.</summary>
    public string Value
    {
        get => _value;
        set
        {
            if (SetProperty(ref _value, value))
            {
                OnPropertyChanged(nameof(IsOn));
                OnPropertyChanged(nameof(IsPending));
                foreach (var choice in Choices)
                {
                    choice.NotifySelectionChanged();
                }
                ValueChanged?.Invoke(this, EventArgs.Empty);
            }
        }
    }

    /// <summary>The value the game was last installed with; the default when it never was.</summary>
    public string InstalledValue
    {
        get => _installedValue;
        set
        {
            if (SetProperty(ref _installedValue, value))
            {
                OnPropertyChanged(nameof(IsPending));
                OnPropertyChanged(nameof(InstalledText));
                OnPropertyChanged(nameof(StatusText));
            }
        }
    }

    /// <summary>Whether the option can hold <paramref name="value"/>.</summary>
    public bool Accepts(string value) => IsToggle
        ? value is PatchOption.On or PatchOption.Off
        : Choices.Any(c => c.Id == value);

    /// <summary>Whether applying would change this option.</summary>
    public bool IsPending => Value != InstalledValue;

    /// <summary>What the option was installed with, as the details panel words it.</summary>
    public string InstalledText => IsToggle
        ? (InstalledValue == PatchOption.On ? "On" : "Off")
        : Choices.FirstOrDefault(c => c.Id == InstalledValue)?.Name ?? InstalledValue;

    /// <summary>The line under the option's description.</summary>
    public string StatusText => $"Installed: {InstalledText}. Default: {DefaultText}.";

    /// <summary>A toggle's state, for its tick box.</summary>
    public bool IsOn
    {
        get => Value == PatchOption.On;
        set => Value = value ? PatchOption.On : PatchOption.Off;
    }

    /// <summary>
    /// Whether the option can be changed: only while its patch is ticked, since the option
    /// of a patch that is not installed chooses nothing.
    /// </summary>
    public bool IsAvailable
    {
        get => _isAvailable;
        set => SetProperty(ref _isAvailable, value);
    }

    private bool _isDetailsExpanded;

    /// <summary>
    /// Whether the option's description is shown under it (the triangle before its name).
    /// </summary>
    public bool IsDetailsExpanded
    {
        get => _isDetailsExpanded;
        set
        {
            if (SetProperty(ref _isDetailsExpanded, value))
            {
                OnPropertyChanged(nameof(DetailsGlyph));
            }
        }
    }

    public string DetailsGlyph => IsDetailsExpanded ? "\u25BE" : "\u25B8";

    public System.Windows.Input.ICommand ToggleDetailsCommand { get; }

    public event EventHandler? ValueChanged;
}
