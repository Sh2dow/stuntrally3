# NFSC Track Usage Research

## Goal

Document how `Need for Speed: Carbon` uses track data so SR3 can adopt Carbon track assets and track logic in a staged, technically credible way.

This note is based on:

- local `hyperlinked` source and patch set
- live `ida-pro-mcp` reads from the active `NFSC.exe` IDB
- shipped Carbon assets in `D:\Games\NFSC Redux\TRACKS`

## Executive Summary

Carbon does not use tracks as one flat "load a map" asset.

Track usage is split across three layers:

1. front-end track metadata and preview assets,
2. runtime streaming sections and section-group management,
3. road/path topology and gameplay zones/barriers.

For SR3, this means "using NFSC tracks" should also be split into three implementation targets:

1. use Carbon track metadata, names, minimaps, and preview art first,
2. use Carbon track path zones and barrier semantics second,
3. only then attempt full streamed section/topology usage.

Trying to jump straight to full `TRACKS\\*.BUN` runtime streaming inside SR3 would be high-risk and low-yield compared with the staged approach above.

## Asset Layout From Shipped Carbon Data

Confirmed from `D:\Games\NFSC Redux\TRACKS`:

- `TRACKS\L5RA\TrackMaps.bin`
- `TRACKS\L5RA\MINI_MAP_CASINOTOWN_LOCKED.bin`
- `TRACKS\L5RA\MINI_MAP_CASINOTOWN_UNLOCKED.bin`
- `TRACKS\L5RA\TroughBoundary.bin`
- `TRACKS\L5RA.BUN`
- `TRACKS\L5RB.BUN`
- `TRACKS\STREAML5RA.BUN`
- `TRACKS\STREAML5RB.BUN`

IDA string reads also confirm Carbon path templates:

- `TRACKS\\%s\\TrackMaps.bin`
- `TRACKS\\%s\\%s.bin`
- `TRACKS\\STREAM%s.BUN`
- `TRACKS\\%s.BUN`
- `TRACKS\\HotPosition%s.HOT`

Implication:

- FE preview and map assets live under per-region folders like `TRACKS\L5RA\...`
- full regional/world data is packed into base bundles like `L5RA.BUN`
- streamable runtime sections are split into `STREAM*.BUN`
- hot positions have their own file family

## Carbon Runtime Ownership Model

### 1. Front-end track metadata

IDA confirms Carbon exposes track FE data through `DALWorldMap` and `TrackInfo`.

Key functions:

- `TrackInfo::GetTrackInfo` at `0x7990C0`
- `DALWorldMap::GetTrackArtName` at `0x4A9500`
- `DALWorldMap::GetTrackID` at `0x4A96B0`
- `DALWorldMap::GetTrackDisplayName` at `0x4A97C0`
- `DALWorldMap::GetTrackEngagePos` at `0x4A9930`
- `DALWorldMap::GetTrackRegion` at `0x4A9150`

Important behavior:

- `TrackInfo::GetTrackInfo` searches `TrackInfoTable` with fixed-size entries of `272` bytes, keyed by a 16-bit value at offset `138`.
- `DALWorldMap::GetTrackArtName` does not directly inspect track files; it resolves race parameters and maps race type to FE art naming.
- `DALWorldMap::GetTrackID` returns the event ID string, then normalizes `.` to `_`.
- `DALWorldMap::GetTrackEngagePos` can either pull map position from race database data or read it from the active career/world-map track handle.
- `DALWorldMap::GetTrackRegion` resolves race region and clamps invalid values.

Conclusion:

- Carbon FE track selection is driven by race database / world-map metadata, not by raw `TRACKS` bundle enumeration.

### 2. Track preview and FE map streaming

IDA confirms dedicated preview streaming exists.

Key functions:

- `UITrackMapStreamer::UITrackMapStreamer` at `0x5B7600`
- `UITrackMapStreamer::SetPan` at `0x69FFE0`
- `UITrackMapStreamer::ResetPan` at `0x56E1F0`
- `UITrackMapStreamer::ResetZoom` at `0x56E1A0`

Important behavior from decompilation:

- `UITrackMapStreamer` builds the path `TRACKS\\%s\\TrackMaps.bin` using `TrackInfo::GetTrackInfo(...)`
- it disables track zone switching through `TrackStreamer::DisableZoneSwitching`
- it schedules FE track map loading rather than using the normal in-race streaming flow

Hyperlinked patch points confirm the same ownership:

- `UITrackMapStreamer::UITrackMapStreamer`
- `WorldMap::SetupTrackPreviewStreamingData` at `0x005C3811`

Conclusion:

- Carbon already treats FE track preview as a separate usage mode of track data
- for SR3, this is the lowest-risk place to begin reusing Carbon track assets

### 3. Runtime track streaming

Carbon uses a dedicated `TrackStreamer`.

Confirmed functions from IDA:

- `TrackStreamer::FindSection` at `0x799ED0`
- `TrackStreamer::DisableZoneSwitching` at `0x79A3C0`
- `TrackStreamer::EnableZoneSwitching` at `0x79A3E0`
- `TrackStreamer::HandleLoading` at `0x7A7230`
- `TrackStreamer::CheckLoadingBar` at `0x7A82E0`
- `TrackStreamer::ServiceGameState` at `0x7A85E0`

