## Game API needs
- Will likely need some testing for edit box and slider
- The list options are typically built with 3 buttons, the main select, and a left/right selector

## GUI needs
- Need to figure out a way to make the button on the options page a bit prettier
	- Right now I'm using fill to make the background the right color, however  this gives it square corners
	- It also doesn't look right when hovering over
	- If I use the rounded corner textures, the shape is better, but the fill is wrong.
	- The close button on that page actually doesn't have a fill, instead the actual background texture is just adjusted. I want to avoid this if I can, so that supporting GUI aspect ratio mods isn't as tough
- Right now, the prototype item on the Options view is configured for Checkboxes only
	- we need to make it more generic so that other options types will work
	- We need to find the aspects that overlap between our targeted option types, and which control ID would be ideal for that.
	- We can then manually set/modify aspects of each control based on the type
- Need to create our own control type for the list select.
	- The base game constructs this from 3 buttons
	- So we'll just need to bundle these together into a custom class to get them working

## Stretch goals
- A `button` option in the TOML format
	- This would have no direct association in an INI file, and instead just run a function
- GUI compatibilities with widescreen UI mods