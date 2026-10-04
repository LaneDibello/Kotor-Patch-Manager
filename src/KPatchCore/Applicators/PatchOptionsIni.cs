using System.Text;
using KPatchCore.Models;

namespace KPatchCore.Applicators;

/// <summary>
/// Writes and reads patch-options.ini, the record of the option values the installed
/// patches were applied with.
/// </summary>
/// <remarks>
/// One file for every patch, in the game directory: a section per patch id and a key per
/// option id. A toggle is 1 or 0 and a choice is its id, the forms the game's own INI
/// settings take, so a patch's code and a script read them with the usual INI functions.
/// The file is this manager's output. It is rewritten on every apply, and nothing reads
/// it to decide which hooks to install: those were chosen when the patches were applied.
/// </remarks>
public static class PatchOptionsIni
{
    public const string FileName = "patch-options.ini";

    /// <summary>The spelling the file uses for an option value.</summary>
    public static string ToIniValue(string value) => value switch
    {
        PatchOption.On => "1",
        PatchOption.Off => "0",
        _ => value
    };

    /// <summary>
    /// The value an entry of the file stands for. Only a toggle is translated: a choice's
    /// id may itself be "0" or "1".
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
    /// The file's content for a configuration, or an empty string when no patch in it has
    /// options.
    /// </summary>
    public static string Generate(PatchConfig config)
    {
        var text = new StringBuilder();
        foreach (var patch in config.Patches.Where(p => p.Options.Count > 0))
        {
            if (text.Length > 0)
            {
                text.Append("\r\n");
            }

            text.Append('[').Append(patch.Id).Append("]\r\n");
            foreach (var (optionId, value) in patch.Options)
            {
                text.Append(optionId).Append('=').Append(ToIniValue(value)).Append("\r\n");
            }
        }

        return text.ToString();
    }

    /// <summary>
    /// Writes the file into the game directory, or removes one left by an earlier apply
    /// when no patch has options now.
    /// </summary>
    public static PatchResult WriteFile(PatchConfig config, string gameDir)
    {
        try
        {
            var path = Path.Combine(gameDir, FileName);
            var text = Generate(config);
            if (text.Length == 0)
            {
                File.Delete(path);
                return PatchResult.Ok("No patch options to record");
            }

            File.WriteAllText(path, text);
            return PatchResult.Ok($"Options recorded: {path}");
        }
        catch (Exception ex)
        {
            return PatchResult.Fail($"Failed to write {FileName}: {ex.Message}");
        }
    }

    /// <summary>
    /// Reads the file: the entries of each section, by patch id then option id, spelled as
    /// the file spells them. A missing or unreadable file has none.
    /// </summary>
    public static Dictionary<string, Dictionary<string, string>> ReadFile(string gameDir)
    {
        var result = new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);
        try
        {
            var path = Path.Combine(gameDir, FileName);
            if (!File.Exists(path))
            {
                return result;
            }

            Dictionary<string, string>? section = null;
            foreach (var raw in File.ReadAllLines(path))
            {
                var line = raw.Trim();
                if (line.Length == 0 || line[0] == ';' || line[0] == '#')
                {
                    continue;
                }

                if (line[0] == '[' && line[^1] == ']')
                {
                    var id = line[1..^1].Trim();
                    if (!result.TryGetValue(id, out section))
                    {
                        result[id] = section = new Dictionary<string, string>();
                    }
                    continue;
                }

                var equals = line.IndexOf('=');
                if (section != null && equals > 0)
                {
                    section[line[..equals].Trim()] = line[(equals + 1)..].Trim();
                }
            }
        }
        catch
        {
            // An unreadable file has no options to report.
        }

        return result;
    }
}
