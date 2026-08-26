## Function Handling
We need some way for patches to regsiter functions that can be referenced by this patch.

A few ideas:

- Somehow get this patch in the dependancy path of patches that want to work with this
- Just have a file this patch creates that other patches can append entries to, that this patch can use as a library to pull up functions
- Have something in the common lib that all patches can pull from that will establish a re-usable IPC (Inter-Patch Communitication) format for other things like this
- Have a means for patches to compile a static lib with the functions they want to regsiter alongside the main deliverable. I'm actually liking this idea a bit more.

## Default options
Each option in the toml spec will need a default field.
The onDefault callback will update the state of all the buttons, and also restore the ini entries to these default values.

## TOML needs
- Defaults
- Option names to render in the options menu

## Game API needs
- Consider implementing `CSWGuiOptionsCheckbox` for these toogles
- Will likley need some testing for edit box and slider
- The list options are typically built with 3 buttons, the main select, and a left/right selector