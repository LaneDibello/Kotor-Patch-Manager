using LibObjectFile.MachO;

// Ad hoc signs a Mach-O binary in place, from any host OS.
//
// The .NET SDK signs the macOS apphost only when it is running on macOS; a
// cross-publish from Linux or Windows leaves it unsigned. An unsigned arm64 image
// is not merely untrusted on Apple Silicon, it will not execute at all, so the
// macOS release cannot be built off a Mac without this step. Intel does not need
// it, but signing both keeps the two builds identical apart from the slice.
//
// The signature carries no certificate: it states only that the image hashes to
// what its code directory says, which is what the kernel wants before it will
// give the process an identity. It is the same thing `codesign -s -` produces,
// and the same thing KPatchCore puts back after it edits a game binary
// (see MachOSigning.cs) -- one library does both, so they cannot drift apart.

if (args.Length is 0 or > 2)
{
    Console.Error.WriteLine("Usage: MachOAdHocSign <macho-file> [identifier]");
    Console.Error.WriteLine();
    Console.Error.WriteLine("  Ad hoc signs the file in place. The identifier defaults to the file");
    Console.Error.WriteLine("  name without its extension, which is what codesign uses.");
    return 1;
}

var path = args[0];
// codesign's own default, and what the binaries we sign already carry.
var identifier = args.Length > 1 ? args[1] : Path.GetFileNameWithoutExtension(path);

if (!File.Exists(path))
{
    Console.Error.WriteLine($"ERROR: no such file: {path}");
    return 1;
}

try
{
    // Read fully into memory first: the signature covers the whole image up to
    // itself, so the file is rewritten from the start and cannot be edited in place.
    using var input = new MemoryStream(File.ReadAllBytes(path));
    using var output = new MemoryStream();

    if (MachOFatFile.IsFat(input))
    {
        var fat = MachOFatFile.Read(input);
        // Each slice carries its own signature; a universal binary is only as
        // runnable as the slice the machine picks.
        foreach (var slice in fat.Slices)
        {
            slice.File?.AdHocSign(identifier);
        }
        fat.UpdateLayout();
        fat.Write(output);
    }
    else
    {
        var file = MachOFile.Read(input);
        file.AdHocSign(identifier);
        file.Write(output);
    }

    // Written only once the whole image is laid out, so a failure part way through
    // leaves the original where it was rather than a half-signed binary.
    File.WriteAllBytes(path, output.ToArray());
}
catch (Exception ex)
{
    Console.Error.WriteLine($"ERROR: could not sign {path}: {ex.Message}");
    return 1;
}

Console.WriteLine($"  [OK] ad hoc signed {Path.GetFileName(path)} as '{identifier}'");
return 0;
