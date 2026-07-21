using KPatchCore.Models;
using LibObjectFile.Elf;

namespace KPatchCore.Applicators;

/// <summary>
/// Injects a DT_NEEDED dependency on the native patcher module (KotorPatcher.so) into a
/// dynamically-linked ELF game executable, so the dynamic loader maps it at startup. This is the
/// native Linux counterpart to staging the KProxy DLL on Windows. The edit is address-preserving
/// (LibObjectFile relocates .dynamic/.dynstr and repurposes a PT_NOTE into a PT_LOAD), so existing
/// code, symbols and relocations stay valid.
/// </summary>
public static class ElfInjector
{
    /// <summary>
    /// Reports whether <paramref name="soName"/> is already a DT_NEEDED dependency of the ELF at
    /// <paramref name="elfPath"/>. Fails if the file is missing, unreadable, or not dynamically linked.
    /// </summary>
    public static PatchResult<bool> IsNeeded(string elfPath, string soName)
    {
        if (!File.Exists(elfPath))
            return PatchResult<bool>.Fail($"Executable not found: {elfPath}");

        try
        {
            ElfFile elf;
            using (var input = File.OpenRead(elfPath))
                elf = ElfFile.Read(input);

            var dynamic = elf.Sections.OfType<ElfDynamicLinkingTable>().FirstOrDefault();
            if (dynamic is null)
                return PatchResult<bool>.Fail($"{Path.GetFileName(elfPath)} is not dynamically linked (no .dynamic section).");

            return PatchResult<bool>.Ok(dynamic.GetNeededLibraries().Contains(soName));
        }
        catch (Exception ex)
        {
            return PatchResult<bool>.Fail($"Failed to read {Path.GetFileName(elfPath)}: {ex.Message}");
        }
    }

    /// <summary>
    /// Adds <paramref name="soName"/> to the DT_NEEDED list of the ELF at <paramref name="elfPath"/>.
    /// Idempotent: if the dependency is already present the file is left untouched. The rewrite goes
    /// through a sibling temp file that is then swapped in, so a failed write never leaves a corrupt
    /// executable on disk (the caller has also backed the exe up before this runs).
    /// </summary>
    public static PatchResult AddNeeded(string elfPath, string soName)
    {
        if (string.IsNullOrWhiteSpace(elfPath))
            return PatchResult.Fail("Executable path cannot be null or empty");
        if (string.IsNullOrWhiteSpace(soName))
            return PatchResult.Fail("Library name cannot be null or empty");
        if (!File.Exists(elfPath))
            return PatchResult.Fail($"Executable not found: {elfPath}");

        try
        {
            ElfFile elf;
            using (var input = File.OpenRead(elfPath))
                elf = ElfFile.Read(input);

            var dynamic = elf.Sections.OfType<ElfDynamicLinkingTable>().FirstOrDefault();
            if (dynamic is null)
                return PatchResult.Fail($"{Path.GetFileName(elfPath)} is not dynamically linked (no .dynamic section).");

            if (dynamic.GetNeededLibraries().Contains(soName))
                return PatchResult.Ok($"{soName} is already a DT_NEEDED dependency; nothing to do.");

            elf.AddNeededLibrary(soName);

            var tempPath = elfPath + ".kpm-inject.tmp";
            try
            {
                using (var output = File.Create(tempPath))
                    elf.Write(output);

                // The game executable is +x; carry its mode onto the temp file, or File.Move would
                // hand over its default (non-executable) permissions. The guard satisfies CA1416:
                // Get/SetUnixFileMode are Windows-unsupported, and this path only runs on Linux.
                if (!OperatingSystem.IsWindows())
                    File.SetUnixFileMode(tempPath, File.GetUnixFileMode(elfPath));

                File.Move(tempPath, elfPath, overwrite: true);
            }
            finally
            {
                if (File.Exists(tempPath))
                    File.Delete(tempPath);
            }

            return PatchResult.Ok($"Added {soName} to DT_NEEDED of {Path.GetFileName(elfPath)}.");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to inject {soName} into {Path.GetFileName(elfPath)}: {ex.Message}");
        }
    }
}
