using System;
using Avalonia.Controls;
using Avalonia.Platform;

namespace KPatchLauncher.Views;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        TrySetWindowIcon();
    }

    // Closing the window is the view's business, not the view model's, so this stays in
    // code-behind rather than becoming a command.
    private void OnExitClicked(object? sender, Avalonia.Interactivity.RoutedEventArgs e) => Close();

    private void TrySetWindowIcon()
    {
        try
        {
            using var stream = AssetLoader.Open(new Uri("avares://KPatchLauncher/Assets/icon.ico"));
            Icon = new WindowIcon(stream);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Failed to load window icon: {ex.Message}");
        }
    }
}