Relevant globals:

- `TheTrackStreamer` at `0x00B70650`
- `TRACKSTREAMER_BACKLOG_THRESH`

Important behavior from IDA:

- `TrackStreamer::FindSection` linearly searches section records with a stride of `92` bytes, keyed by a 16-bit section number.
- `TrackStreamer::DisableZoneSwitching` just flips a disable flag and stores a reason string pointer.
- `TrackStreamer::ServiceGameState` updates section/backlog state each frame and only triggers switching when zone switching is allowed and work exists.
- `TrackStreamer::CheckLoadingBar` inspects pending loading state and section identifiers to decide whether the loading bar should be active.

Hyperlinked makes the chunk model explicit:

- `track_streaming_sections = 0x00034110`
- `track_streaming_infos = 0x00034111`
- `track_streaming_barriers = 0x00034112`
- `track_streaming_discs = 0x00034113`

Hyperlinked streamer loader behavior:

- `track_streaming_sections` loads and sorts `section` records by section number
- `track_streaming_infos` loads one global info block
- `track_streaming_barriers` loads streamer barrier records
- `track_streaming_discs` loads disc-bundle metadata

Conclusion:

- Carbon streaming is section-driven, not whole-track-driven
- it has explicit load/unload bookkeeping, loading-bar logic, and zone-switching control
- SR3 should not attempt to reinterpret `STREAM*.BUN` until a compatible section model exists

### 4. Visible sections and scenery-group control

Hyperlinked exposes the missing middle layer between the streamer and world content: `visible_section::manager`.

Key structures from `hyperlib/streamer/sections.hpp`:

- `visible_section::boundary`
- `visible_section::drivable`
- `visible_section::loading`
- `visible_section::user_info`
- `visible_section::manager`

Important fields:

- `drivable_section_list`
- `loading_section_list`
- `user_infos[27000]`
- `enabled_groups[0x200]`
- `section_lod_offset`
- `current_zone_number`

Important behavior from `sections.cpp`:

- `loader_pack_header(...)` reads pack header and `lod_offset`
- `loader_boundaries(...)` loads visible-section boundaries
- `loader_drivables(...)` loads drivable section definitions
- `loader_loadings(...)` loads loading-section groupings
- `enable_group(...)`, `disable_group(...)`, and `disable_all_groups(...)` control topology/scenery groups
- `unloader(...)` clears drivables, loadings, boundaries, and state when the pack is unloaded

Hyperlinked patch points show Carbon injects this manager into:

- `TrackStreamer::DetermineStreamingSections`
- `TrackStreamer::GetPredictedZone`
- `TrackStreamer::InitRegion`
- `TrackStreamer::SwitchZones`
- `TrackLoader::RedoTopologyAndSceneryGroups`
- `TrackLoader::CloseTopologyAndSceneryGroups`
- `TrackLoader::Unload`

Conclusion:

- the streamer is not enough by itself
- Carbon also maintains a second layer of drivable/visible/loading sections and enabled scenery/topology groups
- this is a major reason full raw track-bundle support is not a short-term import target for SR3

### 5. Road network and gameplay navigation

Carbon has a separate world road/topology layer.

Confirmed chunk ID from `hyperlinked`:

- `world_road_network = 0x0003B800`

Confirmed runtime systems from IDA:

- `WRoadNetwork::ResetRaceSegments` at `0x7EAF90`
- `WRoadNetwork::ResetBarriers` at `0x7EB230`
- `WRoadNetwork::GetSegmentProfile` at `0x7EB290`
- `WRoadNetwork::GetSegmentTrafficLaneRightSide` at `0x7EB350`
- `WRoadNetwork::ResolveBarriers` at `0x8110D0`
- `WRoadNav::InitAtPoint` at `0x80F180`
- `WRoadNav::IncNavPosition` at `0x80C600`
- `PathFinder::Cancel` at `0x7F6780`

Important behavior:

- `ResetRaceSegments` clears per-segment race flags across the full segment table
- `ResolveBarriers` is race-aware and walks live segments/barrier state, not just static track-path barriers
- `WRoadNav` is the gameplay navigation layer used for movement and path progression over the road graph

Conclusion:

- Carbon road usage is split between:
  - track path / zone metadata,
  - visible section streaming,
  - and a true world road network
- SR3 can copy Carbon-like zone/barrier semantics before it can copy full Carbon road-graph behavior

### 6. Track path manager and gameplay zones

Hyperlinked `track_path.hpp` is one of the most useful implementation references for SR3.

Confirmed chunk IDs:

- `track_path_manager = 0x80034147`
- `track_path_points = 0x00034148`
- `track_path_lanes = 0x00034149`
- `track_path_zones = 0x0003414A`
- `track_path_barriers = 0x0003414D`

Useful runtime global:

- `TheTrackPathManager` at `0x00B70EA0`

`track_path::zone::type` includes:

