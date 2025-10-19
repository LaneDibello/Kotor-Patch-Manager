using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Avalonia.Media;
using KPatchLauncher.ViewModels;

namespace KPatchLauncher.Converters;

public class PatchStateColorConverter : IValueConverter
{
    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is not PatchItemViewModel patch)
            return new SolidColorBrush(Color.Parse("#00AFFF")); // Default foreground

        // Orphaned patches are red (highest priority)
        if (patch.IsOrphaned)
            return new SolidColorBrush(Color.Parse("#FF0000"));

        // Installed patches are green
        if (patch.IsInstalled)
            return new SolidColorBrush(Color.Parse("#00FF00"));

        // Incompatible patches are orange
        if (patch.IsIncompatible)
            return new SolidColorBrush(Color.Parse("#FF8800"));

        // Default (pending) patches are default foreground
        return new SolidColorBrush(Color.Parse("#00AFFF"));
    }

    public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
