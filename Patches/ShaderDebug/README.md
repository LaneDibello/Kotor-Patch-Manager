# Shader Debug

Shader Debug dumps KotOR 2 ARB shaders, loads hash-named replacements from disk,
and provides an in-game shader overlay for live editing/toggling.

## Setup

The `shaders/dump` folder contains the dumped vanilla shaders from the game.

Replacement files must use the generated hash name shown by the overlay and dump
folder, such as `fragment_8fa5458be01d11cf.arb`. If a matching replacement is
found, Shader Debug loads it instead of the original shader, fromt he shaders folder.

## In-Game Controls

- `F10`: show or hide the Shader Debug overlay
- `Up` / `Down`: move through the shader list
- `Page Up` / `Page Down`: move by one page
- `Enter`: toggle the selected shader override on or off
- `Shift+Enter`: toggle the selected shader disabled state
