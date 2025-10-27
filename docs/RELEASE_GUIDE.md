# Release Guide

**Script**: `publish.bat`

**Contains**:
- KPatchLauncher.exe (single self-contained executable)
- create-patch.bat (for users to create patches)
- Example patches (.kpatch files) - optional
- README.txt

**Size**: ~60-80 MB (with .NET runtime embedded)

## Steps
- Run the script
- Type the version in #.#.# format
- Select 'y' or 'n' to indicate if the pre-built patches should be included
- Allow the script to finish
- You will find the results in the `releases` directory`
