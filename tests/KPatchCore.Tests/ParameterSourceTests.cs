using Xunit;

using KPatchCore.Models;

namespace KPatchCore.Tests;

/// <summary>
/// The parameter sources each wrapper generator can read.
/// </summary>
/// <remarks>
/// These cases are the C# half of a table the C++ generators implement. Every source named
/// here was read out of the register cases in wrapper_x86.cpp and wrapper_x86_64.cpp, so a
/// change to either generator should fail something in this file.
/// </remarks>
public class ParameterSourceTests
{
    private static Parameter Source(string source) =>
        new() { Source = source, Type = ParameterType.Int };

    [Theory]
    // The seven PUSHAD saves that a hook may name. ESP is the eighth and is not offered.
    [InlineData("eax")]
    [InlineData("ebx")]
    [InlineData("ecx")]
    [InlineData("edx")]
    [InlineData("esi")]
    [InlineData("edi")]
    [InlineData("ebp")]
    // Both generators lowercase the source before matching.
    [InlineData("ESI")]
    [InlineData("esp+0")]
    [InlineData("esp+116")]
    [InlineData("esp-4")]
    public void X86Reads(string source)
    {
        Assert.True(Source(source).IsValidFor(Architecture.x86, out var error), error);
    }

    [Theory]
    // 64-bit spellings, absent from the x86 table and refused. "rbp" is the near miss,
    // x86 having "ebp"; "r15d" the one that looks narrow but is not.
    [InlineData("rbp")]
    [InlineData("r15")]
    [InlineData("r15d")]
    [InlineData("rsp+8")]
    // The stack pointer has no saved copy. "esp+0" is how a hook asks for it.
    [InlineData("esp")]
    // Nothing dereferences, and the brackets reach the generator intact.
    [InlineData("[eax]")]
    // An offset that will not parse.
    [InlineData("esp+zz")]
    [InlineData("esp+")]
    [InlineData("xmm0")]
    public void X86Refuses(string source)
    {
        Assert.False(Source(source).IsValidFor(Architecture.x86, out var error));
        Assert.NotNull(error);
    }

    [Theory]
    [InlineData("rax")]
    [InlineData("rbx")]
    [InlineData("rcx")]
    [InlineData("rdx")]
    [InlineData("rsi")]
    [InlineData("rdi")]
    [InlineData("rbp")]
    // The narrow spellings name the same physical register, so both are accepted.
    [InlineData("eax")]
    [InlineData("ebp")]
    [InlineData("r8")]
    [InlineData("r15")]
    [InlineData("r8d")]
    [InlineData("r15d")]
    [InlineData("rsp+8")]
    [InlineData("rsp-16")]
    // Kept deliberately, so a hook ported off the Windows build needs no edit.
    [InlineData("esp+8")]
    public void X86_64Reads(string source)
    {
        Assert.True(Source(source).IsValidFor(Architecture.x86_64, out var error), error);
    }

    [Theory]
    // LookUpRegister finds these, but SavedGprIndex does not: the wrapper has replaced the
    // stack pointer by the time the patch function runs.
    [InlineData("rsp")]
    [InlineData("esp")]
    [InlineData("[rax]")]
    [InlineData("xmm0")]
    [InlineData("rip")]
    public void X86_64Refuses(string source)
    {
        Assert.False(Source(source).IsValidFor(Architecture.x86_64, out var error));
        Assert.NotNull(error);
    }

    [Theory]
    // A source another generator reads, and one no generator reads. Neither gets further
    // than the missing generator.
    [InlineData("eax")]
    [InlineData("x0")]
    public void ArmHasNoGenerator(string source)
    {
        Assert.False(Source(source).IsValidFor(Architecture.ARM64, out var error));
        Assert.Contains("wrapper generator", error);
    }

    [Theory]
    // Patches are parsed before a game is chosen, so either architecture's spelling passes.
    [InlineData("esi", true)]
    [InlineData("rax", true)]
    [InlineData("r15", true)]
    [InlineData("esp+8", true)]
    [InlineData("rsp+8", true)]
    // Refused everywhere, so refused here too.
    [InlineData("esp", false)]
    [InlineData("rsp", false)]
    [InlineData("[eax]", false)]
    [InlineData("zmm0", false)]
    [InlineData("", false)]
    public void ParseTimeAcceptsWhatSomeGeneratorReads(string source, bool expected)
    {
        Assert.Equal(expected, Source(source).IsValid(out _));
    }

    [Fact]
    public void ErrorNamesTheArchitectureThatCannotReadIt()
    {
        Assert.False(Source("r15").IsValidFor(Architecture.x86, out var error));
        Assert.Contains("r15", error);
        Assert.Contains("x86", error);
        // Each stack register has a plus and a minus prefix, which named it twice.
        Assert.DoesNotContain("esp / esp", error);

        Assert.False(Source("zmm0").IsValidFor(Architecture.x86_64, out var wide));
        Assert.Contains("rsp / esp", wide);
    }
}
