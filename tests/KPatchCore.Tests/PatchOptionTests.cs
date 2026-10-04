using Xunit;

using KPatchCore.Applicators;
using KPatchCore.Models;
using KPatchCore.Parsers;
using KPatchCore.Validators;

namespace KPatchCore.Tests;

/// <summary>
/// Patch options: declared in the manifest, tested by a hook's <c>when</c>, resolved at
/// install time into the hooks that reach patch_config.toml.
/// </summary>
public class PatchOptionTests
{
    private const string Manifest = """
        [patch]
        id = "sample"
        name = "Sample"
        version = "1.0.0"
        author = "Tests"
        description = "A patch with options."

        [[patch.options]]
        id = "map-notes"
        name = "Map notes"
        description = "Marker corrections."
        type = "toggle"
        default = false

        [[patch.options]]
        id = "hud-style"
        name = "HUD style"
        type = "choice"
        choices = [
            { id = "classic", name = "Classic" },
            { id = "compact", name = "Compact" },
        ]
        default = "classic"

        [patch.supported_versions]
        kotor1_gog_103 = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
        """;

    private const string Hooks = """
        [[hooks]]
        address = 0x00401000
        type = "simple"
        original_bytes = [0x90]
        replacement_bytes = [0x91]

        [[hooks]]
        address = 0x00402000
        type = "simple"
        original_bytes = [0x90]
        replacement_bytes = [0x92]
        when = "map-notes"

        [[hooks]]
        address = 0x00403000
        type = "simple"
        original_bytes = [0x90]
        replacement_bytes = [0xA1]
        when = { option = "hud-style", is = "classic" }

        [[hooks]]
        address = 0x00403000
        type = "simple"
        original_bytes = [0x90]
        replacement_bytes = [0xA2]
        when = { option = "hud-style", is = "compact" }
        """;

    private static PatchManifest ParsedManifest(string toml = Manifest)
    {
        var result = ManifestParser.ParseString(toml);
        Assert.True(result.Success, result.Error);
        return result.Data!;
    }

    private static List<Hook> ParsedHooks(string toml = Hooks)
    {
        var result = HooksParser.ParseString(toml);
        Assert.True(result.Success, result.Error);
        return result.Data!;
    }

    private static string ManifestWith(string options) => $"""
        [patch]
        id = "sample"
        name = "Sample"
        version = "1.0.0"
        author = "Tests"
        description = "A patch with options."

        {options}
        """;

    [Fact]
    public void ManifestWithoutOptionsHasNone()
    {
        Assert.Empty(ParsedManifest(ManifestWith("")).Options);
    }

    [Fact]
    public void ManifestDeclaresAToggleAndAChoice()
    {
        var options = ParsedManifest().Options;

        Assert.Equal(new[] { "map-notes", "hud-style" }, options.Select(o => o.Id));

        var toggle = options[0];
        Assert.Equal(PatchOptionType.Toggle, toggle.Type);
        Assert.Equal("Map notes", toggle.Name);
        Assert.Equal("Marker corrections.", toggle.Description);
        Assert.Equal(PatchOption.Off, toggle.Default);

        var choice = options[1];
        Assert.Equal(PatchOptionType.Choice, choice.Type);
        Assert.Equal(new[] { "classic", "compact" }, choice.Choices.Select(c => c.Id));
        Assert.Equal("Compact", choice.Choices[1].Name);
        Assert.Equal("classic", choice.Default);
    }

    [Fact]
    public void AToggleIsOffUnlessItSaysOtherwise()
    {
        var option = ParsedManifest(ManifestWith("""
            [[patch.options]]
            id = "extra"
            name = "Extra"
            """)).Options.Single();

        Assert.Equal(PatchOptionType.Toggle, option.Type);
        Assert.Equal(PatchOption.Off, option.Default);
    }

