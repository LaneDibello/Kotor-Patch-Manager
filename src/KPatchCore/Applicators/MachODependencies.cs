using KPatchCore.Models;
using KPatchCore.Parsers;
using LibObjectFile.MachO;

namespace KPatchCore.Applicators;

/// <summary>
/// The LC_LOAD_DYLIB list of a Mach-O executable. Adding to it costs only the space Apple's linker
/// already left after the load commands, so nothing in the image moves and addresses patched
/// beforehand stay valid.
/// </summary>
internal sealed class MachODependencies : IExecutableDependencies
{
    /// <summary>
    /// dyld resolves this against the directory holding the executable, which is where the manager
    /// stages the patcher. A bare name would be looked for on the system search paths instead. The
    /// game already names libBink2Macx64.dylib this way, so the prefix is known to work here.
    /// </summary>
    private const string LoaderRelativePrefix = "@executable_path/";

    private readonly string _exePath;

    public MachODependencies(string exePath) => _exePath = exePath;

    public PatchResult<bool> Contains(string moduleName)
    {
        if (!File.Exists(_exePath))
            return PatchResult<bool>.Fail($"Executable not found: {_exePath}");

        try
        {
            var image = Read();
            var targets = image.Patchable.ToList();
            if (targets.Count == 0)
                return PatchResult<bool>.Fail(NoPatchableSliceMessage());

            return PatchResult<bool>.Ok(targets.All(f => IsLinked(f, moduleName)));
        }
        catch (Exception ex)
        {
            return PatchResult<bool>.Fail($"Failed to read {Path.GetFileName(_exePath)}: {ex.Message}");
        }
    }

    public PatchResult Add(string moduleName)
    {
        if (string.IsNullOrWhiteSpace(moduleName))
            return PatchResult.Fail("Library name cannot be null or empty");
        if (!File.Exists(_exePath))
            return PatchResult.Fail($"Executable not found: {_exePath}");

        var qualified = Qualify(moduleName);
        var tempPath = _exePath + ".kpm-inject.tmp";

        try
        {
            var image = Read();
            var targets = image.Patchable.ToList();
            if (targets.Count == 0)
                return PatchResult.Fail(NoPatchableSliceMessage());

            if (targets.All(f => IsLinked(f, moduleName)))
                return PatchResult.Ok($"{qualified} is already a load command; nothing to do.");

            foreach (var file in targets)
            {
                if (IsLinked(file, moduleName))
                    continue;

                // A load command is 24 bytes plus the name, padded. Refusing here beats writing an
                // image whose commands run past the space the linker left for them.
                var needed = 24 + qualified.Length + 1;
                if (file.AvailableLoadCommandSpace < needed)
                    return PatchResult.Fail(
                        $"{Path.GetFileName(_exePath)} has {file.AvailableLoadCommandSpace} bytes of load " +
                        $"command space left, and naming {qualified} needs about {needed}.");

                file.AddLoadDylib(qualified);
            }

            image.ResignWhatWasSigned(MachOSigning.IdentifierFor(_exePath));

            using (var output = File.Create(tempPath))
                image.Write(output);

            // Swapped in only once it is written, so a failed write never leaves a broken game.
            File.Move(tempPath, _exePath, overwrite: true);
            return PatchResult.Ok($"Added {qualified} to {Path.GetFileName(_exePath)}.");
        }
        catch (Exception ex)
        {
            try { File.Delete(tempPath); } catch { /* the original is still intact */ }
            return PatchResult.Fail($"Failed to add {qualified} to {Path.GetFileName(_exePath)}: {ex.Message}");
        }
    }

    private string Qualify(string moduleName) =>
        moduleName.Contains('/') ? moduleName : LoaderRelativePrefix + moduleName;

    /// <summary>Matches whether the name is written bare or with a loader-relative prefix.</summary>
    private bool IsLinked(MachOFile file, string moduleName)
    {
        var bare = Path.GetFileName(moduleName);
        return file.LinkedLibraries.Any(l =>
            string.Equals(Path.GetFileName(l.Value), bare, StringComparison.Ordinal));
    }

    private string NoPatchableSliceMessage() =>
        $"{Path.GetFileName(_exePath)} has no x86_64 slice, which is the architecture the patcher is built for.";

    private Image Read()
    {
        using var stream = File.OpenRead(_exePath);
        if (MachOFatFile.IsFat(stream))
        {
            stream.Position = 0;
            return new Image(MachOFatFile.Read(stream), null);
        }

        stream.Position = 0;
        return new Image(null, MachOFile.Read(stream));
    }

    /// <summary>A Mach-O read from disk, universal or thin, so callers stop caring which.</summary>
    private sealed record Image(MachOFatFile? Fat, MachOFile? Thin)
    {
        /// <summary>
        /// The slices worth naming the patcher in. Only x86_64: it is what the engine is built for,
        /// and what every 64-bit Mac runs. Naming it in an i386 slice would leave that slice unable
        /// to start at all, since no i386 patcher exists to find.
        /// </summary>
        public IEnumerable<MachOFile> Patchable =>
            Fat is not null
                ? Fat.Slices.Where(s => s.File is not null && s.CpuType == MachOCpuType.X86_64).Select(s => s.File!)
                : Thin is { CpuType: MachOCpuType.X86_64 } ? new[] { Thin } : Array.Empty<MachOFile>();

        public void ResignWhatWasSigned(string identifier)
        {
            if (Fat is not null)
                MachOSigning.ResignWhatWasSigned(Fat, identifier);
            else
                MachOSigning.ResignWhatWasSigned(Thin!, identifier);
        }

        public void Write(Stream stream)
        {
            if (Fat is not null)
            {
                Fat.UpdateLayout();
                Fat.Write(stream);
            }
            else
            {
                Thin!.Write(stream);
            }
        }
    }
}