- `reset`
- `guided_reset`
- `tunnel`
- `overpass`
- `streamer_prediction`
- `garage`
- `traffic_pattern`
- `dynamic`
- `neighborhood`
- `jump_camera`
- `no_cop_spawn`
- `pursuit_start`
- `highway`
- `canyon_drop`
- `vertigo_camera`

`track_path::barrier` supports:

- two-point barrier geometry
- enabled state
- player-only barriers
- handedness
- `group_key`

Conclusion:

- Carbon track-path data is the best near-term bridge into SR3
- it already encodes exactly the kind of authored metadata SR3 needs for:
  - canyon drop fail zones,
  - jump camera triggers,
  - vertigo beats,
  - traffic patterns,
  - barrier enable/disable groups,
  - garage/start presentation markers

## What This Means For SR3

## Best Low-Risk Reuse Targets

### A. FE/world-map track usage

Use first:

- `TrackMaps.bin`
- locked/unlocked minimaps
- track display names / art names / event IDs
- engage positions / region mapping

SR3 implementation targets:

- `src/game/data/CareerXml.*`
- `src/game/Gui_Games.cpp`
- `src/game/Gui_InitGames.cpp`
- `src/game/Gui_ProgressLoad.cpp`

Why first:

- least architectural risk
- aligns with Carbon world-map shell work already planned
- gives immediate Carbon identity without runtime world streaming

### B. Track-path zones and barriers

Use second:

- track-path zones
- track-path barriers
- zone types like `jump_camera`, `canyon_drop`, `vertigo_camera`, `traffic_pattern`

SR3 implementation targets:

- `src/road/SplineBase.h`
- `src/road/Road_File.cpp`
- `src/road/Road_Prepass.cpp`
- `src/road/Road_Rebuild.cpp`
- `src/road/PaceNotes*.cpp`

Why second:

- Carbon-like authored event logic can be layered onto SR3 tracks without full Carbon bundle loading
- directly supports the planned canyon and spectacle metadata slice

### C. Hot positions / engage positions

Use third:

- `GetTrackEngagePos`
- `TRACKS\\HotPosition%s.HOT`

SR3 implementation targets:

- race spawn markers
- FE transition camera anchors
- canyon boss/introduction start positions

### D. Full streamed section support

Use later:

- `STREAM*.BUN`
- visible section manager
- topology/scenery group toggling
- section user-info ownership

Why later:

- requires a Carbon-like section model, loading model, and scenery-group system
- highest implementation cost
- least likely to pay off early compared with FE maps and track-path metadata

## Recommended SR3 Implementation Order

1. add Carbon FE track metadata and minimap ingestion,
2. add Carbon-style track-path zones/barrier semantics to SR3 road metadata,
3. add engage/hot-position support for event starts and FE transitions,
4. prototype one importer/parser for `TrackMaps.bin` and track-path chunks,
5. only after that evaluate whether full `STREAM*.BUN` section streaming is worth porting.

## Concrete Deliverables

### Deliverable 1: FE Track Metadata Adapter

Build a small SR3 adapter that can store:

- track display name
- track art name
- event ID
- region
- minimap preview paths
- locked/unlocked state
- engage position

### Deliverable 2: Carbon Zone Vocabulary In SR3

Add SR3-side metadata equivalents for:

- `jump_camera`
- `canyon_drop`
- `vertigo_camera`
- `traffic_pattern`
- `garage`
- `pursuit_start`
- barrier group enable/disable

### Deliverable 3: Track Preview Import Path

Support importing or converting:

- `TrackMaps.bin`
- `MINI_MAP_*`

into an SR3 FE preview format or a simple intermediate export.

### Deliverable 4: Track Research Parser Layer

Build one standalone parser/research tool for:

- chunk IDs from `TRACKS\\*.BUN`
- visible section metadata
- track-path metadata
- hot positions

This parser layer should exist before any engine-side runtime import attempt.

## Recommended Non-Goals For Now

Do not start with:

- direct runtime use of Carbon `STREAM*.BUN` in SR3
- one-to-one port of `TrackStreamer`
- one-to-one port of `WRoadNetwork`
- direct loading of all Carbon world bundles into SR3 rendering

Those are late-stage goals, not the first usable track-integration milestone.

## Key Research References

Local source:

- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlib\chunk.hpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlib\streamer\track_path.hpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlib\streamer\sections.hpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlib\streamer\sections.cpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlib\streamer\streamer.cpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlinked\patches\streamer\sections.cpp`
- `D:\Repos\Games\NFSC\hyperlinked\src\hyperlinked\patches\streamer\streamer.cpp`

Live IDA systems used:

- `TrackStreamer`
- `UITrackMapStreamer`
- `TrackInfo`
- `DALWorldMap`
- `WRoadNetwork`
- `WRoadNav`

Shipped assets:

- `D:\Games\NFSC Redux\TRACKS`

## Bottom Line

The technically correct way to "use NFSC tracks" in SR3 is:

- use Carbon track metadata first,
- use Carbon track-path gameplay metadata second,
- delay full Carbon streaming/section/runtime road-network import until much later.

That order matches Carbon's real architecture and gives SR3 the most Carbon value for the least engineering risk.
