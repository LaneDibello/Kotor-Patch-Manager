# K1AlternatePazaakTurnOrder

## Purpose

A KotOR 1 patch that modifies the original Pazaak minigame behaviour by randomly choosing the starter for the first Pazaak set of a match, then alternating the starter for each subsequent set in that match. This provides a fairer challenge by adopting the alternating turn order found in the sequel, KotOR 2.

## Author

Salk

## Acknowledgements

ChatGPT assisted with development, review, and testing.

ShaeMyName's *Fair Pazaak Turn Order* patch inspired this project.

## Compatibility and testing

The manifest lists these KotOR 1 executable builds:

- GOG 1.03
- CD-crack 1.03
- Steam 1.03

In-game testing has been successful with the original Steam 1.03 executable, the GOG 1.03 executable and the CD-crack 1.03 executable.

## Important

Do not enable this patch together with **Fair Pazaak Turn Order**: both patches modify the hook at `0x00680085`, so only one of the two should be active.

## Source files

- `manifest.toml` — patch identity and supported executable hashes.
- `kotor1.hooks.toml` — runtime hook definitions.
