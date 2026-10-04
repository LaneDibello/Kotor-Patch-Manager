using System.Text;
using KPatchCore.Models;

namespace KPatchCore.Applicators;

/// <summary>
/// Records the option values the installed patches were applied with, in each patch's
/// INI file in the configs folder.
/// </summary>
/// <remarks>
/// The configs folder in the game directory holds one INI file per patch, named after
/// the patch's id, for all of that patch's settings. This manager owns one section of
/// it, [Patch Options], with a key per option id. A toggle is 1 or 0 and a choice is its
/// id, the forms the game's own INI settings take, so a patch's code and a script read
/// them with the usual INI functions. Everything else in the file is the patch's and is
/// left as it is.
/// The section is rewritten on every apply, and nothing reads it to decide which hooks
/// to install: those were chosen when the patches were applied.
/// </remarks>
public static class PatchOptionsIni
{
    public const string DirectoryName = "configs";
    public const string Section = "Patch Options";

    // A patch's file is the patch's: its other sections may hold text in whatever code
    // page the game or the patch wrote. Latin-1 maps every byte to one character and
    // back, so that text comes through a rewrite unchanged. The section itself is ASCII.
    private static readonly Encoding FileEncoding = Encoding.Latin1;

    /// <summary>The file that holds a patch's settings, inside the folder.</summary>
    public static string FileNameFor(string patchId) => patchId + ".ini";

    /// <summary>The spelling the section uses for an option value.</summary>
    public static string ToIniValue(string value) => value switch
    {
        PatchOption.On => "1",
        PatchOption.Off => "0",
        _ => value
    };

    /// <summary>
    /// The value an entry of the section stands for. Only a toggle is translated: a
    /// choice's id may itself be "0" or "1".
    /// </summary>
    public static string FromIniValue(bool isToggle, string value) => isToggle
        ? value switch
        {
            "1" => PatchOption.On,
            "0" => PatchOption.Off,
            _ => value
        }
        : value;

    /// <summary>
    /// A patch's section, or an empty string for a patch without options.
    /// </summary>
    public static string Generate(PatchConfig.EnabledPatch patch)
    {
        if (patch.Options.Count == 0)
        {
            return string.Empty;
        }

        var text = new StringBuilder();
        text.Append('[').Append(Section).Append("]\r\n");
        foreach (var (optionId, value) in patch.Options)
        {
            text.Append(optionId).Append('=').Append(ToIniValue(value)).Append("\r\n");
        }

        return text.ToString();
    }

    /// <summary>
    /// Writes the section into the file of each patch that has options, and takes it out
    /// of every other file in the folder: a patch that is no longer installed has no
    /// applied values. The rest of each file is kept.
    /// </summary>
    public static PatchResult WriteFiles(PatchConfig config, string gameDir)
    {
        try
        {
            var withOptions = config.Patches.Where(p => p.Options.Count > 0).ToList();
            var badId = withOptions.FirstOrDefault(p =>
                p.Id.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || p.Id is "." or "..");
            if (badId != null)
            {
                return PatchResult.Fail($"Patch id '{badId.Id}' cannot name a file in {DirectoryName}");
            }

            var directory = Path.Combine(gameDir, DirectoryName);
            var keep = withOptions
                .Select(p => FileNameFor(p.Id))
                .ToHashSet(StringComparer.OrdinalIgnoreCase);
            RemoveSections(gameDir, keep);

            if (withOptions.Count == 0)
            {
                return PatchResult.Ok("No patch options to record");
            }

            Directory.CreateDirectory(directory);
            foreach (var patch in withOptions)
            {
                var path = Path.Combine(directory, FileNameFor(patch.Id));
                var rest = File.Exists(path) ? WithoutSection(File.ReadAllText(path, FileEncoding)) : string.Empty;
                File.WriteAllText(path, Generate(patch) + (rest.Length > 0 ? "\r\n" + rest : string.Empty), FileEncoding);
            }

            return PatchResult.Ok($"Options recorded: {directory}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to write {DirectoryName}: {ex.Message}");
        }
    }

