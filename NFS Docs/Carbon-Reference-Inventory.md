# Carbon Reference Inventory

## Scope

Reference repos and data sources inspected locally:

- `D:\Repos\Games\NFSC\hyperlinked`
- `D:\Repos\Games\NFSC\nfsc-sdk`
- `D:\Games\NFSC PS3`

Goal of this document:

- identify which Carbon systems are already represented in the reference repos,
- separate proven reverse-engineered coverage from assumptions,
- define what still needs live IDA access.

## IDA Pro MCP State

As of March 27, 2026, `ida-pro-mcp` resources are working in this Codex session.

Live IDB metadata from `ida://idb/metadata`:

- IDB: `D:\Development\Debug_symbols\IDB_PC\NFSCarbon-v1.4\NFSCarbon-v1.4.i64`
- module: `NFSC.exe`
- base: `0x400000`
- image size: `0x82b34e`

Live segment layout from `ida://idb/segments`:

- `.text` at `0x401000` - `0x9c1000`
- `.rdata` at `0x9c1000` - `0xa4a000`
- `.data` at `0xa4a000` - `0xbce000`

Live entry point from `ida://idb/entrypoints`:

- `start` at `0x87e926`

This document is no longer only source-backed. It now includes initial IDA-backed type information as well.

## Game Resource Source

`D:\Games\NFSC PS3` contains useful full-game data buckets for future extraction:

- `CARS`
- `FRONTEND`
- `FX`
- `GLOBAL`
- `LANGUAGES`
- `MOVIES`
- `NIS`
- `SOUND`
- `TRACKS`
- `NFSC.exe`

Use this path for:

- UI art and HUD reference mining
- shader/effect asset mining
- vehicle/track content discovery
- event/frontend package naming cross-checks

Initial filename-level evidence from `D:\Games\NFSC PS3` already confirms several Carbon-specific systems:

- HUD textures:
  - `GLOBAL\HUDTEXRACE.BIN`
  - `GLOBAL\HUDTEXSPLIT.BIN`
  - `GLOBAL\HUDTEXTURESPHOTOFINISH.BIN`
- tone mapping / presentation:
  - `FX\MODULES\tonemap_variants.fx`
- canyon media:
  - multiple `MOVIES\canyon_*.vp6`
  - `MOVIES\tutorial_canyon_english_ntsc.vp6`
  - `NIS\Scene_FINISHCAMCANYON_BundleB.bun`
  - `SOUND\MIXMAPS\mapoutputcanyon.*`
- crew media:
  - `MOVIES\crew_colin_*`
  - `MOVIES\crew_neville_*`
  - `MOVIES\crew_nikki_*`
  - `MOVIES\crew_sal_*`
  - `MOVIES\crew_samson_*`
  - `MOVIES\crew_yumi_*`
  - matching `SUBTITLES\Crew_*.sub`
- busted flow:
  - `MOVIES\seq02_busted_english_ntsc.vp6`
  - `SUBTITLES\Seq02_Busted.sub`
- district / world-map style assets:
  - `TRACKS\L5RA\MINI_MAP_CASINOTOWN_LOCKED.bin`
  - `TRACKS\L5RA\MINI_MAP_CASINOTOWN_UNLOCKED.bin`
  - `TRACKS\L5RA\TrackMaps.bin`

Takeaway:

- crew, busted, canyon, photo-finish, and territory/minimap states are all visible in shipped assets,
- the Carbon identity is backed by both runtime code and packaged content,
- SR3 should plan against both layers, not only against gameplay code.

## `hyperlinked` Coverage

`hyperlinked` is not a general Carbon design doc. It is a reverse-engineering codebase with a strong bias toward runtime systems.

High-level file distribution from `src/`:

- `hyperlib/renderer`: 69 files
- `hyperlinked/patches`: 40 files
- `hyperlib/assets`: 30 files
- `hyperlib/memory`: 12 files
- `hyperlib/gameplay`: 12 files
- `hyperlib/streamer`: 8 files
- `hyperlib/world`: 8 files
- `hyperlib/collections`: 8 files

