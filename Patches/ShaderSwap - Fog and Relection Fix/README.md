# ShaderSwap - Fog and Relection Fix

Fog and reflection shader replacements for the KOTOR 2 Aspyr GOG and Steam executables.

This patch embeds its shaders in a small provider DLL. It requires ShaderSwap,
which owns the OpenGL hook, hashes the original shader source, and uses the
matching embedded replacement. Replacements may be any length.

After changing files in `shaders`, run `prepare-shaders.bat`, then run
`create-patch.bat` normally. The preparation step generates
`ShaderReplacements.generated.h`; the normal builder then compiles the provider
DLL and packages it.
