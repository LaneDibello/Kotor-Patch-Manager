# ShaderSwap - Subtle Force Speed Effect

Confines the vanilla Force Speed blur to the outer edges of the screen while
leaving the center clear in KOTOR 1 and the KOTOR 2 Aspyr GOG and Steam
executables.

This patch embeds its shader in a small provider DLL and requires ShaderSwap,
which owns the OpenGL hook.

After changing the ARB file in `shaders`, run `prepare-shaders.bat`, then run
`create-patch.bat` normally.