This already tells us where the repo is strongest: rendering, scene presentation, runtime patching, and streamed world data.

## Active Patch Modules

`src/hyperlinked/patches.cpp` currently initializes these patch families:

- generic bootstrap
- memory
- assets: scenery, textures, world animation
- renderer: camera, culling, directx, effect, flare renderer, light renderer, renderer, view, world renderer
- streamer: sections, streamer
- world: collision, world

This is important: the repo is already organized around replacing or intercepting Carbon subsystems, not just dumping structs.

## Carbon Systems Proven In Code

### Rendering and presentation

`hyperlinked` has direct coverage for:

- camera state and camera mover data
- renderer/view/world renderer split
- post-process and screen-effect pipeline
- flare, streak, fog, rain, depth-of-field style effects
- time-of-day lighting
- per-view rendering paths

Key evidence:

- `src/hyperlib/renderer/camera.hpp`
- `src/hyperlib/renderer/post_process.hpp`
- `src/hyperlib/renderer/screen_effect.hpp`
- `src/hyperlib/renderer/time_of_day.hpp`
- `src/hyperlib/renderer/visual_treatment.hpp`

### Track metadata and routing

`src/hyperlib/assets/track.hpp` shows Carbon track metadata well beyond simple lap race config:

- drift-related fields
- map calibration fields
- traffic density and spacing fields
- reverse/forward difficulty
- start/finish traffic allowances

`src/hyperlib/streamer/track_path.hpp` shows route-zone semantics that matter for Carbon feel:

- `garage`
- `traffic_pattern`
- `jump_camera`
- `pursuit_start`
- `canyon_drop`
- `vertigo_camera`
- barrier enable/disable groups

This is one of the clearest signals that Carbon relied on authored world zones, not just passive roads.

### Gameplay state

`src/hyperlib/gameplay/g_race.hpp` exposes:

- race contexts: `quick_race`, `online`, `challenge`, `career`
- race modes: `grip`, `high_speed_challenge`, `drag`, `drift`
- runtime race status helpers such as drift/racing/pursuit checks

`src/hyperlib/gameplay/dal.hpp` exposes a DAL interface layer with interfaces for:

- career
- world map
- pursuit
- rewards
- crew member
- challenge
- online database

This is a strong hint that Carbon’s progression and front-end state were heavily data/interface driven.

### HUD and GPS

Coverage exists but is lighter:

- `src/hyperlib/gameplay/gps.*`
- `src/hyperlib/ui/feng_hud.*`

That is enough to confirm HUD/GPS are first-class systems, but not enough on its own to reconstruct the full FE stack without more reverse engineering.

### World, collision, streaming

Coverage exists for:

- world representation
- collision/world grid
- streamer/sections
- memory pools used by streamed systems

This matters if SR3 ever attempts Carbon-style traffic, barriers, or semi-open routing.

## Carbon Signals Embedded In Camera Data

`src/hyperlib/renderer/camera.hpp` contains unusually specific runtime fields:

- drift state
- drift amount
- NOS percentage left
- `is_drift_race`
- `is_canyon_drift_race`
- `is_close_to_roadblock`
- `vertigo_camera_direction`
- `time_doing_vertigo_effect`

This is one of the most useful planning clues in the entire repo. It suggests Carbon’s camera logic was deeply aware of event type, high-speed spectacle, and road danger states.

## `nfsc-sdk` Coverage

`nfsc-sdk` is useful as a naming/index layer even when it is not a full implementation reference.

Important header families include:

- AI and traffic: `AICopManager.h`, `AIPursuit.h`, `AITrafficManager.h`, `AIWingman.h`
- camera and HUD: `Camera.h`, `CameraAI.h`, `HUD.h`
- gameplay and progression: `GCareer.h`, `GameFlowManager.h`, `GRaceParameters.h`, `GRaceStatus.h`
- FE/customization: `FeGarageMain.h`, `FECustomizationRecord.h`, `FECarRecord.h`, `FEPlayerCarDB.h`
- physics/vehicle: `PVehicle.h`, `DamageVehicle.h`, `Engine.h`, `IVehicle.h`
- world/collision: `TrackInfo.h`, `WCollider.h`, `WCollisionMgr.h`, `WCollisionBarrier.h`

