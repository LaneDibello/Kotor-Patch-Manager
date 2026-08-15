# ShaderSwap - Color Correction

Color correction for the KOTOR 2 Aspyr GOG and Steam executables, using the
shader from the original K2AspyrColorCorrection patch.

This patch embeds its shader in a small provider DLL and requires ShaderSwap,
which owns the OpenGL hook.

After changing the ARB file in `shaders`, run `prepare-shaders.bat`, then run
`create-patch.bat` normally.
