# Mercenaries Recompiled — Cheat-Menu Build

A fork of [KraftMacAndChee/Mercenaries-Recompiled](https://github.com/KraftMacAndChee/Mercenaries-Recompiled),
the native Windows port of **Mercenaries: Playground of Destruction** (Xbox), with a full cheat
menu, airstrikes you can call anywhere, a warzone spawner and extra image options.

**Source only. Bring your own game.** This repository contains no disc image, extracted game
data, generated code or game executable. You build it from your own USA retail Xbox copy.

Full feature list, controls, test status and changed files: **[docs/CHEAT_MENU.md](docs/CHEAT_MENU.md)**

## What it adds

- **Cheats page** in the F9 developer panel: god mode, infinite ammo, money, shop and
  cheat-code unlocks, 15 outfits (including Han Solo and Indiana Jones), weapon and pickup
  drops, factions friendly/hostile, infinite money, NPC density x1–x3.
- **Airstrikes anywhere**: 23 strikes at your crosshair, on foot, driving or flying. Includes
  Gunship 2/3, the Chinese artillery barrages, two unused dev "killbox" strikes and three cut aircraft.
- **Warzone**: hostile tank fields, armored assaults, infantry companies and explosive fuel
  depots at your crosshair, plus optional ambush waves.
- **Deck of 52**: load any numbered card's camp on demand.
- **Wake-up range** x1–x4: NPCs, vehicles and camps become active further out.
- **Free Cam**: strikes, warzones and spawns follow the free camera.
- **Recomp Options**: brightness slider and sharpening (Off / Low / Medium / High).

## Controls

| Input | Action |
|---|---|
| F9, or L3 + R3 on a controller | Open / close the developer panel |
| LB / RB, D-pad, A, B | Change page, move / change value, activate, close |
| Hold LB + click R3, or F7 | Call the selected airstrike at the crosshair |
| Hold LB + D-pad left/right | Choose the airstrike (shown on the HUD) |

The panel needs `developer_menu=1` in `developer.ini` next to the game.

## Install (easiest)

Download **MercenariesCheatBuild-Installer** from [Releases](https://github.com/DuckyNerdball/Mercenaries-Recompiled/releases),
extract it and run `Install.cmd`. It asks for your Xbox ISO, installs Python 3.12 and the Visual Studio 2022
Build Tools if needed (several GB), builds the game on your PC from your own disc (20–40 minutes), and puts a
shortcut on your desktop. The download contains only the installer script: no game code or data.

## Build from source

Prerequisites:

- Python **3.12** (Windows `py` launcher)
- Visual Studio **2022** with the C++ desktop tools, MSVC **14.44.35207** and Windows SDK **10.0.26100.0**
- PowerShell 7 or Windows PowerShell 5.1
- Your **USA retail Xbox Mercenaries ISO** (`default.xbe` SHA-256 `AA08EA21D952AC35F49C02C7E2ED08AA25AD7535FBBBCCC95636775F37BE99D7`)

```powershell
git clone -b cheat-menu https://github.com/DuckyNerdball/Mercenaries-Recompiled.git
cd Mercenaries-Recompiled
& .\ports\mercenaries\scripts\Build-From-Iso.ps1 -IsoPath 'D:\Games\Mercenaries.iso'
```

Output is in `build/mercenaries/bin/Release/`. Run `Mercenaries Recompiled.exe` and keep the
DLLs and folders next to it. See the [port guide](ports/mercenaries/README.md) for details.

**Already have the prebuilt release installed?** Close the game and copy `mercenaries_recomp.exe`
and `Mercenaries Recompiled.exe` from the build output over the ones in your install (back them up
first). Your saves, settings and mods stay as they are.

## Known limits

- Several heavy bomber strikes fired within a second or two can crash the game.
- Big warzones drop the frame rate hard; the engine was built for the original Xbox's object budget.
- Draw distance is not extended, and new buildings can't be added (they are part of each region's map data).

Bugs in this fork: open an issue here. Bugs that also happen on the unmodified port belong
[upstream](https://github.com/KraftMacAndChee/Mercenaries-Recompiled/issues). Don't attach disc
images, game data or memory dumps to a public issue.

## Credits and licensing

Mercenaries Recompiled is by Trenton Turner (KraftMacAndChee), building on sp00nz's
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp) toolkit and work from the Xbox emulation
and modding communities. The cheat-menu additions are by DuckyNerdball. This is an unofficial fan
project, not an official game release. Mercenaries and its game assets belong to their respective owners.

The root [MIT license](LICENSE) retains the toolkit's copyright notice and does not replace imported
components' licenses. xemu-derived code carries per-file LGPL notices; SDL2, DSP56300, Lua, fonts and
other dependencies keep their own licenses, and the bundled Windows SDK shader compiler has separate
redistribution terms. See the [dependency inventory](docs/dependencies.md) and notice files before
redistributing a modified package. Maintainer-supplied artwork, controller prompts and optional upgrade
resources are retained and gain no new rights under the root MIT license.

Upstream documentation is unchanged: [Architecture](ARCHITECTURE.md) · [Code map](CODE_MAP.md) ·
[Documentation index](docs/README.md) · [Contributing](CONTRIBUTING.md)
