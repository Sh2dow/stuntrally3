# SR3 to Carbon Sorted Execution Plan

This file keeps the old path name, but the contents are now sorted into an execution order.

## Goal

Make SR3 feel structurally closer to `Need for Speed: Carbon` without blowing up SR3's architecture too early.

The correct order is:

1. handling and race feel,
2. presentation and HUD language,
3. track metadata and track usage,
4. event modes,
5. AI and progression shell,
6. only then large systems like pursuit or open-world streaming.

## Core Rules

- Do not start from open world.
- Do not start from police.
- Do not start from a full FE clone.
- Do not start from raw `TRACKS\\STREAM*.BUN` runtime integration.

Build in this order:

`Car -> Track -> AI -> Race -> Game -> World`

That matches both Black Box-era game feel and the Carbon runtime structure now confirmed through `hyperlinked` and `ida-pro-mcp`.

## Phase 1: Handling Foundation

This is the highest gameplay ROI.

Target:

- move SR3 away from pure sim feel and toward controlled arcade handling

Implementation direction:

- add an arcade control/assist layer above the existing vehicle physics
- use hidden assists instead of trying to get Carbon feel from raw VDrift tuning alone

Priority systems:

1. speed-sensitive steering
2. downforce / high-speed grip hack
3. yaw stabilization
4. brake-to-drift / drift assist
5. nitro as forward force
6. weight transfer / front-rear grip shaping
7. air control

Important non-goal:

- do not rewrite Bullet first

SR3 work areas:

- `src/vdrift/car.cpp`
- `src/vdrift/cardynamics_update.cpp`
- `src/game/Update_Poses.cpp`
- `src/game/CGame.*`

## Phase 2: Presentation Foundation - Done

Once handling starts feeling right, align the screen language with Carbon.

Target:

- darker, glossier, more aggressive Carbon presentation

Priority systems:

- chase/drift/canyon camera behavior
- HUD layout and visual language
- post-processing, glare, bloom, night fog, headlight emphasis
- real HDR luminance path instead of the current dummy placeholder

SR3 work areas:

- `src/game/FollowCamera.*`
- `src/game/CHud.*`
- `src/game/Hud_*.cpp`
- `src/common/AppGui_Compositor.cpp`
- `src/common/AppGui_SSAO.cpp`
- `src/common/CScene_Sky.cpp`

See:

- `NFS Docs/SR3-Carbon-HDR-Luminance-Fix-Plan.md`

## Phase 3: Track Usage Foundation

This phase changed significantly after the Carbon track research.

Carbon track usage is not one big "load a track bundle" operation. It is split into:

1. FE track metadata and previews,
2. track-path gameplay zones and barriers,
3. runtime streamed sections and road network.

So SR3 should use Carbon track data in this order:

1. FE metadata and minimaps first,
2. path/zones/barriers second,
3. full runtime section streaming much later.

Priority systems:

- `TrackMaps.bin` and FE preview assets
- locked/unlocked minimaps
- track display name / track art / event ID / engage position / region
- Carbon zone vocabulary:
  - `jump_camera`
  - `canyon_drop`
  - `vertigo_camera`
  - `traffic_pattern`
  - `garage`
  - barrier group enable/disable

SR3 work areas:

- `src/game/data/CareerXml.*`
- `src/game/Gui_Games.cpp`
- `src/game/Gui_InitGames.cpp`
- `src/game/Gui_ProgressLoad.cpp`
- `src/road/SplineBase.h`
- `src/road/Road_File.cpp`
- `src/road/Road_Prepass.cpp`
- `src/road/Road_Rebuild.cpp`

See:

- `NFS Docs/NFSC-Track-Usage-Research.md`

## Phase 4: Event Mode Layer

After handling and track metadata exist, event logic becomes much easier to build cleanly.

Priority systems:

- sprint / circuit wrappers with stronger Carbon presentation
- drift event scoring and combo logic
- canyon duel prototype
- boss-race wrappers

Good first event target:

- one canyon-style duel on a handcrafted validation track

Why:

- `GCanyonRaceStatus` is confirmed in the live IDB
- Carbon track-path zone types already expose useful canyon-style trigger vocabulary

SR3 work areas:

- `src/game/CGame.*`
- `src/game/ChampChall.cpp`
- `src/game/Challenges.cpp`
- `src/game/Championships.cpp`
- `src/game/Update_Poses.cpp`

## Phase 5: Racing AI and Traffic Foundation

Do this after tracks and event rules are stable.

Priority systems:

- racing line spline data
- AI look-ahead steering and target-speed logic
- drift event AI behavior
- traffic routes on authored tracks

Why now:

- racing AI depends on track data
- traffic is more useful once Carbon route metadata exists

SR3 work areas:

- AI update systems
- track/editor metadata
- race event controllers

## Phase 6: Career and World Map Shell

This is where the FE track metadata work pays off.

Priority systems:

- district ownership state
- boss progression
- menu-driven territory map shell
- reward flags
- track preview integration

Important rule:

- keep it menu-driven first
- do not block on free-roam traversal

SR3 work areas:

- `src/game/data/CareerXml.*`
- `src/game/Gui_Games.cpp`
- `src/game/Gui_InitGames.cpp`
- `src/game/Gui_ProgressLoad.cpp`

## Phase 7: Pursuit and Failure States

Pursuit is late-stage.

Priority systems:

- busted state plumbing
- pursuit FE state hooks
- police-related event wrappers
- roadblock/route metadata use

Why late:

- depends on route metadata
- depends on AI work
- depends on presentation and FE shell

## Phase 8: Vehicle Identity and Customization

Customization should follow the core race loop, not precede it.

Priority systems:

- tuner / muscle / exotic handling profiles
- class-specific camera / HUD / presentation flavor
- vinyl/decal layering
- visual package presets

Defer:

- full Autosculpt parity

## Phase 9: Optional Full Carbon Streaming / Open World

This is the last major phase, not the opening move.

Only consider it after:

- SR3 already has Carbon-feeling handling,
- Carbon-styled events,
- track metadata and FE previews,
- AI and progression shell,
- stable route/barrier vocabulary.

Why last:

- Carbon uses `TrackStreamer`, visible sections, group toggling, and `WRoadNetwork`
- that is a large architectural commitment
- the research now shows it is not necessary for the first real Carbon-feeling slice

## Best First Vertical Slice

Build one polished playable slice with:

- one night urban or canyon-inspired track
- one tuned arcade handling preset
- one Carbon chase/drift camera profile
- one aggressive race HUD theme
- one Carbon-style sprint or canyon duel
- one FE preview/minimap entry

If that slice works, expand outward.

## Immediate Work Order

1. handling foundation
2. presentation foundation
3. track metadata and zone vocabulary
4. one event prototype
5. racing line / AI basics
6. world map shell
7. traffic / pursuit hooks
8. customization depth
9. optional full streaming/open world

## Canonical Supporting Docs

- `NFS Docs/SR3-Carbon-Implementation-Plan.md`
- `NFS Docs/SR3-Carbon-Backlog.md`
- `NFS Docs/NFSC-Track-Usage-Research.md`
- `NFS Docs/SR3-Carbon-HDR-Luminance-Fix-Plan.md`
