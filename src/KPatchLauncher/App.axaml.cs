using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using KPatchCore.Models;
using KPatchLauncher.ViewModels;
using KPatchLauncher.Views;
using System;
using System.Linq;

namespace KPatchLauncher;

public partial class App : Application
{
    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            desktop.MainWindow = new MainWindow
            {
                DataContext = new MainViewModel(),
            };
        }

        base.OnFrameworkInitializationCompleted();
    }

    /// <summary>
    /// Dynamically switches the application theme based on game title
    /// </summary>
    /// <param name="gameTitle">The game title (KOTOR1, KOTOR2, or Unknown)</param>
    public void LoadTheme(GameTitle gameTitle)
    {
        // Determine which theme file to load
        var themeUri = gameTitle switch
        {
            GameTitle.KOTOR2 => new Uri("avares://KPatchLauncher/Themes/Kotor2Theme.axaml"),
            _ => new Uri("avares://KPatchLauncher/Themes/Kotor1Theme.axaml") // Default to KOTOR1 for Unknown
        };

        try
        {
            // Load the new theme resource dictionary
            var newTheme = (ResourceDictionary)AvaloniaXamlLoader.Load(themeUri);

            // Remove any existing theme dictionaries (from Themes folder)
            var existingThemes = Resources.MergedDictionaries
                .Where(dict => dict.TryGetResource("PrimaryBrush", null, out _))
                .ToList();

            foreach (var theme in existingThemes)
            {
                Resources.MergedDictionaries.Remove(theme);
            }

            // Add the new theme
            Resources.MergedDictionaries.Add(newTheme);
        }
        catch (Exception ex)
        {
            // If theme loading fails, silently continue with current theme
            System.Diagnostics.Debug.WriteLine($"Failed to load theme: {ex.Message}");
        }
    }
}
