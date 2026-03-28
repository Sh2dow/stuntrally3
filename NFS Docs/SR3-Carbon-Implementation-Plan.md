# SR3 to Carbon Implementation Plan

## Goal

Do not aim for a literal Carbon clone first.

Aim for this in order:

1. make SR3 feel like Carbon within a track-based structure,
2. add Carbon-specific race/camera/presentation systems,
3. only then decide whether open-world, cops, and full territory gameplay are worth the architectural cost.

That order matches SR3's existing architecture and gives the highest return per hour.

## What SR3 Already Has

SR3 already overlaps with Carbon in more places than expected.

### Camera and driving presentation

Current SR3 camera ownership is already concentrated in:

- `src/game/FollowCamera.h`
- `src/game/FollowCamera.cpp`
- `src/game/Update_Poses.cpp`
- `src/vdrift/cardynamics_update.cpp`

Existing capabilities already include:

- multiple camera modes
- terrain-aware camera tilt
- camera collision avoidance
- speed/FOV response
- loop-specific camera switching

### HUD and race guidance

Current SR3 HUD ownership is already concentrated in:

- `src/game/CHud.cpp`
- `src/game/CHud.h`
- `src/game/Hud_Create.cpp`
- `src/game/Hud_Update.cpp`
- `src/game/Hud_UpdatesAll.cpp`
- `src/road/PaceNotes.*`

Existing capabilities already include:

- minimap
- checkpoint arrow/beam
- racing line / trail
- pacenotes
- speed/RPM gauges
- split-screen-aware HUD logic

### Replay, ghost, boost, rewind

SR3 already has strong arcade support layers in:

- `src/game/Replay.*`
- `src/game/Rewind.cpp`
- `src/game/Update_Poses.cpp`
- `src/vdrift/car.cpp`
- `src/vdrift/cardynamics_update.cpp`

### Rendering and atmosphere

Current SR3 rendering ownership is already concentrated in:

- `src/common/AppGui_Compositor.cpp`
- `src/common/AppGui_SSAO.cpp`
- `src/common/CScene_Sky.cpp`
- `src/common/Atmosphere.*`
- `src/game/CarModel_Create.cpp`
- `src/game/CarModel_Config.cpp`

Existing capabilities already include:

- SSAO
- HDR toggle
- lens flare
- sunbeams
- sky/fog tuning
- reflective materials
- vehicle flares/lights

### Progression and customization

SR3 already has scaffolding for:

- championships
- challenges
- collections
- a lightweight career state
- paint customization

Relevant files:

- `src/game/Championships.cpp`
- `src/game/Challenges.cpp`
- `src/game/Collections.cpp`
- `src/game/data/CareerXml.h`
- `src/game/data/CareerXml.cpp`
- `src/game/PaintsIni.*`
- `src/game/Gui_Paints.cpp`

## Main Gap Versus Carbon

SR3 is feature-rich, but its identity is rally/stunt/sci-fi. Carbon is urban, nocturnal, aggressive, and authored around street-race spectacle.

The missing Carbon-specific layers are:

- a Carbon-style visual direction
- a Carbon-style chase/drift/canyon camera language
- authored race trigger zones like jump cameras, vertigo beats, traffic patterns, and barrier states
- district/territory style progression
- crew, boss, reward, and garage loop depth
- civilian traffic and possibly pursuit logic
- street-racing vehicle roster and track art direction

## IDA-Validated Priority Stack

Live `ida-pro-mcp` type reads confirm these Carbon systems are real and distinct in the shipping game:

- `GCanyonRaceStatus`
- `DALWorldMap`
- `FEWorldMapStateManager`
- `FEWorldMapQuickList`
- `WorldMap`
- `FEngHud`
- `AutoSculpt`
- `FERewardCardManager`
- `FeBustedScreen`
- `WRoadNetwork`
- `TrackInfo`

This suggests the Carbon-conversion priority for SR3 should be:

