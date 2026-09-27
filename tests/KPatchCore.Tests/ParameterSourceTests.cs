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
    [InlineData("zmm0", false)]
    [InlineData("", false)]
    public void ParseTimeAcceptsWhatSomeGeneratorReads(string source, bool expected)
    {
        Assert.Equal(expected, Source(source).IsValid(out _));
    }

    [Theory]
    // Decimal and hexadecimal, in either case, matching ParseConstantSource in
    // wrapper_base.h. A constant is a literal, so both generators take it.
    [InlineData("const:0")]
    [InlineData("const:0xBC")]
    [InlineData("const:4294967295")]
    public void ConstantsAreReadableEverywhere(string source)
    {
        Assert.True(Source(source).IsValidFor(Architecture.x86, out var x86), x86);
        Assert.True(Source(source).IsValidFor(Architecture.x86_64, out var x64), x64);
    }

    [Theory]
    // A sign would wrap into a huge unsigned value rather than fail, so it is turned away.
    [InlineData("const:-1")]
    [InlineData("const: 5")]
    [InlineData("const:")]
    [InlineData("const:zz")]
    [InlineData("const:0xZZ")]
    [InlineData("const:12abc")]
    public void MalformedConstantsAreRefused(string source)
    {
        Assert.False(Source(source).IsValid(out var error));
        Assert.NotNull(error);
    }

    [Fact]
    public void ConstantMustFitItsDeclaredType()
    {
        var tooWide = new Parameter { Source = "const:0x100", Type = ParameterType.Byte };
        Assert.False(tooWide.IsValidFor(Architecture.x86, out var error));
        Assert.Contains("Byte", error);

        var fits = new Parameter { Source = "const:0xFF", Type = ParameterType.Byte };
        Assert.True(fits.IsValidFor(Architecture.x86, out _));
    }

    [Fact]
    public void OnlyASixtyFourBitPointerHoldsAnAddressConstant()
    {
        var address = new Parameter
        {
            Source = "const:0x100480ABD",
            Type = ParameterType.Pointer
        };

        Assert.True(address.IsValidFor(Architecture.x86_64, out _));
        // The same constant on a 32-bit build is a value that target cannot represent.
        Assert.False(address.IsValidFor(Architecture.x86, out var error));
        Assert.Contains("0xFFFFFFFF", error);
    }

    [Theory]
    // No source of any kind makes these readable on a 32-bit target, so the refusal is
    // about the type rather than the source.
    [InlineData(ParameterType.Int64)]
    [InlineData(ParameterType.UInt64)]
    [InlineData(ParameterType.Double)]
    public void SixtyFourBitTypesAreRefusedOnX86(ParameterType type)
    {
        var register = new Parameter { Source = "eax", Type = type };
        Assert.False(register.IsValidFor(Architecture.x86, out var error));
        Assert.Contains("64 bits wide", error);

        Assert.True(register.IsValidFor(Architecture.x86_64, out var wide), wide);
    }

    [Fact]
    public void AConstantCannotBeAFloat()
    {
        var value = new Parameter { Source = "const:1", Type = ParameterType.Float };

        Assert.False(value.IsValidFor(Architecture.x86, out var error));
        // Not the width message: a float is refused outright, so quoting a widest value
        // for it would be nonsense.
        Assert.Contains("cannot be a Float", error);
    }

    [Theory]
    // A dereference reads through the register, so what matters is that the register is
    // readable. ESP is allowed here although it is refused on its own: the slot's contents
    // are reachable even though the wrapper keeps no saved copy of the pointer.
    [InlineData("[eax]")]
    [InlineData("[esi+4]")]
    [InlineData("[ebp-0x10]")]
    [InlineData("[esp+8]")]
    [InlineData("[esp]")]
    public void X86ReadsADereference(string source)
    {
        Assert.True(Source(source).IsValidFor(Architecture.x86, out var error), error);
    }

    [Theory]
    [InlineData("[r15]")]
    [InlineData("[rsi+0x10]")]
    [InlineData("[rsp+8]")]
    public void X86_64ReadsADereference(string source)
    {
        Assert.True(Source(source).IsValidFor(Architecture.x86_64, out var error), error);
    }

    [Theory]
    // The register inside still has to be one this target reads, and the offset still has
    // to parse.
    [InlineData("[r15]")]
    [InlineData("[nosuchreg]")]
    [InlineData("[esi+zz]")]
    [InlineData("[esi+]")]
    [InlineData("[]")]
    [InlineData("[esi")]
    [InlineData("esi]")]
    public void X86RefusesABadDereference(string source)
    {
        Assert.False(Source(source).IsValidFor(Architecture.x86, out var error));
        Assert.NotNull(error);
    }

    [Fact]
    public void ADereferenceMayBeNarrowWhereAnAddressMayNot()
    {
        // The address form yields a pointer whatever the slot holds, so a narrow type there
        // describes something else. Through brackets a value arrives, so it can be narrow.
        var address = new Parameter { Source = "esp+8", Type = ParameterType.Byte };

        Assert.False(address.IsValidFor(Architecture.x86, out var error));
        Assert.Contains("cannot be read as Byte", error);

        var value = new Parameter { Source = "[esp+8]", Type = ParameterType.Byte };
        Assert.True(value.IsValidFor(Architecture.x86, out error), error);
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
