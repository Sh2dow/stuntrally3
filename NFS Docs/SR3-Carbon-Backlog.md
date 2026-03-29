# SR3 Carbon Backlog

This is the execution-facing companion to the higher-level plan.

## Slice 1: Handling Foundation

- Add a Carbon-oriented control/assist layer above the existing vehicle baseline in `src/vdrift/car.cpp`, `src/vdrift/cardynamics_update.cpp`, `src/game/Update_Poses.cpp`, and `src/game/CGame.*`.
- Implement first-order arcade handling systems:
  - speed-sensitive steering
  - downforce / high-speed grip shaping
  - yaw stabilization
  - brake-to-drift behavior
  - nitro as forward force
- Create one tuned handling preset and one drift-capable preset for validation.

## Slice 2: Presentation

- Rework chase camera behavior in `src/game/FollowCamera.cpp` toward stronger speed compression, lateral drift emphasis, and more cinematic look-ahead.
- Audit SR3 HUD layout in `src/game/CHud.*`, `src/game/Hud_Create.cpp`, and `src/game/Hud_Update.cpp` against Carbon's race HUD texture packs and photo-finish assets.
- Build one Carbon-inspired post-processing preset in `src/common/AppGui_Compositor.cpp`, `src/common/AppGui_SSAO.cpp`, and `src/common/CScene_Sky.cpp`.
- Replace the temporary HDR luminance placeholder with a real luminance / auto-exposure chain before treating HDR grading as final.
- Use Carbon references:
  - IDA type: `FEngHud`
  - game assets: `GLOBAL/HUDTEXRACE.BIN`, `GLOBAL/HUDTEXSPLIT.BIN`, `GLOBAL/HUDTEXTURESPHOTOFINISH.BIN`
  - game assets: `FX/MODULES/tonemap_variants.fx`
- See also:
  - `NFS Docs/SR3-Carbon-HDR-Luminance-Fix-Plan.md`

## Slice 3: Track Metadata and Preview

- Add FE track metadata support for track display name, art name, event ID, region, minimap preview, and engage position.
- Add SR3-side authored zone and barrier vocabulary based on Carbon track-path data:
  - `jump_camera`
  - `canyon_drop`
  - `vertigo_camera`
  - `traffic_pattern`
  - `garage`
  - barrier group enable/disable
- Keep full `STREAM*.BUN` runtime support out of scope for this slice.
- Use Carbon references:
  - IDA systems: `TrackInfo`, `DALWorldMap`, `UITrackMapStreamer`, `TrackStreamer`, `WRoadNetwork`
  - game assets: `TRACKS/L5RA/TrackMaps.bin`, `TRACKS/L5RA/MINI_MAP_CASINOTOWN_LOCKED.bin`, `TRACKS/L5RA/MINI_MAP_CASINOTOWN_UNLOCKED.bin`
  - repo source: `hyperlinked/src/hyperlib/streamer/track_path.hpp`
- See also:
  - `NFS Docs/NFSC-Track-Usage-Research.md`

## Slice 4: Canyon Event Prototype

- Add an SR3 event wrapper for a canyon-style duel in `src/game/CGame.*`, `src/game/ChampChall.cpp`, and `src/game/Update_Poses.cpp`.
- Add track metadata for canyon danger zones, cinematic trigger zones, and fail states under `src/road/*`.
- Create one dedicated canyon validation track.
- Use Carbon references:
  - IDA type: `GCanyonRaceStatus`
  - repo call sites: `0x00616DB0`, `0x00656C10`
  - game assets: `MOVIES/canyon_*.vp6`, `NIS/Scene_FINISHCAMCANYON_BundleB.bun`

## Slice 5: World Map Shell

- Expand `ProgressCareer` in `src/game/data/CareerXml.*` with district ownership, boss progression, and reward flags.
- Add an SR3 front-end shell for district selection in `src/game/Gui_Games.cpp`, `src/game/Gui_InitGames.cpp`, and `src/game/Gui_ProgressLoad.cpp`.
- Keep it menu-driven first; do not block on free-roam.
- Use Carbon references:
  - IDA types: `DALWorldMap`, `FEWorldMapStateManager`, `FEWorldMapQuickList`, `WorldMap`
  - repo call site: `0x005C3811`
  - game assets: `TRACKS/L5RA/MINI_MAP_CASINOTOWN_LOCKED.bin`, `TRACKS/L5RA/MINI_MAP_CASINOTOWN_UNLOCKED.bin`, `TRACKS/L5RA/TrackMaps.bin`

## Slice 6: AI, Traffic, and Pursuit Hooks

- Add racing line / spline-guided AI support before full police gameplay.
- Introduce authored traffic routes using the same track metadata vocabulary added earlier.
- Reserve busted / pursuit UI-state plumbing before implementing full pursuit logic.
- Use Carbon references:
  - IDA types: `WRoadNetwork`, `FeBustedScreen`
  - game assets: `MOVIES/seq02_busted_english_ntsc.vp6`, `SUBTITLES/Seq02_Busted.sub`

## Slice 7: Crew and Rewards

- Extend career save state with crew members, role/perk selection, and reward unlocks.
- Add placeholder UI states for crew presentation and reward cards before implementing full logic depth.
- Use Carbon references:
  - IDA types: `FECrewLogoLoader`, `FERewardCardManager`
  - repo header: `nfsc-sdk/include/GCareer.h`
  - game assets: `MOVIES/crew_*.vp6`, `SUBTITLES/Crew_*.sub`

## Slice 8: Customization

- Add a first vinyl/decal system on top of SR3 paint support in `src/game/PaintsIni.*`, `src/game/Gui_Paints.cpp`, and `src/common/HlmsPbs2.cpp`.
- Defer full geometry morphing until the rest of the Carbon loop is established.
- Use Carbon references:
  - IDA type: `AutoSculpt`
  - IDA structs: `MissingPartInfo`, `CarMemoryInfo`

## Slice 9: Optional Full Carbon Streaming

- Treat direct `TRACKS\\STREAM*.BUN` runtime support as a late architectural milestone, not an opening task.
- Only approach this after SR3 has FE track previews, track zones/barriers, event flow, AI, and progression shell.

## Immediate Order

1. Handling foundation.
2. Presentation slice.
3. Track metadata and preview slice.
4. Canyon prototype slice.
5. World map shell.
6. AI / traffic / pursuit hooks.
7. Crew and rewards.
8. Vinyl customization.
9. Optional full streaming.
