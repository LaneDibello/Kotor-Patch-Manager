## Game API needs
- The list options are typically built with 3 buttons, the main select, and a left/right selector

## GUI needs
- Need to create our own control type for the list select.
	- The base game constructs this from 3 buttons
	- So we'll just need to bundle these together into a custom class to get them working

## Stretch goals
- A `button` option in the TOML format
	- This would have no direct association in an INI file, and instead just run a function
- GUI compatibilities with widescreen UI mods