using KPatchCore.Models;

namespace KPatchLauncher.ViewModels;

/// <summary>
/// One entry of a choice option, as a row the player ticks. Ticking one unticks the others.
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

    /// <summary>The option this is an entry of, for the row's own bindings.</summary>
    public PatchOptionViewModel Option => _option;

    public bool IsSelected
    {
        get => _option.Value == Id;
        set
        {
            // Only ticking does anything: a choice always holds one of its entries, so
            // unticking the current one would leave it holding none.
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
/// One option of a patch, as a row under that patch in the list: a tick box for a toggle,
/// one tick per entry for a choice.
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
    private bool _isSelected;

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

    /// <summary>What kind of option this is, as the details panel words it.</summary>
    public string TypeText => IsToggle ? "On or off" : "One of several";

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
                OnPropertyChanged(nameof(ValueText));
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

    /// <summary>The picked value, as the details panel words it.</summary>
    public string ValueText => IsToggle
        ? (IsOn ? "On" : "Off")
        : Choices.FirstOrDefault(c => c.Id == Value)?.Name ?? Value;

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

    /// <summary>Whether this is the option the details panel is describing.</summary>
    public bool IsSelected
    {
        get => _isSelected;
        set => SetProperty(ref _isSelected, value);
    }

    public event EventHandler? ValueChanged;
}
