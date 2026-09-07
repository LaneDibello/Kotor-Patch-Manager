using System;
using System.Windows.Input;

namespace KPatchLauncher.ViewModels;

/// <summary>
/// Simple ICommand implementation without reactive dependencies
/// </summary>
public class SimpleCommand : ICommand
{
    private readonly Action<object?> _execute;
    private readonly Func<bool>? _canExecute;

    public SimpleCommand(Action execute, Func<bool>? canExecute = null)
    {
        ArgumentNullException.ThrowIfNull(execute);
        _execute = _ => execute();
        _canExecute = canExecute;
    }

    /// <summary>
    /// For commands bound inside an item template, where the command needs to act on the row that
    /// was clicked rather than on the list's current selection.
    /// </summary>
    public SimpleCommand(Action<object?> execute, Func<bool>? canExecute = null)
    {
        ArgumentNullException.ThrowIfNull(execute);
        _execute = execute;
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter)
    {
        return _canExecute?.Invoke() ?? true;
    }

    public void Execute(object? parameter)
    {
        if (CanExecute(parameter))
        {
            _execute(parameter);
        }
    }

    public void RaiseCanExecuteChanged()
    {
        CanExecuteChanged?.Invoke(this, EventArgs.Empty);
    }
}
