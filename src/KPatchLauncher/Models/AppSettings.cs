using System.Text.Json;
using KPatchCore.Launcher;

namespace KPatchLauncher.Models;

/// <summary>
/// Application settings for persistence
/// </summary>
public class AppSettings
{
    /// <summary>
    /// Last selected game executable path
    /// </summary>
    public string GamePath { get; set; } = string.Empty;

    /// <summary>
    /// Last selected patches directory
    /// </summary>
    public string PatchesPath { get; set; } = string.Empty;

    /// <summary>
    /// Last directory used by the game executable picker. Kept separate from the patches picker.
    /// </summary>
    public string LastGameBrowseDirectory { get; set; } = string.Empty;

    /// <summary>
    /// Last directory used by the patches directory picker. Kept separate from the game picker.
    /// </summary>
    public string LastPatchesBrowseDirectory { get; set; } = string.Empty;

    /// <summary>
    /// List of checked patch IDs
    /// </summary>
    public List<string> CheckedPatchIds { get; set; } = new();

    /// <summary>
    /// How to start the game when patches are deployed via proxy (Steam or a
    /// custom command). Unused by the injection method.
    /// </summary>
    public LaunchMethod LaunchMethod { get; set; } = LaunchMethod.Steam;

    /// <summary>
    /// Custom launch command used when <see cref="LaunchMethod"/> is Custom.
    /// The token "{exe}" is replaced with the game executable path.
    /// </summary>
    public string CustomLaunchCommand { get; set; } = string.Empty;

    /// <summary>
    /// Legacy property for backwards compatibility (TODO: Remove after migration)
    /// </summary>
    [Obsolete("Use CheckedPatchIds instead")]
    public List<string>? ActivePatchIds
    {
        get => null;
        set
        {
            if (value != null)
            {
                CheckedPatchIds = value;
            }
        }
    }

    /// <summary>
    /// Path to settings file
    /// </summary>
    private static string SettingsFilePath =>
        Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "KPatchLauncher",
            "settings.json"
        );

    /// <summary>
    /// Loads settings from disk (or returns defaults if file doesn't exist)
    /// </summary>
    public static AppSettings Load()
    {
        try
        {
            if (!File.Exists(SettingsFilePath))
            {
                return new AppSettings();
            }

            var json = File.ReadAllText(SettingsFilePath);
            return JsonSerializer.Deserialize<AppSettings>(json) ?? new AppSettings();
        }
        catch
        {
            // If anything goes wrong, return defaults
            return new AppSettings();
        }
    }

    /// <summary>
    /// Saves settings to disk
    /// </summary>
    public void Save()
    {
        try
        {
            var directory = Path.GetDirectoryName(SettingsFilePath);
            if (directory != null && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            var json = JsonSerializer.Serialize(this, new JsonSerializerOptions
            {
                WriteIndented = true
            });

            File.WriteAllText(SettingsFilePath, json);
        }
        catch
        {
            // Silently fail - settings are not critical
        }
    }
}
