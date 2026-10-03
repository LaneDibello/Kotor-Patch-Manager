# KotOR Patch Manager Version 1.0.0 Release Road Map
This document is intended to outline the prerequisites for a 1.0.0 release version. At time of writing we are still in Beta 0.7.X, and new releases remain in Beta. It is our hope that by 2027 we this project has reached a point where we can have an official release.

To summarize our goals:
- Upgrade the Launcher UI
- Cement support for MacOS and Linux platforms
- Expand support for KotOR 2 versions
- Promotion of KPM to the wider community, and community testing
- Addressing remaining action items from the [2026 road-map](./Roadmap2026.md)

## UI Upgrades
The Patch Launcher UI was also meant to be a temporary measure, and the degree to which it has been adopted was unexpected. Originally it was intended that the UI would be retired in-favor of a headless approach running with an existing mod management platform (such as HoloPatcher and/or Mod Organizer). And while we have proven out the potential for headless approaches to patching, demand for a dedicated UI remains, and has been adopted as a goal of this project.

Blue (of KotOR.js fame) has demonstrated a UI rework [here](https://github.com/KobaltBlu/Kotor-Patch-Manager/tree/feat/ui-rework). It is not currently in a ready state, but is approaching what we're looking for in the final version of the UI.

Particularly we want:
- Patch Organization via:
	- Categorization
	- A search feature
	- Some way of "Tagging" patches (could be baked into categorization)
	- Patch load-outs or presets
- Friendly UX on Error paths (it's currently very difficult to get any sort of error message or code from the UI)
- More robust patch information on selection
	- Some degree fo formatting for the patch description
	- Patch requirements and conflicts in the UI
	- Patch supported versions in the UI
- The ability to save and easily switch between paths/directories for patches and games.
- Something that looks slick 😎

## Cross Platform Compatibility
For 1.0.0, we wanted to see patches being applied on Mac OS and Linux. Thanks to the work of Synchro and FTD, this has been accomplished!

What remains:
- Game API support for these platforms
	- Basic detours have been demonstrated
	- More advanced GameAPI usage will require expansion to the address DataBases for these versions

Out of scope:
- Patching Support for Mobile Platforms
- Patching Support for Xbox versions

Ultimately I do not feel this category is blocking 1.0.0 anymore.

## KotOR 2 Version Support
The core features are largely supported for KotOR 2 at this point. The main area where we are lacking is in Address Database (and by extension GameAPI) support for this game.

Getting support here is a little more tedious than with KotOR 1, as there are multiple versions of KotOR 2 (with different addressing) that are readily used by the community. Namely the Steam and GoG versions (not to mention the various MacOS and Linux variants).

There has been a lot of progress on the reverse engineering front for this game's versions, and this effort is well on its way.

Before 1.0.0, I want the existing patches using GameAPI (that are relevant to KotOR 2) to be supported in at least one of the versions (likely GoG).

## Widespread Promotion and Testing
We've already had many users try out KPM, and find issues. We want to continue with this process. In order to do this we'll need to promote the program more, and ask for testers. This will mean sharing about KPM on Reddit, DeadlyStream, and Discord.

## A Review of 2026 Goals and Milestones
Our [2026 road map](./Roadmap2026.md) has several goals and milestones laid out as well, many of which have already been accomplished:

- Onboard at least 2 more developers
	- **Complete!** We have 4 major collaborators as well as 6 other minor contributors.
- Demo an integration with HoloPatcher
	- **Complete!** Vriff has made many advancements to HoloPatcher, and J has demonstrated KPM running headless with other mod organization software. So this path has been proved out.
- At least one mod exists using KPM
	- **Complete!** There are several (albeit not particularly popular) mods using KPM now. INcluding but not limited to: Lane's Mod Options Menus, Rayman's KMRP, Vriff's Active Party Extension, and more
- KotOR 2 Reverse Engineering progress
	- As mentioned above there is still more to be done here. However some seriously significant progress has been made here, thanks to efforts of Vriff and Synchro.
- Collaboration Ground Work
	- This was half met. Several tutorials and guides were created, but they were hardly comprehensive.
	- At time of writing there is a drafted collaboration guide, though it isn't yet complete
- Patch Framework & Game API enhancements
	- A lot was done here, but we haven't quite made it yet
	- Address databases have expanded some, but still need a lot of work for KotOR 2 and other platforms
	- GameAPI has expanded *a lot*, and only continues to
	- We have not designed nor achieved a great options for "Global Data Redirection", that is having a hook type that allows us to replace references to a region of memory we control. Currently, patch makers that want to do this are just detouring around the reference, and handling this in C++
	- We never did switch to relative jumps for detour and replace hooks. While an interesting idea in theory, this complicates things to a degree that just turned out not to be tenable. We may revisit this at some point. But honestly a 4-byte hook instead of a 5-byte hook isn't all that significant in most cases.
	- Support for KotOR 2 really took off since the last roadmap, however there are still lacking areas. See above discussion for details
	- Support for GameAPIO version gating was adding via the `GameVersion` class
- PyKotor/HoloPatcher Integrations
	- We really spent time re-thinking a lot of this
	- We no longer consider PyKotor to be a relevant target for any of our efforts here
	- Vriff's updates to HoloPatcher have been promising however
	- J has demonstrated a headless deployment of patches, as mentioned above.
	- So while the original milestone wasn't necessarily achieved, I do feel that the spirit of this effort was accomplished.
- Patch Creation
	- So many patches have been created. I believe we've achieved this with flying colors. 