Takeaway:

- `hyperlinked` is the stronger runtime behavior reference.
- `nfsc-sdk` is the faster symbol/name discovery reference.
- combined, they are enough to build a good implementation roadmap without IDA.

## IDA-Backed Carbon Type Signals

The live IDB local types confirm that Carbon contains first-class systems for the exact areas we care about.

Confirmed type names from `ida://types` include:

- `GCanyonRaceStatus`
- `DALWorldMap`
- `FEWorldMapStateManager`
- `FEWorldViewMenuScreen`
- `FECrewLogoLoader`
- `WorldMap`
- `FeQuickRaceOptions`
- `AutoSculpt`
- `FERewardCardManager`
- `FEngHud`
- `FEWorldMapQuickList`
- `FeBustedScreen`
- `FEPhotoModeStateManager`
- `FePhotoModeMenuScreen`
- `WRoadNetwork`
- `TrackInfo`
- `Camera`

This materially strengthens the implementation plan:

- canyon flow is a real dedicated system, not just track scripting,
- world map and FE world-view screens are real systems, not just menu labels,
- crew, rewards, HUD, and Autosculpt all exist as named type-level concepts in the IDB,
- busted and photo subscreens are also explicit FE concepts.

## Repo-Pinned Carbon Runtime Sites

`hyperlinked` provides concrete addresses for some of these systems, which helps anchor later IDA work:

- `Camera::SetCameraMatrix` at `0x004822F0`
- `WorldMap::SetupTrackPreviewStreamingData` patch site at `0x005C3811`
- `GCanyonRaceStatus::IsIntermissionSync` patch site at `0x00616DB0`
- `Game_CanyonRaceSetup` patch site at `0x00656C10`
- `GRaceStatus::SetRoaming` patch sites at `0x00641377` and `0x0064138A`

These addresses came from:

- `hyperlinked/src/hyperlinked/patches/renderer/camera.cpp`
- `hyperlinked/src/hyperlinked/patches/streamer/streamer.cpp`

Takeaway:

- the Carbon feel is not only a content problem,
- it is tightly tied to world-map, roaming, canyon, and camera runtime systems,
- these are already partially localized for future IDA-backed investigation.

## IDA-Backed Customization Structs

Two concrete struct reads already exposed useful customization data:

`ida://struct/MissingPartInfo`

- `CarTypeID`
- `CarSlotID`
- `PartNameHash`

`ida://struct/CarMemoryInfo`

- `CarMemType`
- `Sizes[7]`

Takeaway:

- part ownership / missing-part state is directly modeled,
- car memory budgeting or category sizing is also explicitly modeled,
- this is strong evidence that Carbon's customization and garage pipeline is deeper than a simple cosmetic menu layer.

## What The Reference Repos Do Not Yet Fully Give Us

Even with initial IDA access, these remain partially unresolved:

- exact state machines for canyon duel flow
- boss battle orchestration
- territory unlock logic
- crew member perk wiring
- full FE widget composition and transitions
- exact police/traffic spawn scheduling
- exact tuning of jump/vertigo/canyon camera triggers

## IDA Backlog

Next IDA work should answer these first:

1. Which functions drive canyon duel progression, failure, and camera switching?
2. Where are territory ownership, district unlocks, and boss progression stored?
3. Which DAL calls back the world map, rewards, and crew systems?
4. How are traffic and police spawn volumes connected to `track_path::zone` data?
5. Which camera mover subclasses own drift, canyon, jump, and vertigo behavior?
6. Which FE packages render the minimap, race HUD, and garage/customization screens?

## Planning Conclusion

The local Carbon repos already support a serious SR3 adaptation plan in four areas:

- camera and spectacle
- post-process rendering and lighting
- track/world trigger metadata
- progression/front-end structure

They are weakest where Carbon’s exact high-level game flow must be reconstructed from executable behavior. That is the part to reserve for future IDA-backed work.
