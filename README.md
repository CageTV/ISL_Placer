# ISL Placer

An in-game light editor for **Light Placer** lights and **Inverse Square Lighting** (Community Shaders). Look at a light, change it live, and write the result back as Light Placer json.
It works with ISL Helper SKSE, it does not replace it. (It started as the light editor of LoS2 Control, without the lantern schedule.)

Skyrim SE, AE (1.5.97 to 1.7.x) and VR. The installer offers two DLLs: the current one for 1.6.1170 and newer, and the older one for VR and 1.6.1130 and below. The VR build compiles but has not been tested in a headset.

## What it does
- **Pick** the light you are looking at: any light placed by Light Placer, converted by ISL Helper or placed by the game (lanterns, forge fires, nirnroot, torches). Pick the object you look at even if it has no light.
- **Edit live:** radius, size, cutoff, fade, colour and the Community Shaders flags (inverse square, linear, shadow, portal strict, ignore scale, no external emittance). Optionally on every loaded copy of the object. Revert puts the values back.
- **See where it comes from:** lists the Light Placer json entries that define the object, and warns when two entries light the same node (two bulbs).
- **Write it as Light Placer json:** saves your values to `Data/LightPlacer/ISLPlacer/ISLPlacer_Patches.json`: always on, outdoors only, indoors only, at night (hours you choose), or only while a global variable of your choice is 1.
- **Turn off for this object:** conflict resolution. Takes the object out of another json entry in a rewritten copy of the file. The first version of every changed file is kept in `Data/SKSE/Plugins/ISLPlacer/backups` and "Put back" restores it.
- **Add a light to this object:** for models that have no light. Attach to a node or a point, choose a base light record, save.

Light Placer reads its files once at game start, so json written from the editor shows after a restart (live edits of existing lights are immediate). ISL Helper skips any light that a Light Placer file names, so lights written here are left alone by it.

## Requirements
- Skyrim SE / AE or VR, SKSE, Address Library
- [Light Placer](https://www.nexusmods.com/skyrimspecialedition/mods/127557)
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (the menu)
- powerofthree's Tweaks (EditorID lookup)
- Community Shaders with Inverse Square Lighting; ISL Helper SKSE if you want the vanilla lights converted

## Install
Install the download with your mod manager (FOMOD): choose the current DLL or the older one (VR, 1.6.1130 and below); it is pre-selected from your game version.
Open the SKSE Menu Framework menu and look for "ISL Placer".

## Building
SE/AE: alandtse's CommonLibSSE-NG, `ng.cmake` describes the plugin. VR and older: CharmedBaryon's CommonLibSSE-NG through vcpkg, `plugin/build.cmd`. Both build the same `src/`.

## License
MIT for the files written here; the DLL built with alandtse's CommonLibSSE-NG is a combined work under the GPL (the GPL text is in `LICENSE`). See `LICENSE` and `NOTICE`.
