# Shader Override

Shader Override lets KotOR 2 load replacement ARB shader files from disk.

## Setup

Place the `shaders` folder directly in the KotOR 2 game folder, next to the game executable. Shader files in that folder override matching shaders at runtime.

The `shaders/dump` folder contains the dumped vanilla shaders from the game.

Replacement files can match either:

- the friendly ident name from `shader_ident.txt`
- the default generated shader name

If a matching replacement file is found, Shader Override loads it instead of the original shader.

## In-Game Controls

- `F10`: show or hide the Shader Override overlay
- `Up` / `Down`: move through the shader list
- `Page Up` / `Page Down`: move by one page
- `Enter`: toggle the selected shader override on or off
- `Shift+Enter`: toggle the selected shader disabled state

The overlay lists discovered shaders, their target type, ident/default name, and current program id.
