using LibObjectFile.MachO;

namespace KPatchCore.Parsers;

/// <summary>
/// The one rule for putting a Mach-O's signature back after editing it, shared by everything that
/// writes one so the two cannot drift apart.
/// </summary>
/// <remarks>
/// Ad hoc is enough because the kernel checks a binary's own signature when it execs it, and an
/// ad-hoc one satisfies that. It does not satisfy the bundle seal: Contents/_CodeSignature/
/// CodeResources names the executable with a cdhash and a requirement pinning Aspyr's team
/// identifier, and both stop matching the moment we patch. Nothing on the launch path reads that
/// seal, restoring the backup makes it valid again, and repairing it would need Aspyr's
/// certificate, so it is left alone.
///
/// All of that holds only while the game ships with CodeDirectory flags of 0. Hardened runtime or
/// library validation would refuse to load an unsigned KotorPatcher.dylib whatever is signed here.
/// </remarks>
internal static class MachOSigning
{
    /// <summary>
    /// codesign uses the file name without its extension as the default identifier, and it is what
    /// these binaries already carry. The reader does not expose the existing one to copy.
    /// </summary>
    public static string IdentifierFor(string exePath) => Path.GetFileNameWithoutExtension(exePath);

    /// <summary>
    /// Re-signs ad hoc, and only where a signature already existed. A file that arrived unsigned
    /// stays unsigned: giving it one would change what the loader checks for no reason.
    /// </summary>
    public static void ResignWhatWasSigned(MachOFile file, string identifier)
    {
        if (file.CodeSignature is not null)
            file.AdHocSign(identifier);
    }

    /// <inheritdoc cref="ResignWhatWasSigned(MachOFile, string)"/>
    /// <remarks>Each slice of a universal binary carries its own signature.</remarks>
    public static void ResignWhatWasSigned(MachOFatFile fat, string identifier)
    {
        foreach (var slice in fat.Slices)
        {
            if (slice.File is not null)
                ResignWhatWasSigned(slice.File, identifier);
        }
    }
}