    /// <summary>
    /// Takes the section out of the folder's files, except those named in
    /// <paramref name="keep"/>. A file that held nothing else is deleted, and so is the
    /// folder once it is empty. Returns the files deleted, relative to the game directory.
    /// </summary>
    public static List<string> RemoveSections(string gameDir, ISet<string>? keep = null)
    {
        var removed = new List<string>();
        var directory = Path.Combine(gameDir, DirectoryName);
        if (!Directory.Exists(directory))
        {
            return removed;
        }

        foreach (var file in Directory.GetFiles(directory, "*.ini"))
        {
            if (keep != null && keep.Contains(Path.GetFileName(file)))
            {
                continue;
            }

            var text = File.ReadAllText(file, FileEncoding);
            var rest = WithoutSection(text);
            if (rest.Length == text.Length)
            {
                continue;
            }

            if (rest.Trim().Length == 0)
            {
                File.Delete(file);
                removed.Add(Path.Combine(DirectoryName, Path.GetFileName(file)));
            }
            else
            {
                File.WriteAllText(file, rest, FileEncoding);
            }
        }

        if (Directory.GetFileSystemEntries(directory).Length == 0)
        {
            Directory.Delete(directory);
        }

        return removed;
    }

    /// <summary>
    /// Reads the section of each file in the folder, by patch id then option id, spelled
    /// as the files spell them. A missing folder or an unreadable file has none.
    /// </summary>
    public static Dictionary<string, Dictionary<string, string>> ReadFiles(string gameDir)
    {
        var result = new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);
        try
        {
            var directory = Path.Combine(gameDir, DirectoryName);
            if (!Directory.Exists(directory))
            {
                return result;
            }

            foreach (var file in Directory.GetFiles(directory, "*.ini"))
            {
                var values = ReadSection(file);
                if (values.Count > 0)
                {
                    result[Path.GetFileNameWithoutExtension(file)] = values;
                }
            }
        }
        catch
        {
            // An unreadable folder has no options to report.
        }

        return result;
    }

    private static bool IsHeader(string line, out string name)
    {
        var trimmed = line.Trim();
        if (trimmed.Length >= 2 && trimmed[0] == '[' && trimmed[^1] == ']')
        {
            name = trimmed[1..^1].Trim();
            return true;
        }

        name = string.Empty;
        return false;
    }

    /// <summary>The file's text with this manager's section taken out.</summary>
    private static string WithoutSection(string text)
    {
        var kept = new StringBuilder();
        var inSection = false;
        var dropped = false;
        foreach (var line in text.Split('\n'))
        {
            if (IsHeader(line, out var name))
            {
                inSection = string.Equals(name, Section, StringComparison.OrdinalIgnoreCase);
                dropped |= inSection;
            }

            if (!inSection)
            {
                kept.Append(line).Append('\n');
            }
        }

        if (!dropped)
        {
            return text;
        }

        // Split leaves one piece more than there were line ends.
        if (kept.Length > 0)
        {
            kept.Length -= 1;
        }
        return kept.ToString().TrimStart('\r', '\n');
    }

    private static Dictionary<string, string> ReadSection(string path)
    {
        var values = new Dictionary<string, string>();
        var inSection = false;
        foreach (var raw in File.ReadAllLines(path, FileEncoding))
        {
            var line = raw.Trim();
            if (line.Length == 0 || line[0] == ';' || line[0] == '#')
            {
                continue;
            }

            if (IsHeader(line, out var name))
            {
                inSection = string.Equals(name, Section, StringComparison.OrdinalIgnoreCase);
                continue;
            }

            var equals = line.IndexOf('=');
            if (inSection && equals > 0)
            {
                values[line[..equals].Trim()] = line[(equals + 1)..].Trim();
            }
        }

        return values;
    }
}
