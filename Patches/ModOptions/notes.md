## Description handling
Should cannibalize `CSWGuiOptionsMain::SetDescription`

Need to create an `OnEnterSlot` function that will fire on HoverEnter for all the options. This function will set the current description. This will be done first by setting the text params on the description label. Clear items on the description list box. Set the extent height of the label text using GetIdealHeight. And add the label.

The constructor for the menu also should run initialize against the description label, using the proto item extent, and a float param of 1.0f

## Modal stack
Adding a new panel with options `3, 1`, should put it on the modal stack. The back button should be able to get away with the default `OnBButtonPressed` result.

## Refreshing Mod Options
Should just poll the toml files again in case one was added, and rebuild the options menus. 

## Input Event Handling
For both the ModOptions and OptionsMenu classes.

On `A` button, I think the underlying panel handler should divert the event to the active control which will select the button just fine. 

On `B` button and escape, we should just divert to the back button handler. We may want to add a sound here, or directly pop the modal if that leads to issues

## Function Handling
Who even knows man