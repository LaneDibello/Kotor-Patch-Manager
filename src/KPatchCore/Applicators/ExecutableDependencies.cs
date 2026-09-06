using KPatchCore.Models;
using KPatchCore.Parsers;

namespace KPatchCore.Applicators;

/// <summary>
/// The list of libraries an executable asks its dynamic loader to map at startup, and the ability
/// to add to it. Naming the patcher there is what makes it load with no injector and no proxy,
/// whoever starts the game.
/// </summary>
/// <remarks>
/// There is deliberately no removal. Uninstall restores the executable from the backup taken
/// before it was touched, which undoes the edit byte for byte. A separate strip would be a second,
/// lossier undo of the same thing.
/// </remarks>
public interface IExecutableDependencies
{
    /// <summary>Whether <paramref name="moduleName"/> is already in the executable's list.</summary>
    PatchResult<bool> Contains(string moduleName);

    /// <summary>
    /// Adds <paramref name="moduleName"/> to the list. Idempotent: a module already present leaves
    /// the file untouched. The edit preserves every existing address, so code patched beforehand
    /// stays valid.
    /// </summary>
    PatchResult Add(string moduleName);
}

/// <summary>
/// Opens an executable's dependency list using the right mechanism for its format. This is the one
/// place that choice is made, so callers stay format-agnostic.
/// </summary>
public static class ExecutableDependencies
{
    public static PatchResult<IExecutableDependencies> Open(string exePath)
    {
        var format = ExecutableFormatDetector.Detect(exePath);
        if (!format.Success)
            return PatchResult<IExecutableDependencies>.Fail(format.Error!);

        return format.Data switch
        {
            ExecutableFormat.Elf => PatchResult<IExecutableDependencies>.Ok(new ElfDependencies(exePath)),
            ExecutableFormat.MachO => PatchResult<IExecutableDependencies>.Ok(new MachODependencies(exePath)),

            // A PE's import table cannot be grown without moving what follows it, and a packed
            // executable would not honour the edit anyway. Windows reaches the same end through
            // DeploymentMethod.LibraryProxy instead.
            _ => PatchResult<IExecutableDependencies>.Fail(
                $"{Path.GetFileName(exePath)} is a {format.Data} executable, which does not support " +
                $"adding a dependency."),
        };
    }
}