1. camera and HUD language,
2. world-map style progression shell,
3. canyon event logic,
4. rewards / busted / pursuit-facing FE states,
5. deeper customization.

That priority is better aligned with the actual Carbon architecture than starting with only art swaps or only physics tuning.

## Recommended Phases

## Phase 1: Carbon Presentation Pass

Highest ROI. Do this first.

Target systems:

- lock SR3 to dusk/night-heavy scene presets
- push stronger bloom-like contrast, flares, glare, headlight emphasis, and fog color grading
- make chase camera more aggressive at speed and in drift
- restyle HUD to Carbon composition and typography

Primary SR3 files:

- `src/game/FollowCamera.*`
- `src/game/Hud_*.cpp`
- `src/game/CHud.*`
- `src/common/AppGui_Compositor.cpp`
- `src/common/AppGui_SSAO.cpp`
- `src/common/CScene_Sky.cpp`
- `src/game/CarModel_Create.cpp`

Deliverables:

- one Carbon visual preset pack for urban/night tracks
- one revised chase camera profile
- one revised race HUD theme
- one or two validation tracks that demonstrate the target feel

Carbon asset references already available locally for this phase:

- `D:\Games\NFSC PS3\GLOBAL\HUDTEXRACE.BIN`
- `D:\Games\NFSC PS3\GLOBAL\HUDTEXSPLIT.BIN`
- `D:\Games\NFSC PS3\GLOBAL\HUDTEXTURESPHOTOFINISH.BIN`
- `D:\Games\NFSC PS3\FX\MODULES\tonemap_variants.fx`
- `D:\Games\NFSC PS3\NIS\Scene_FINISHCAMCANYON_BundleB.bun`

## Phase 2: Carbon Track Metadata Layer

Add authored event metadata to SR3 tracks instead of jumping straight to open world.

Carbon-inspired trigger concepts to add:

- jump camera zones
- drift scoring sections
- canyon danger / drop zones
- vertigo camera zones
- traffic route markers
- barrier enable/disable groups
- garage/start/finish presentation markers

Primary SR3 files:

- `src/road/SplineBase.h`
- `src/road/Road_Rebuild.cpp`
- `src/road/Road_Prepass.cpp`
- `src/road/Road_File.cpp`
- `src/road/PaceNotes*.cpp`
- track/editor data loaders under `src/common/data`

Why this phase matters:

- it gives SR3 Carbon-style authored spectacle without requiring a free-roam city first,
- it creates the same kind of trigger vocabulary that `hyperlinked` exposes in `track_path.hpp`.

## Phase 3: Event Mode Layer

Build Carbon-flavored event logic on top of SR3's existing challenge/championship framework.

Good first candidates:

- stronger drift event scoring
- checkpoint/sprint variants with cinematic cameras
- canyon-style duel logic on dedicated tracks
- boss event wrappers around existing race flow

IDA-backed reason to prioritize canyon here:

- `GCanyonRaceStatus` exists as a dedicated game type,
- `hyperlinked` also identifies a `Game_CanyonRaceSetup` site and a `GCanyonRaceStatus::IsIntermissionSync` site,
- this is strong evidence that canyon play is a coherent subsystem and not just a flavored drift race.

Primary SR3 files:

- `src/game/CGame.*`
- `src/game/ChampChall.cpp`
- `src/game/Challenges.cpp`
- `src/game/Championships.cpp`
- `src/game/Update_Poses.cpp`
- `src/vdrift/cardynamics_*`

Implementation note:

- do not wait for full career or open world before adding these modes,
- prove the event feel first on handcrafted tracks.

## Phase 4: Career, Garage, and Territory

SR3 already has a stub career model. Expand that instead of replacing it.

`ProgressCareer` should grow to include:

- owned districts / territory state
- boss ladder progression
- crew members and active perks
- garage list / current safehouse
- reward unlock flags
- car classes aligned to Carbon's structure

