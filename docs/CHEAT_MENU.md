# Cheat-menu build (DuckyNerdball fork, branch `cheat-menu`)

An unofficial fork of [KraftMacAndChee/Mercenaries-Recompiled](https://github.com/KraftMacAndChee/Mercenaries-Recompiled)
that adds a cheat menu, quick airstrikes, a "warzone" spawner and a few image options
to the developer (F9) panel. Like upstream, **this repository contains source code only**.
No disc image, game data, generated code or game executable is included; you build it
from your own retail Xbox copy exactly as described in the main [README](../README.md).

Base: upstream `f513ae4` ("Added DLL Plugin System").

## Installer

`installer/Install.cmd` (also attached to each release) builds and installs everything from the player's own
ISO: prerequisites, source download, the official `Build-From-Iso.ps1`, install to `%USERPROFILE%\Games\Mercenaries Cheat Build\Game`,
F9 panel on, and the **All Supports** / **Clear Skies** mods generated from the player's own game files
(enabled, normal save slots), and 1920x1080 borderless unless the player already picked a resolution or display mode.
`Install.cmd -Update` rebuilds from the latest source, keeps saves and settings, and re-ticks both mods.
`Install.cmd -SkipBuild` reinstalls from an existing build without rebuilding.

## Turning it on

- Set `developer_menu=1` in `developer.ini` next to the game.
- **F9** opens the developer panel. On a controller, **L3 + R3** opens and closes it.
- Optional: `logging=1` writes preview logs. Modded launches put them in
  `mod-cache\<fingerprint>\runtime\preview-logs\`, and each launch overwrites the last one.

### Controller navigation in the panel

| Input | Action |
|---|---|
| L3 + R3 | Open / close the panel |
| B | Close |
| LB / RB | Previous / next page |
| D-pad | Move focus / change the focused value |
| A | Activate (on a list: Spawn card / Call strike / Start the war) |

## Cheats page

| Feature | What it does |
|---|---|
| Cheat effects | Calls the game's own PDA cheat routines directly: god mode, infinite ammo, +$1,000,000, unlock all shop items, reset faction moods, unlock all cheat codes, and 15 outfits (Prisoner, NK/SK Elite, Ace of Diamonds, UN Doctor, Mafia Heavy, card NPCs, Allied Pilot, faction bosses, Josef, Han Solo, Indiana Jones). |
| Drops | Spawns pickups next to you: health, armor, ammo packs, grenades, money, every firearm, the three cheat weapons, C4 + detonator, mafia bribe. "Drop all weapons" drops the whole set. |
| Airstrikes | Calls any strike at your crosshair, on foot, driving or flying (see below). |
| Deck of 52 | Loads a numbered card's camp (Clubs 3–10, Diamonds/Hearts/Spades 2–10), or every card camp in the current province with "Spawn all here". Cards appear when you get near their camp. Face cards and Aces come from contracts (Missions page). |
| Faction buttons | Make every faction friendly or hostile (retail relationship setter). |
| Infinite money | Keeps your balance topped up to $9,999,999. |
| NPC density | Normal / Double / Triple traffic and civilian density in every traffic zone (multiplies the values the mission scripts set). |
| Sharpening | Off / Low / Medium / High contrast-adaptive sharpening on the final image. |
| Wake-up range | x1 / x1.5 / x2 / x3 / x4. Multiplies every world object's authored wake-up and hibernation distance, so NPCs, vehicles and camps become active further out. x2 is about 4x as many live objects; x3 and x4 are heavy. Only affects areas that are already streamed in; it does not extend terrain or draw distance. |
| Warzone | Spawns hostile, fully crewed units at your crosshair (at least ~60 m away): Tank field (4 T-62, 2 T-54, 2 ZSU-57), Armored assault (T-54s, BMPs, BTRs, SA-8, M1985 + infantry), Infantry company (30 NK), Chinese armor column (hostile only if China is), Fuel depot (12 explosive fuel tanks), All-out war (tank field + infantry + fuel depot). |
| Ambush waves | Every ~45 s a random hostile NK group spawns 120–160 m away and attacks; the HUD says which direction. |

## Airstrikes anywhere

Strikes use the same beacon the game creates when you throw a support smoke grenade, so the
real attack run happens. Aim is your crosshair (in Free Cam: the centre of the free camera's view,
up to 1.5 km). Aiming at the sky drops it ~150 m ahead.

| Input | Action |
|---|---|
| Hold LB + click R3 | Fire the selected strike at the crosshair |
| Hold LB + D-pad left/right | Choose the strike (shown on the HUD) |
| F7 | Fire (keyboard) |
| F9 → Cheats → Call strike | Same, from the panel (shares the selection) |

While LB is held it is hidden from the game, so the chord never switches grenades or ejects
passengers; a normal LB tap still reaches the game on release.

Strike list (23):
- **Retail:** Cluster Bomb (A-10), Tank Buster (A-10), Carpet Bomb (B-52), Bunker Buster (F-15),
  Surgical Strike (Tornado), Laser-Guided Missile (EF2000), Stealth Fighter (F-117),
  Stealth Bomber (B-2), Air Superiority (F-22), Gunship 1/2/3 (AC-130, 10/20/45 s),
  Cruise Missile, Fuel-Air Bomb (Su-25), Strategic Missile (FROG), Chinese Artillery Barrage 1/2/3 (30/45/60 m).
- **DEV:** Killbox (F-22) and Killbox (NK Su-25) — aircraft armed with an unused "killbox" weapon.
- **CUT:** Airburst (China "66air"), Air Superiority on an F-15 body ("F2"), alternate Tornado.

Not included: the SK "supportgunship" support item references an aircraft that doesn't exist in
the game data, so it can't be called.

There is no cooldown. Firing many heavy bombers within a second or two can crash the game.

## Free Cam

With Free Cam on, strikes and warzones target the free camera's view, drops land just in front of
the camera, and the vehicle/troop spawner finds ground up to 2 km below the camera (was 300 m).

## Recomp Options menu

- **Brightness** slider (50–150, default 100), saved as `Brightness` in the options INI. Applied as
  a gamma pass on the final image, previews live while you move the slider.
- **Sharpening** level is saved as `Sharpen`.

## Test status

| Feature | Status |
|---|---|
| F9 Cheats page, controller open/close and navigation | Tested, works |
| Outfits (incl. Han Solo), shop unlock, cheat-code unlock | Tested, works |
| Vehicle spawning (tanks) | Tested, works |
| Sharpening (High) | Tested, works |
| Quick strikes: LB+R3, LB+D-pad cycling, F7, HUD messages | Tested, works |
| Beacon airstrikes (B-52, F-15, Tornado, others) | Tested, works |
| Warzone "All-out war" | Tested, works: 11 of 13 groups spawned plus 12 fuel tanks; about 18 FPS while it all fights |
| Deck of 52 card spawning | Tested: cards appear at their camps when you get near |
| Brightness slider | Built, not confirmed in game |
| Infinite money, NPC density | Built, not confirmed in game |
| Ambush waves | Built, not tested |
| Gunship 2/3, artillery barrages, DEV and CUT strikes | Built, not tested |
| Free Cam strikes/drops/spawns, wake-up range | Built, not tested |

## Known issues and limits

- Several heavy bomber strikes in quick succession can crash the game.
- Large warzones drop the frame rate hard (the engine was built for the original Xbox's object budget).
- Draw distance is not extended: the terrain cache has a fixed 64-entry budget.
- New buildings, bases or vehicle bays can't be spawned; buildings are part of each region's map data.
- Real-time (no cutscene) loading of captured card NPCs onto the extraction chopper is not done.

## Free shop (optional, local data mod)

The PDA shop treats a price of 0 as "not for sale", so a free shop needs every `DefaultPrice` in
`DATAPOD\itemshop.ini` set to **1**, not 0. Make that edit in a mod folder from your own game files
(`mods\<name>\game_files\DATAxbox\DATAPOD\itemshop.ini`); game data is not distributed here.

## What changed (files)

| File | Change |
|---|---|
| `ports/mercenaries/src/cheat_menu.c/.h` (new) | Cheat, drop, card, strike and warzone catalogues; action queue; toggles (money, density, ambush, wake-up range, quick strike) |
| `ports/mercenaries/src/dev_cheats_guest.h` (new) | Game-thread side: effects, drops, cards, Lua calls, crosshair/Free Cam aim, beacon strikes, warzone, ambush waves, pad chord filter, HUD messages |
| `ports/mercenaries/src/dev_menu.c` | Cheats page UI, controller navigation timer |
| `ports/mercenaries/src/dev_spawn.h` | Warzone spawn queue with explicit origin/facing; cheat tick; longer Free Cam ground search |
| `ports/mercenaries/src/recomp_manual.c` | Includes the cheat code; pad chord hook; brightness row painter |
| `ports/mercenaries/src/freecam_streaming.h` | Wake-up range also applies to the Free Cam observer |
| `ports/mercenaries/scripts/Patch-Generated.py` | Hooks the two world wake-up / hibernation distance checks (0x1EC2A0, 0x1EC320) |
| `ports/mercenaries/src/recomp_options*.c/.h` | Brightness and sharpening options |
| `ports/mercenaries/src/main.c` | Registers the present-time brightness/sharpen callbacks |
| `src/d3d/d3d8_device.c`, `d3d8_internal.h` | Final-image gamma + CAS sharpening pass |
| `ports/mercenaries/CMakeLists.txt` | Adds `cheat_menu.c` |
| `tools/recomp/test_mercenaries_recomp_options.py` | Options count is now 16 |

Note: `Patch-Generated.py` changes only take effect on a fresh lift (`Recompile.ps1` /
`Build-From-Iso.ps1`). An existing generated tree has to be patched separately.
