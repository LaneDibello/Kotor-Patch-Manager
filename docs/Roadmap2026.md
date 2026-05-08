# KotOR Patch Manager 2026 Road-Map

This intention of this document is to track planned work and goals for this project as of the start of 2026. These plans are not comprehensive, and can (and likely will) change as time goes on.

## Goals
In this upcoming year I hope to accomplish the following:
- Onboard at least 2 more developers to the Patch Manager project
	- This would look like them being able to make a contribution (of any nature) and PR to the project
	- One prerequisite to this would be to create some form of contribution guide, as well as additional tutorials for patch creation
- Demo an integration with [HoloPatcher](https://github.com/OldRepublicDevs/PyKotor/tree/master/Tools/HoloPatcher) 
- At least one mod exists that relies on a patch created in this ecosystem
- KotOR 2 Reverse Engineering has significant progress
	- I am intentionally not defining "significant" here

## Milestones
Currently this project is in a state where the road-map isn't particularly linear. Rather, there are several concurrent Milestones I want to work towards.

### Collaboration Ground Work
The following needs to be completed in-order to facilitate collaboration on this project:

- Road Map document (this)
	- By laying out my intentions with this project in this document, I  hope that potential collaborators will have a better concept of what there is to be worked on, and what help is needed
- Collaboration Guide
	- A guide with information about how to collaborate is a must for a project of this scale
	- This would include a walk-through for building from source (expanding upon the existing information in the root README)
	- We'd also want to include some basics regarding our preferred PR process, code-style, and the like
- Ghidra guides/tutorials
	- There are plenty of good guides out there for working with Ghidra itself (which should be linked in associated tutorials)
	- I want to take some time to document aspects the particular Ghidra project for kotor 1 that I've worked on, and tutorialize certain common things one may run into when working with this project
	- Furthermore, I want to document how best to find things in kotor 1 in Ghidra
	- I also want to document how to use ghidra as a source of truth for patch creation

### Patch Framework & Game API enhancements
Currently, a host of useful helper-functions and integrations exists in the `Patches/Common` directory. I've been loosely referring to this as the "Patch Framework". And the portion that directly interfaces with the Game itself via the AddressDatabases, I've been calling GameAPI.
There are several improvements to this ecosystem that I'm hoping to make:

- Address Database Expansion
	- Right now (nearly) every function address in kotor 1 has been exported to its data base
	- Tooling exists for exporting datatype offsets as well
	- This database should continue to grow with more exported information from the reverse engineering project. This would include exporting datatype offsets, formalizing some of the function schema, and exporting Global Constants/Pointers into the database as well
- GameAPI Expansion 
	- In additional to growing address databases, wrappers to support these functions, offsets, and globals are also necessary.
	- I've laid out some rudimentary plans in issue [#63](https://github.com/LaneDibello/Kotor-Patch-Manager/issues/63); itemizing various classes that I think would be appropriate to include in the Game API ecosystem
	- New GameAPI classes should follow the inheritance/usage patterns laid out in `Patches/Common/GameAPI`
- Global Data Redirection Support
	- Within our existing hook types, there isn't a great option currently for adjusting a global pointer to point to allocated memory that we control
	- This would ideally work similarly to the existing `detour` hook type, but instead of directing to a function via a `JMP` instruction, it'd instead redirect to a runtime-allocated buffer by replacing the reference at the selected address
	- More details would need to be worked out here. Such as do we want to overwrite inline references (may require doing some relative offset math)? Do we need to be concerned about endianness between instruction representations of pointers and data-block representations of pointer
	- An example of where this feature would be particularly useful would be for allocating larger spaces for ARB shaders. As J is often asking for
- Use of relative Jumps for `detour` and `replace` hooks
	- Right now these hooks rely on at least 5 bytes of instruction space to contain the absolute `JMP` instruction they operate on (1 byte for the opcode + 4 bytes for the address)
	- There is potential for reducing this to fewer bytes using a relative or absolute indirect addressing scheme.
	- The benefit would be that this gives us more space to slot in redirection in patches
	- However, this isn't a simple process as the potentially available JMP instructions will vary based on where the target address is relative to the source instruction.
	- Further research needed
- Better kotor 2 support
	- Right now the bulk of our Game API and patch tooling is focused on kotor 1
	- We need to further label/reverse engineer the relevant kotor 2 binaries so that we can have available function addresses and offsets
	- These addresses and offsets need to be added to their relevant Address databases
	- Added support needs to be validated with existing GameAPI frameworks
- Version gating on Game API classes
	- It is very likely that we will end up with some Game API classes that are only relevant to some versions (kotor 1 vs kotor 2)
	- For example, if ever we wrap the "Puppet" functionality in kotor 2, this will have to be version gated, as that system does not exist in kotor 1
	- Furthermore, as the project goes through stages of development, it will not be uncommon for a GameAPI feature to be ready in kotor 1, but not for kotor 2. So version gating would also be useful in this case as well

### PyKotor/HoloPatcher Integrations
A major goal of this project is to integrate it with an existing modding pipeline such as TSL Patcher. That way users will not be required to engage with/learn a new tool that they aren't already familiar with. And these patches can be frictionlessly integrated into existing modding ecosystems.

- [HoloPatcher](https://github.com/OldRepublicDevs/PyKotor/tree/master/Tools/HoloPatcher) can apply patches
	- A new section should be added to the changes.ini format in which `.kpatch` files may be listed
	- HoloPatcher should integrate with KPatchCore as an assembly reference
	- Given the provided `.kpatch` files, HoloPatcher should apply the patches using the `PatchApplicator` system exposed within KPatchCore
- Launching Capabilities
	- In order for our patches to function, the game needs to be run with a special launcher that performs the DLL injection
	- The necessary functions to launch the game with patches are included in the KPatchCore `GameLauncher` class
	- This could be made to be rather headless, by creating just a small portable process that just calls the launcher on the configured executable. Or even just using a proper launcher process with a command line mode, wrapped in a Batch file
	- Discussions should be had about what this should look like to minimize user friction/confusion

### Patch Creation
There have been a variety of patches requested by various modders. users, and community members. These need to be organized into a proper itemized list.

- Refine the current [GitHub Issues](https://github.com/LaneDibello/Kotor-Patch-Manager/issues) backlog
	- Patch requests should be indicated by a label
	- Issues that are done/addressed should be closed
	- Game specific (kotor 1 vs kotor 2) issues should be labeled as such
- Patch feature requirements mapped out
	- Some patches may require features that haven't yet been implemented
	- Some may need specific GameAPI classes in place, systems to be reverse engineered in kotor 2, or specialized hook types (such as the Global Data Redirection mentioned above) in order to accomplish their goals
	- These requirements should be noted on the relevant issues, and separate "blocking" issues be created for these
- Start Making Patches 😈 

## Notes & Acknowledgements
I'd like to acknowledge Wizard, for PyKotor, but also for being some I can talk to about this nonsense.

I'd like to acknowledge the Kotor Modding community for being so enthusiastic about the work I've done so far.

And I'd like to thank my fiance for being far too patient with me