Primary SR3 files:

- `src/game/data/CareerXml.h`
- `src/game/data/CareerXml.cpp`
- `src/game/CGui.h`
- `src/game/Gui_Games.cpp`
- `src/game/Gui_InitGames.cpp`
- `src/game/Gui_ProgressLoad.cpp`

Recommended stance:

- implement a territory map front end before attempting free-roam traversal,
- keep progression menu-driven until the authored race loop is solid.

IDA-backed reason to prioritize world-map shell early:

- Carbon has distinct `DALWorldMap`, `FEWorldMapStateManager`, `FEWorldMapQuickList`, and `WorldMap` type entries,
- this indicates the world map is a structural progression layer, not just a menu background.

Asset-backed reason:

- Carbon ships locked/unlocked minimap binaries and `TrackMaps.bin`,
- this strongly suggests territory state and map presentation were central authored assets, not just dynamic overlays.

## Phase 5: Vehicle Identity and Customization

SR3 already supports paint editing, but Carbon identity needs stronger street-car authorship.

Priority order:

- rebalance roster toward tuner/muscle/exotic identities
- build preset visual packages by class
- extend paint into decal/vinyl layering
- leave full Autosculpt-like geometry editing for later

Primary SR3 files:

- `src/game/PaintsIni.*`
- `src/game/Gui_Paints.cpp`
- `src/common/HlmsPbs2.cpp`
- vehicle data under `data/vehicles`

Important scope warning:

- vinyl layering is reasonable,
- full Autosculpt parity is expensive and should not be an early milestone.

IDA-backed reason:

- `AutoSculpt` is confirmed as a real type in the live IDB,
- but the current MCP resource surface does not yet expose enough layout or call-graph detail to justify making it an early milestone.

## Phase 6: Traffic and Pursuit

This is late-stage work, not first-slice work.

Traffic can be introduced earlier on authored tracks, but pursuit should wait until:

- track trigger zones exist,
- event flow exists,
- urban layouts exist,
- camera/HUD presentation already feels Carbon.

Likely work areas:

- new AI/gameplay systems
- route metadata integration from Phase 2
- HUD/FE updates for police state
- additional collision and spawn management

Asset-backed note:

- `FeBustedScreen` exists in the IDB,
- shipped media also includes `seq02_busted` movies and subtitles,
- SR3 can defer full police gameplay, but should still reserve UI/state hooks for busted-style failure presentation if pursuit ever lands.

## What Not To Do First

Avoid these as the opening move:

- full open-world city streaming
- police pursuit system
- full Autosculpt clone
- complete FE recreation before camera and track feel are solved

Those are expensive and low-confidence until the moment-to-moment racing experience already feels right.

## Best First Vertical Slice

Build one polished Carbon-style race slice with:

- one night urban or canyon-inspired track
- one tuned chase camera profile
- one aggressive HUD theme
- one drift or sprint event variant
- one car class presentation package

If that slice works, then scale outward into progression and world structure.

## IDA-Dependent Follow-Ups

Use the current `ida-pro-mcp` session to validate:

1. canyon duel flow and failure rules,
2. camera mover selection rules for drift/canyon/jump states,
3. DAL-backed world map, rewards, and crew logic,
4. traffic/pursuit spawn behavior and zone consumption,
5. FE assembly for race HUD, garage, and territory screens.

## Recommended Immediate Work Order

If implementation starts now, the order should be:

1. Carbon visual preset pack for SR3 rendering and sky/fog.
2. Carbon chase/drift camera rewrite in `FollowCamera`.
3. Carbon HUD layout/theme pass in `CHud` and `Hud_*`.
4. Carbon trigger metadata for tracks and editor.
5. One new Carbon-style event mode on a dedicated track.
6. Expanded `ProgressCareer` and territory front end.

That sequence keeps SR3's architecture intact while moving the game toward Carbon in the shortest path.
