## Function Handling
We need some way for patches to register functions that can be referenced by this patch.

Since the other patches already export functions, a GetModuleHandle call should be able to retrieve them.

## Default options
Each option in the toml spec will need a default field.
The onDefault callback will update the state of all the buttons, and also restore the ini entries to these default values.

## TOML needs
- Defaults
- Option names to render in the options menu

## Game API needs
- Consider implementing `CSWGuiOptionsCheckbox` for these toggles
- Will likely need some testing for edit box and slider
- The list options are typically built with 3 buttons, the main select, and a left/right selector

## GUI files
I should really build out the `.gui` files for these menus. Probably gonna base them off of `CSWGuiOptionsFeedback`
Will just need to swap default for refresh on the ModOptions panel, and also work out how a generic prototype for 
each of these different option types will work out.