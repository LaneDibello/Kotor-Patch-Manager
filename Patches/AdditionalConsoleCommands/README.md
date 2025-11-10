# Additional Console Commands

This is a patch that proves out the addition of custom console commands to the game's cheat menu.
- [See related demo video](https://www.youtube.com/watch?v=aMvVloM7EuY)
- [Also see this video](https://www.youtube.com/watch?v=kQW1VIIueJY)

## Background
The cheat menu in this game has always left a bit to be desired functionality wise in my opinion. While there were some useful commands in there, you couldn't do more advanced things like running scripts, triggering debug views, etc. So i figured I'd look into it.

## How Console Commands work in Kotor
All of these commands get initialized statically shortly after game start-up. The game does this by building an object called a `ConsoleFunc`.
- A `ConsoleFunc` is simply a name (80 characters or less), and a pointer to a function.
- These functions can take either No parameters, an int parameter, or a string parameter (note that the string parameter is a good option for multi-param/variadic approaches as you can just parse out whatever params you want)
- A part of this object's constructors, adds the command to a registry of other functions.

## How this patch works
In this patch we compile C++ and inject it into the game.
It is made available via `InitializeAdditionalCommands`, which we patch in shortly after the existing console commands are initialized. This function simply builds `ConsoleFunc`s by calling the relevant in-game constructor. Each of these contains a pointer to an actual function that we define here.
Console Functions are always called with the `__cdecl` convention. Take a look at `runscript` for example. It takes a `char*` parameter (the string param variant of `ConsoleFunc`). We then call the internal game function `GetPlayerCreatureId` (in the `CServerExoApp` class), to get the ID of the player. Finally we call the internal game function `RunScript` (in the `CVirtualMachine` class), to run the script supplied as called by the player.

### More details
For kotor 1, we specially hook at address: `0x006fb4ce`, which is actually in the EXE entry point, shortly after all the other static structures are initialized, but before the main program begins.
The bytes (`original_bytes`) we overwrite: `[0x89, 0x45, 0xe0, 0x3b, 0xc6]` represent the x86 instructions:
```
MOV dword ptr [EBP + -0x20]
CMP EAX,ESI
```
These get replaced by an unconditional jump, that brings us to code that saves all the register values, calls our `InitializeAdditionalCommands`, restores the registers, runs the overwritten byte code (the move and compare above), and finally jumps back to after the overwritten bytes at `0x006fb4d3` (5 bytes later). and allows the process to continue.
This is what I call a "detour" patch. We basically take a break in the middle of execution go do our own thing, and return back to where we started as if nothing happens.

## How might I go about added a console command?
Well the hard part is over-with. So to add additional commands you just need to write up a new C++ function to be compiled with this patch, and add a `ConsoleFunc` construction for it in `InitializeAdditionalCommands`.
