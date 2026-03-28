# SR3 Carbon Backlog

This is the execution-facing companion to the higher-level plan.

## Slice 1: Presentation

- Rework chase camera behavior in `src/game/FollowCamera.cpp` toward stronger speed compression, lateral drift emphasis, and more cinematic look-ahead.
- Audit SR3 HUD layout in `src/game/CHud.*`, `src/game/Hud_Create.cpp`, and `src/game/Hud_Update.cpp` against Carbon's race HUD texture packs and photo-finish assets.
- Build one Carbon-inspired post-processing preset in `src/common/AppGui_Compositor.cpp`, `src/common/AppGui_SSAO.cpp`, and `src/common/CScene_Sky.cpp`.
- Use Carbon references:
  - IDA type: `FEngHud`
  - game assets: `GLOBAL/HUDTEXRACE.BIN`, `GLOBAL/HUDTEXSPLIT.BIN`, `GLOBAL/HUDTEXTURESPHOTOFINISH.BIN`
  - game assets: `FX/MODULES/tonemap_variants.fx`

## Slice 2: Canyon Event Prototype

- Add an SR3 event wrapper for a canyon-style duel in `src/game/CGame.*`, `src/game/ChampChall.cpp`, and `src/game/Update_Poses.cpp`.
- Add track metadata for canyon danger zones, cinematic trigger zones, and fail states under `src/road/*`.
- Create one dedicated canyon validation track.
- Use Carbon references:
  - IDA type: `GCanyonRaceStatus`
  - repo call sites: `0x00616DB0`, `0x00656C10`
  - game assets: `MOVIES/canyon_*.vp6`, `NIS/Scene_FINISHCAMCANYON_BundleB.bun`

## Slice 3: World Map Shell

- Expand `ProgressCareer` in `src/game/data/CareerXml.*` with district ownership, boss progression, and reward flags.
- Add an SR3 front-end shell for district selection in `src/game/Gui_Games.cpp`, `src/game/Gui_InitGames.cpp`, and `src/game/Gui_ProgressLoad.cpp`.
- Keep it menu-driven first; do not block on free-roam.
- Use Carbon references:
  - IDA types: `DALWorldMap`, `FEWorldMapStateManager`, `FEWorldMapQuickList`, `WorldMap`
  - repo call site: `0x005C3811`
  - game assets: `TRACKS/L5RA/MINI_MAP_CASINOTOWN_LOCKED.bin`, `TRACKS/L5RA/MINI_MAP_CASINOTOWN_UNLOCKED.bin`, `TRACKS/L5RA/TrackMaps.bin`

## Slice 4: Crew and Rewards

- Extend career save state with crew members, role/perk selection, and reward unlocks.
- Add placeholder UI states for crew presentation and reward cards before implementing full logic depth.
- Use Carbon references:
  - IDA types: `FECrewLogoLoader`, `FERewardCardManager`
  - repo header: `nfsc-sdk/include/GCareer.h`
  - game assets: `MOVIES/crew_*.vp6`, `SUBTITLES/Crew_*.sub`

## Slice 5: Customization

- Add a first vinyl/decal system on top of SR3 paint support in `src/game/PaintsIni.*`, `src/game/Gui_Paints.cpp`, and `src/common/HlmsPbs2.cpp`.
- Defer full geometry morphing until the rest of the Carbon loop is established.
- Use Carbon references:
  - IDA type: `AutoSculpt`
  - IDA structs: `MissingPartInfo`, `CarMemoryInfo`

## Slice 6: Pursuit and Failure States

- Reserve UI/state plumbing for busted and pursuit results, even if police AI is deferred.
- Introduce authored traffic routes before any full pursuit system.
- Use Carbon references:
  - IDA types: `FeBustedScreen`, `WRoadNetwork`
  - game assets: `MOVIES/seq02_busted_english_ntsc.vp6`, `SUBTITLES/Seq02_Busted.sub`

## Immediate Order

1. Presentation slice.
2. Canyon prototype slice.
3. World map shell.
4. Crew and rewards.
5. Vinyl customization.
6. Traffic and pursuit.
