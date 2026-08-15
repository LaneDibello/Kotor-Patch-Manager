# ShaderSwap

ShaderSwap is the shared runtime hook used by separate shader replacement patches.
Dependent provider DLLs register their embedded replacement shaders at startup.

## Creating a replacement patch

1. Create a separate patch that requires `shader-swap`.
2. Put each replacement `.arb` file in that patch's `shaders` directory.
3. Name it with the original shader ID, for example
   `fragment_41e7c099fbb00335.arb` or `vertex_de4f8c79d8920b7e.arb`.
4. Run its preparation script to generate the embedded header.
5. Run that patch's `create-patch.bat`.

The original shader ID is the `fragment_...` or `vertex_...` ID produced by the
Shader Debug patch. The provider C++ does not need to change when shaders
are added or edited.

At runtime the patch hashes each original shader and replaces only exact ID
matches. Replacements may be any length. ShaderSwap uses the builder's minimal
mode, so only the patch's local sources are compiled; the Common/GameAPI runtime
and SQLite are not included. The C/C++ runtime is linked statically.

ShaderSwap also intercepts ARB fragment programs created by the K1 Modern Driver
Compatibility patch. Use shader IDs dumped while that patch is active, because
its generated fragment programs differ from K1's original vendor shader paths.

## Supported executables

- KotOR 1 GOG 1.0.3
- KotOR 1 Steam 1.0.3
- KotOR 1 HellSpawn/DeadlyStream CD crack 1.0.3
- KotOR 2 GOG Aspyr 1.0.2
- KotOR 2 Steam Aspyr 1.0.2

The hook is shared across these versions and contains no game-address-specific
code. Shader IDs are based on exact source bytes, so use the ID calculated from
the executable version containing the shader you intend to replace.