    [Theory]
    // No id, no name.
    [InlineData("[[patch.options]]\nname = \"X\"", "id")]
    [InlineData("[[patch.options]]\nid = \"x\"", "name")]
    // The id is what a hook and a command line name, so it keeps to the patch id's alphabet.
    [InlineData("[[patch.options]]\nid = \"has space\"\nname = \"X\"", "[a-zA-Z0-9_-]")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\n[[patch.options]]\nid = \"x\"\nname = \"Y\"", "more than once")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"slider\"", "unknown type")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ndefault = \"yes\"", "true or false")]
    // A choice needs something to choose between, and a default that is one of them.
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\ndefault = \"a\"", "choices array")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a\" }]\ndefault = \"a\"", "at least two")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a\" }, { id = \"b\" }]", "default")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a\" }, { id = \"b\" }]\ndefault = \"c\"", "not one of its choices")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a\" }, { id = \"a\" }]\ndefault = \"a\"", "more than once")]
    public void ManifestRefusesAMalformedOption(string options, string expected)
    {
        var result = ManifestParser.ParseString(ManifestWith(options));

        Assert.False(result.Success);
        Assert.Contains(expected, result.Error);
    }

    [Fact]
    public void HookWithoutWhenIsUnconditional()
    {
        Assert.Null(ParsedHooks()[0].When);
    }

    [Fact]
    public void WhenAsAStringIsAToggleThatIsOn()
    {
        var when = ParsedHooks()[1].When;

        Assert.NotNull(when);
        Assert.Equal("map-notes", when!.OptionId);
        Assert.Equal(PatchOption.On, when.Value);
    }

    [Fact]
    public void WhenAsATableNamesTheValue()
    {
        var when = ParsedHooks()[3].When;

        Assert.NotNull(when);
        Assert.Equal("hud-style", when!.OptionId);
        Assert.Equal("compact", when.Value);
    }

    [Fact]
    public void WhenCanTestAToggleForOff()
    {
        var hooks = ParsedHooks("""
            [[hooks]]
            address = 0x00401000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x91]
            when = { option = "map-notes", is = false }
            """);

        Assert.Equal(PatchOption.Off, hooks.Single().When!.Value);
    }

    [Theory]
    [InlineData("when = 3")]
    [InlineData("when = \"\"")]
    [InlineData("when = { is = \"compact\" }")]
    [InlineData("when = { option = \"hud-style\", is = 3 }")]
    public void HooksRefuseAMalformedWhen(string when)
    {
        var result = HooksParser.ParseString($"""
            [[hooks]]
            address = 0x00401000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x91]
            {when}
            """);

        Assert.False(result.Success);
        Assert.Contains("invalid when", result.Error);
    }

    [Fact]
    public void ShippedShapeValidates()
    {
        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), ParsedHooks());

        Assert.True(result.Success, result.Error);
    }

    [Fact]
    public void AConditionMustNameADeclaredOption()
    {
        var hooks = ParsedHooks(Hooks.Replace("when = \"map-notes\"", "when = \"map-markers\""));

        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), hooks);

        Assert.False(result.Success);
        Assert.Contains("'map-markers'", result.Error);
        Assert.Contains("does not declare", result.Error);
    }

    [Fact]
    public void AConditionMustNameAValueTheOptionCanHold()
    {
        var hooks = ParsedHooks(Hooks.Replace("is = \"compact\"", "is = \"tiny\""));

        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), hooks);

        Assert.False(result.Success);
        Assert.Contains("can never be 'tiny'", result.Error);
    }

    [Theory]
    // Unconditional beside conditional: both installed whenever the condition holds.
    [InlineData("", "when = \"map-notes\"")]
    // The same condition twice.
    [InlineData("when = \"map-notes\"", "when = \"map-notes\"")]
    // Different options: nothing stops both being on.
    [InlineData("when = \"map-notes\"", "when = { option = \"hud-style\", is = \"compact\" }")]
    public void HooksSharingAnAddressMustExcludeEachOther(string first, string second)
    {
        var hooks = ParsedHooks($"""
            [[hooks]]
            address = 0x00405000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x91]
            {first}

            [[hooks]]
            address = 0x00405000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x92]
            {second}
            """);

        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), hooks);

        Assert.False(result.Success);
        Assert.Contains("0x00405000", result.Error);
    }

    [Fact]
    public void AToggleCanSplitOneAddressBetweenOnAndOff()
    {
        var hooks = ParsedHooks("""
            [[hooks]]
            address = 0x00405000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x91]
            when = { option = "map-notes", is = true }

            [[hooks]]
            address = 0x00405000
            type = "simple"
            original_bytes = [0x90]
            replacement_bytes = [0x92]
            when = { option = "map-notes", is = false }
            """);

        Assert.True(OptionValidator.ValidateHookConditions(ParsedManifest(), hooks).Success);
    }

    [Fact]
    public void NothingChosenResolvesToTheDefaults()
    {
        var values = OptionValidator.ResolveValues(ParsedManifest(), null).Data!;

        Assert.Equal(PatchOption.Off, values["map-notes"]);
        Assert.Equal("classic", values["hud-style"]);
    }

    [Fact]
    public void AChosenValueReplacesItsDefault()
    {
        var chosen = new Dictionary<string, string> { ["map-notes"] = "TRUE", ["hud-style"] = "compact" };

        var values = OptionValidator.ResolveValues(ParsedManifest(), chosen).Data!;

        Assert.Equal(PatchOption.On, values["map-notes"]);
        Assert.Equal("compact", values["hud-style"]);
    }

    [Theory]
    [InlineData("map-markers", "true", "has no option 'map-markers'")]
    [InlineData("map-notes", "maybe", "cannot be 'maybe'")]
    [InlineData("hud-style", "tiny", "cannot be 'tiny'")]
    public void AChosenValueThatCannotApplyIsRefused(string option, string value, string expected)
    {
        var result = OptionValidator.ResolveValues(
            ParsedManifest(), new Dictionary<string, string> { [option] = value });

        Assert.False(result.Success);
        Assert.Contains(expected, result.Error);
    }

    [Fact]
    public void DefaultsInstallTheUnconditionalHookAndTheDefaultVariant()
    {
        var values = OptionValidator.ResolveValues(ParsedManifest(), null).Data!;

        var installed = OptionValidator.SelectHooks(ParsedHooks(), values);

        Assert.Equal(new ulong[] { 0x00401000, 0x00403000 }, installed.Select(h => h.Address));
        Assert.Equal(0xA1, installed[1].ReplacementBytes![0]);
    }

    [Fact]
    public void ChosenValuesInstallTheirHooksAndVariant()
    {
        var chosen = new Dictionary<string, string> { ["map-notes"] = "true", ["hud-style"] = "compact" };
        var values = OptionValidator.ResolveValues(ParsedManifest(), chosen).Data!;

        var installed = OptionValidator.SelectHooks(ParsedHooks(), values);

        Assert.Equal(new ulong[] { 0x00401000, 0x00402000, 0x00403000 }, installed.Select(h => h.Address));
        Assert.Equal(0xA2, installed[2].ReplacementBytes![0]);
        // What is left is what today's one-hook-per-address rule accepts.
        Assert.Empty(HookValidator.DetectOverlappingHooks(installed));
    }

    [Fact]
    public void APatchsSectionRecordsTheInstalledValues()
    {
        var chosen = new Dictionary<string, string> { ["map-notes"] = "true" };
        var values = OptionValidator.ResolveValues(ParsedManifest(), chosen).Data!;
        var config = new PatchConfig();
        config.AddPatch("sample", "patches/sample.dll", OptionValidator.SelectHooks(ParsedHooks(), values), values);

        // One section and a key per option; a toggle is 1 or 0, as the game's own INI
        // settings are.
        Assert.Equal("sample.ini", PatchOptionsIni.FileNameFor("sample"));
        Assert.Equal("[Patch Options]\r\nmap-notes=1\r\nhud-style=classic\r\n", PatchOptionsIni.Generate(config.Patches[0]));

        var toml = ConfigGenerator.GenerateConfigString(config);
        var model = Tomlyn.Toml.ToModel(toml);
        var patch = ((Tomlyn.Model.TomlTableArray)model["patches"])[0];

        // Neither the values nor the condition reach the runtime's file.
        Assert.DoesNotContain("options", toml);
        Assert.DoesNotContain("when", toml);
        Assert.Equal(3, ((Tomlyn.Model.TomlTableArray)patch["hooks"]).Count);
    }

    [Fact]
    public void APatchWithoutOptionsRecordsNothing()
    {
        var config = new PatchConfig();
        config.AddPatch("plain", string.Empty, new List<Hook>());

        Assert.Equal(string.Empty, PatchOptionsIni.Generate(config.Patches[0]));

        // An apply without options also clears what an earlier apply left behind.
        var directory = Directory.CreateTempSubdirectory("kpatch-options-").FullName;
        try
        {
            var folder = Path.Combine(directory, PatchOptionsIni.DirectoryName);
            Directory.CreateDirectory(folder);
            File.WriteAllText(Path.Combine(folder, "gone.ini"), "[Patch Options]\r\nx=1\r\n");

            Assert.True(PatchOptionsIni.WriteFiles(config, directory).Success);
            Assert.False(Directory.Exists(folder));
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [Fact]
    public void EachPatchWithOptionsGetsItsOwnFileAndKeepsItsOtherSettings()
    {
        var values = OptionValidator.ResolveValues(ParsedManifest(), null).Data!;
        var config = new PatchConfig();
        config.AddPatch("sample", string.Empty, new List<Hook>(), values);
        config.AddPatch("other", string.Empty, new List<Hook>(), new Dictionary<string, string> { ["x"] = "true" });
        config.AddPatch("plain", string.Empty, new List<Hook>());

        var directory = Directory.CreateTempSubdirectory("kpatch-options-").FullName;
        try
        {
            var folder = Path.Combine(directory, PatchOptionsIni.DirectoryName);
            Directory.CreateDirectory(folder);
            // The patch's own settings, with values from an earlier apply among them. One
            // is a byte that is not valid UTF-8, as a game writing its own code page leaves.
            var own = System.Text.Encoding.Latin1;
            File.WriteAllText(Path.Combine(folder, "other.ini"),
                "[Display]\r\nscale=2\r\n\r\n[Patch Options]\r\nx=0\r\nold=1\r\n\r\n[Sound]\r\nvolume=7\u00E9\r\n", own);
            // A patch that is no longer installed: one file with nothing but the section,
            // one with settings of its own.
            File.WriteAllText(Path.Combine(folder, "uninstalled.ini"), "[Patch Options]\r\nx=1\r\n");
            File.WriteAllText(Path.Combine(folder, "kept.ini"), "[Patch Options]\r\nx=1\r\n[Mine]\r\na=b\r\n");

            Assert.True(PatchOptionsIni.WriteFiles(config, directory).Success);

            Assert.Equal(
                new[] { "kept.ini", "other.ini", "sample.ini" },
                Directory.GetFiles(folder).Select(Path.GetFileName).OrderBy(n => n, StringComparer.Ordinal));
            Assert.Equal("[Patch Options]\r\nx=1\r\n\r\n[Display]\r\nscale=2\r\n\r\n[Sound]\r\nvolume=7\u00E9\r\n",
                File.ReadAllText(Path.Combine(folder, "other.ini"), own));
            Assert.Equal("[Mine]\r\na=b\r\n", File.ReadAllText(Path.Combine(folder, "kept.ini")));

            var read = PatchOptionsIni.ReadFiles(directory);
            Assert.Equal(new Dictionary<string, string> { ["x"] = "1" }, read["other"]);
            Assert.False(read.ContainsKey("kept"));

            // Uninstalling takes the sections out and leaves the patches' own settings.
            Assert.Equal(new[] { Path.Combine(PatchOptionsIni.DirectoryName, "sample.ini") },
                PatchOptionsIni.RemoveSections(directory));
            Assert.Equal("[Display]\r\nscale=2\r\n\r\n[Sound]\r\nvolume=7\u00E9\r\n",
                File.ReadAllText(Path.Combine(folder, "other.ini"), own));
            Assert.Equal(0xE9, File.ReadAllBytes(Path.Combine(folder, "other.ini"))[^3]);
            Assert.Empty(PatchOptionsIni.ReadFiles(directory));
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [Fact]
    public void OnlyAToggleReadsOneAndZeroAsOnAndOff()
    {
        Assert.Equal(PatchOption.On, PatchOptionsIni.FromIniValue(isToggle: true, "1"));
        Assert.Equal(PatchOption.Off, PatchOptionsIni.FromIniValue(isToggle: true, "0"));
        // A choice may have an entry whose id is a digit.
        Assert.Equal("1", PatchOptionsIni.FromIniValue(isToggle: false, "1"));
    }

    [Theory]
    // A choice id is what a hook and a command line name, like an option id.
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a b\" }, { id = \"c\" }]\ndefault = \"c\"", "[a-zA-Z0-9_-]")]
    // "true" and "false" are a toggle's values, and the config writes them as booleans.
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"true\" }, { id = \"false\" }]\ndefault = \"true\"", "cannot be called")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = \"choice\"\nchoices = [{ id = \"a\" }, \"b\"]\ndefault = \"a\"", "every choice needs an id")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\ntype = 3", "toggle")]
    [InlineData("[[patch.options]]\nid = \"x\"\nname = \"X\"\nchoices = [{ id = \"a\" }, { id = \"b\" }]", "takes no choices")]
    [InlineData("options = \"x\"", "array of tables")]
    public void ManifestRefusesAMalformedChoiceOrType(string options, string expected)
    {
        var result = ManifestParser.ParseString(ManifestWith(options));

        Assert.False(result.Success);
        Assert.Contains(expected, result.Error);
    }

    [Fact]
    public void ChoicesMayBeWrittenAsATableArray()
    {
        var manifest = ParsedManifest(ManifestWith(Q3Text(
            "[[patch.options]]",
            "id = \"hud-style\"",
            "name = \"HUD style\"",
            "type = \"choice\"",
            "default = \"compact\"",
            "[[patch.options.choices]]",
            "id = \"classic\"",
            "[[patch.options.choices]]",
            "id = \"compact\"",
            "name = \"Compact\"")));

        var option = Assert.Single(manifest.Options);
        Assert.Equal(new[] { "classic", "compact" }, option.Choices.Select(c => c.Id));
        // A choice without a name is shown by its id.
        Assert.Equal("classic", option.Choices[0].Name);
        Assert.Equal("compact", option.Default);
    }

    [Fact]
    public void WhenAsATableWithoutIsMeansOn()
    {
        var hooks = ParsedHooks(Q3Text(
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"simple\"",
            "original_bytes = [0x90]",
            "replacement_bytes = [0x91]",
            "when = { option = \"map-notes\" }"));

        Assert.Equal("map-notes", hooks[0].When!.OptionId);
        Assert.Equal(PatchOption.On, hooks[0].When!.Value);
    }

    [Fact]
    public void TheShorthandCannotNameAChoice()
    {
        var hooks = ParsedHooks(Q3Text(
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"simple\"",
            "original_bytes = [0x90]",
            "replacement_bytes = [0x91]",
            "when = \"hud-style\""));

        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), hooks);

        Assert.False(result.Success);
        Assert.Contains("can never be 'true'", result.Error);
    }

    [Fact]
    public void TwoPlainHooksAtOneAddressAreNotAnOptionsError()
    {
        var hooks = ParsedHooks(Q3Text(
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"simple\"",
            "original_bytes = [0x90]",
            "replacement_bytes = [0x91]",
            "",
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"simple\"",
            "original_bytes = [0x90]",
            "replacement_bytes = [0x92]"));

        var result = OptionValidator.ValidateHookConditions(ParsedManifest(), hooks);

        Assert.False(result.Success);
        Assert.Contains("Multiple hooks at address 0x00405000", result.Error);
        Assert.DoesNotContain("when", result.Error);
    }

    [Fact]
    public void AChoiceCanSplitOneAddressThreeWays()
    {
        var manifest = ParsedManifest(ManifestWith(Q3Text(
            "[[patch.options]]",
            "id = \"size\"",
            "name = \"Size\"",
            "type = \"choice\"",
            "choices = [{ id = \"small\" }, { id = \"medium\" }, { id = \"large\" }]",
            "default = \"medium\"")));
        var hooks = ParsedHooks(string.Join("\n", new[] { "small", "medium", "large" }.Select((size, i) => Q3Text(
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"simple\"",
            "original_bytes = [0x90]",
            $"replacement_bytes = [0x9{i + 1}]",
            $"when = {{ option = \"size\", is = \"{size}\" }}",
            ""))));

        Assert.True(OptionValidator.ValidateHookConditions(manifest, hooks).Success);

        var values = OptionValidator.ResolveValues(manifest, new Dictionary<string, string> { ["size"] = "large" }).Data!;
        var installed = Assert.Single(OptionValidator.SelectHooks(hooks, values));
        Assert.Equal(new byte[] { 0x93 }, installed.ReplacementBytes);
    }

    [Fact]
    public void AStaticHookWhoseOptionIsOffIsLeftOut()
    {
        var hooks = ParsedHooks(Q3Text(
            "[[hooks]]",
            "address = 0x00405000",
            "type = \"static\"",
            "original_bytes = [0x90]",
            "replacement_bytes = [0x91]",
            "when = \"map-notes\""));
        var manifest = ParsedManifest();

        // Selection runs before the static step, so a dropped static hook is never written.
        Assert.Empty(OptionValidator.SelectHooks(hooks, OptionValidator.ResolveValues(manifest, null).Data!));
        Assert.Single(OptionValidator.SelectHooks(hooks, OptionValidator.ResolveValues(
            manifest, new Dictionary<string, string> { ["map-notes"] = "true" }).Data!));
    }

    [Fact]
    public void AToggleValueIgnoresCaseAndAChoiceValueDoesNot()
    {
        var manifest = ParsedManifest();

        var toggle = OptionValidator.ResolveValues(manifest, new Dictionary<string, string> { ["map-notes"] = "TRUE" });
        Assert.True(toggle.Success, toggle.Error);
        Assert.Equal(PatchOption.On, toggle.Data!["map-notes"]);

        // A choice id is spelled as the manifest spells it, as it is in a hook's `is`.
        var choice = OptionValidator.ResolveValues(manifest, new Dictionary<string, string> { ["hud-style"] = "Compact" });
        Assert.False(choice.Success);
        Assert.Contains("cannot be 'Compact'", choice.Error);
    }

    [Fact]
    public void InstalledValuesReadBackAsTheyWereChosen()
    {
        var chosen = new Dictionary<string, string> { ["map-notes"] = "true", ["hud-style"] = "compact" };
        var values = OptionValidator.ResolveValues(ParsedManifest(), chosen).Data!;
        var config = new PatchConfig();
        config.AddPatch("sample", "patches/sample.dll", OptionValidator.SelectHooks(ParsedHooks(), values), values);
        config.AddPatch("plain", string.Empty, new List<Hook>());

        var directory = Directory.CreateTempSubdirectory("kpatch-options-").FullName;
        try
        {
            var game = Path.Combine(directory, "swkotor.exe");
            File.WriteAllBytes(game, Array.Empty<byte>());
            File.WriteAllText(Path.Combine(directory, "patch_config.toml"), ConfigGenerator.GenerateConfigString(config));
            Assert.True(PatchOptionsIni.WriteFiles(config, directory).Success);

            var info = PatchRemover.GetInstallationInfo(game);

            Assert.True(info.Success, info.Error);
            var installed = info.Data!.InstalledOptions["sample"];
            // The files' spelling comes back; a toggle is translated by whoever knows it is one.
            Assert.Equal("1", installed["map-notes"]);
            Assert.Equal(PatchOption.On, PatchOptionsIni.FromIniValue(isToggle: true, installed["map-notes"]));
            Assert.Equal("compact", installed["hud-style"]);
            Assert.False(info.Data.InstalledOptions.ContainsKey("plain"));
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    private static string Q3Text(params string[] lines) => string.Join("\n", lines);
}
