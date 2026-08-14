# ShaderSwap

ShaderSwap is the shared runtime hook used by separate shader replacement patches.

## Creating a replacement patch

1. Create a separate patch that requires `shader-swap`.
2. Put each replacement `.arb` file in that patch's `shaders` directory.
3. Name it with the original shader ID, for example
   `fragment_41e7c099fbb00335.arb` or `vertex_de4f8c79d8920b7e.arb`.
4. Run its preparation script to generate the embedded header.
5. Run `create-patch.bat` in the directory
