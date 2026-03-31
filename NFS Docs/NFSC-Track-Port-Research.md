# NFSC Full Track Port Research

## Goal

Document the real work required to port `Need for Speed: Carbon` track content into SR3 as playable content, not just as front-end metadata or preview assets.

This note is intentionally separate from `NFSC-Track-Usage-Research.md`.

- `NFSC-Track-Usage-Research.md` covers FE metadata, previews, zones, barriers, and staged reuse of Carbon track logic.
- this document covers the late-stage, high-cost path of importing Carbon track bundles, geometry, materials, collision, and road/navigation data into SR3.

## Executive Summary

Full Carbon track porting is feasible, but it is not one task.

It breaks into at least seven major sub-problems:

1. parse and extract Carbon `BUN` / `LZC` / stream-section data,
2. identify which extracted blobs are geometry, textures, collision, and metadata,
3. convert geometry into an interchange format SR3 tooling can consume,
4. convert or recreate materials and textures,
5. rebuild collision in an SR3-friendly form,
6. extract road/path/navigation data for AI and gameplay,
7. integrate the result into SR3 as authored track content.

The important architectural conclusion is:

- Carbon track *usage* can be adopted early,
- Carbon track *porting* should remain a late milestone,
- and the first real target should be one hand-picked validation track, not full city streaming.

## Evidence Base

This note is based on:

- `D:\Repos\Games\NFSC\hyperlinked`
- live `ida-pro-mcp` reads from `D:\Development\Debug_symbols\IDB_PC\NFSCarbon-v1.4\NFSCarbon-v1.4.i64`
- `D:\Repos\Games\Binarius`
- `D:\Repos\Games\Binarius\Binary\output\modelulator`
- `D:\Repos\Games\Maps\NFSMWMapLoader\NFSMWMapLoader.asm`
- local Carbon assets in:
  - `D:\Games\NFSC PS3`
  - `D:\Games\NFSC Redux`

## Carbon Track Asset Families

Confirmed local track-related asset families:

- `TRACKS\<REGION>\TrackMaps.bin`
- `TRACKS\<REGION>\MINI_MAP_*.bin`
- `TRACKS\<REGION>\TroughBoundary.bin`
- `TRACKS\<REGION>.BUN`
- `TRACKS\STREAM<REGION>.BUN`
- `TRACKS\HotPosition<REGION>.HOT`

The files do not represent one flat "map file".

The real split is:

- FE preview / map assets,
- region bundles,
- streamed runtime sections,
- boundary / zone / barrier data,
- hot-position / placement data,
- runtime road-network data consumed by gameplay systems.

## Raw Stream Section Findings

Direct inspection of `D:\Repos\Games\Binarius\Binary\output\L5RA_raw` changes the earlier assumptions substantially.

The streamed `DATA.BIN` files are not flat vertex/index arrays.

Confirmed section roles:

- `A*` sections start with `GeometryPack = 0x80134000`
- those geometry packs contain nested `ScenerySection = 0x80034100`
- inside the scenery section, the important chunks are:
  - `scenery_header = 0x00034101`
  - `scenery_infos = 0x00034102`
  - `scenery_instances = 0x00034103`
  - `scenery_tree_nodes = 0x00034105`
  - `light_texture_collections = 0x0003410D`
- `Y*` sections start with `TPKBlocks = 0xB3300000`
- `Y*` texture sections contain `TPK_InfoBlock = 0xB3310000` and the expected `0x3331000*` info chunks
- `Z0` contains repeated `WCollisionPack = 0x0003B801` blocks

Important implication:

- the old converter assumption that `A*/DATA.BIN` is a sequence of `position + normal + uv` vertices is false
- the old assumption that `Y*` or `X*` files are direct index buffers is also false for this extraction set
- any real port must parse Carbon chunk trees first, then resolve geometry/collision/texture ownership from those trees

The current repo tool `tools\CarbonTrackConverter.py` has been moved to section-inventory duties accordingly and now emits a manifest instead of a fake mesh export.

## What The Current Parser Now Resolves

The current Python parser is no longer limited to top-level chunk names.

It now confirms and reports:

- `0x80134001` as the solid-list container inside a `GeometryPack`
- `0x00134002` as the in-file solid-list header
- `0x80134010` as per-solid containers
- `0x00134011` as the live `solid` record
- `0x80134100` as the nested per-solid platform container
- `0x00134900` as `platform_info`
- `0x00134B02` as the `mesh_entry` table
- `0x00134B03` as the index-buffer payload
- `0x00134B01` as the file vertex-buffer payload
- `0x00134C02` as per-submesh material-name payloads

It also resolves `scenery_infos` solid keys against the local per-section solid set and reports:

- resolved vs unresolved solid-key counts
- per-solid mesh metadata
- per-submesh material names
- per-submesh vertex/index-buffer byte sizes

That means the project now has an auditable raw-data path for `scenery -> solid key -> solid -> mesh entries -> file vertex/index buffers`.

## What Hyperlinked Contributes

`hyperlinked` is the clearest structural reference for Carbon track packaging and runtime ownership.

### Chunk IDs and ownership

Confirmed chunk IDs:

- `track_streaming_sections = 0x00034110`
- `track_streaming_infos = 0x00034111`
- `track_streaming_barriers = 0x00034112`
- `track_streaming_discs = 0x00034113`
- `track_path_manager = 0x80034147`
- `track_path_points = 0x00034148`
- `track_path_lanes = 0x00034149`
- `track_path_zones = 0x0003414A`
- `track_path_barriers = 0x0003414D`
- `world_road_network = 0x0003B800`

This already shows that a full port needs more than static mesh extraction.

### Track-path gameplay structures

`src\hyperlib\streamer\track_path.hpp` exposes:

- `track_path::zone`
- `track_path::barrier`
- `track_path::manager`

Important zone types:

- `reset`
- `guided_reset`
- `tunnel`
- `overpass`
- `streamer_prediction`
- `garage`
- `traffic_pattern`
- `jump_camera`
- `no_cop_spawn`
- `pursuit_start`
- `canyon_drop`
- `vertigo_camera`

Important implication:

- if a track is ported only as geometry, it still will not behave like Carbon until these authored path zones are preserved or recreated.

### Visible section manager

`src\hyperlib\streamer\sections.hpp` exposes a second major system:

- `visible_section::boundary`
- `visible_section::drivable`
- `visible_section::loading`
- `visible_section::textures`
- `visible_section::user_info`
- `visible_section::manager`

Important fields:

- `drivable_boundary_list`
- `drivable_section_list`
- `loading_section_list`
- `user_infos[27000]`
- `enabled_groups[0x200]`
- `section_lod_offset`
- `current_zone_number`

Important implication:

- Carbon streaming is not just "load section N";
- it also manages drivable sections, loading groups, topology/scenery group toggles, and per-section auxiliary data.

### Streamer implementation meaning

`src\hyperlib\streamer\streamer.cpp` shows the real streamer owns:

- section activation,
- section memory allocation,
- current-section visibility bookkeeping,
- disc-bundle handling,
- user memory allocation,
- loading state progression.

Important implication:

- a one-to-one runtime port of Carbon streaming into SR3 is a major subsystem project, not a converter script.

## What Hyperlinked Says About Geometry

`hyperlinked` also explains why a generic raw-vertex decoder is insufficient.

In `src\hyperlib\assets\geometry.hpp`:

- `geometry::solid::flags` includes `compressed_verts = 0x1`
- `geometry::mesh_entry` contains:
  - `file_vertex_buffer`
  - `file_vertex_buffer_size`
  - `vertex_buffer_data`
  - `vertex_offset`
  - `triangle_count`
  - `index_start`
  - `effect`

In `src\hyperlib\renderer\effect.hpp`:

- vertex layout is shader/effect-driven
- supported input component types include:
  - `float1`
  - `float2`
  - `float3`
  - `float4`
  - `color`
  - `ubyte4`
  - `short4n`

In `src\hyperlib\assets\geometry.cpp`:

- runtime vertex buffers are built from `entry.file_vertex_buffer`
- draw stride comes from `entry.effect->stride()`

Important implication:

- even after locating the correct geometry payloads, there may not be one universal Carbon world-vertex struct
- the exporter needs to reproduce or decode `solid` / `mesh_entry` vertex-buffer preparation, not just reinterpret bytes as one fixed layout

## What IDA Pro MCP Confirms

The active IDB is:

- module: `NFSC.exe`
- base: `0x400000`

### Track metadata and FE usage

Confirmed functions:

- `TrackInfo::GetTrackInfo` at `0x7990C0`
- `DALWorldMap::GetTrackArtName` at `0x4A9500`
- `DALWorldMap::GetTrackID` at `0x4A96B0`
- `DALWorldMap::GetTrackDisplayName` at `0x4A97C0`
- `DALWorldMap::GetTrackEngagePos` at `0x4A9930`
- `DALWorldMap::GetTrackRegion` at `0x4A9150`
- `UITrackMapStreamer::UITrackMapStreamer` at `0x5B7600`

Decompilation-backed findings:

- `TrackInfo::GetTrackInfo` walks a fixed `272` byte table and matches entries by the 16-bit value at offset `138`.
- `UITrackMapStreamer::UITrackMapStreamer` builds `TRACKS\\%s\\TrackMaps.bin`, then disables zone switching through `TrackStreamer::DisableZoneSwitching`.

Important implication:

- Carbon preview loading is a special case, not the same path as in-race streaming.

### Runtime streaming

Confirmed functions:

- `TrackStreamer::FindSection` at `0x799ED0`
- `TrackStreamer::DisableZoneSwitching` at `0x79A3C0`
- `TrackStreamer::EnableZoneSwitching` at `0x79A3E0`
- `TrackStreamer::HandleLoading` at `0x7A7230`
- `TrackStreamer::CheckLoadingBar` at `0x7A82E0`
- `TrackStreamer::ServiceGameState` at `0x7A85E0`

Decompilation-backed findings:

- `TrackStreamer::FindSection` scans `92` byte section records keyed by 16-bit section number.
- this matches Binarius `CarbonStreamingSection` sizing and confirms the section table format is not guessed.

### Road and collision systems

Confirmed functions:

- `WRoadNetwork::ResetRaceSegments` at `0x7EAF90`
- `WRoadNetwork::ResetBarriers` at `0x7EB230`
- `WRoadNetwork::ResolveBarriers` at `0x8110D0`
- `LoaderWCollisionPack` at `0x699810`
- `WCollisionAssets::SetExclusionFlags` at `0x8125B0`

Important findings:

- `LoaderWCollisionPack` only accepts the world-collision chunk ID `243713`, then dispatches its loader.
- `WRoadNetwork::ResolveBarriers` is large, race-aware, and modifies live segment state.

Important implication:

- collision and barrier behavior are their own runtime layer,
- so a geometry-only import still will not give Carbon gameplay behavior.

## What Binarius Contributes

`Binarius` is immediately useful for extraction and metadata work.

### Proven Carbon stream-section extractor

The strongest reusable source is:

- `D:\Repos\Games\Binarius\Modules\Endscript\Endscript\Streamable\CarbonStream.cs`

Confirmed behavior:

- opens Carbon `LXRY` and `STREAMLXRY` file pair,
- scans `LXRY` for `BinBlockID.TrackStreamingSections`,
- treats each section record as `0x5C` bytes,
- deserializes an array of `CarbonStreamingSection`,
- seeks into the stream file by `FileOffset`,
- extracts section payloads as `DATA.BIN`,
- writes section metadata as `Settings.end`,
- supports saving / repacking back into the original files.

Important implication:

- SR3 does not need a first-principles `STREAM*.BUN` section extractor;
- there is already a proven Carbon section extractor/repacker that matches IDA-confirmed structure size.

### Carbon track metadata model

Useful metadata source:

- `D:\Repos\Games\Binarius\Modules\Darius\Nikki\Support.Shared\Class\Track.cs`

Confirmed useful fields:

- `RegionName`
- `TrackDirectory`
- `RegionDirectory`
- `RaceGameplayMode`
- forward/reverse difficulty
- sun info

Important implication:

- Binarius can help reconstruct track database content and event-facing metadata, not just raw bundles.

### Texture, material, collision classes

Useful Carbon-side classes also exist in Binarius:

- `Support.Carbon\Class\Texture.cs`
- `Support.Carbon\Class\Material.cs`
- `Support.Carbon\Class\Collision.cs`
- `Support.Carbon\Class\TPKBlock.cs`
- `Support.Carbon\Class\STRBlock.cs`

Important implication:

- even if Binarius does not provide a turnkey SR3 importer, it is still a strong source for Carbon binary layout and texture/material/collision parsing helpers.

## What Modelulator Contributes

`D:\Repos\Games\Binarius\Binary\output\modelulator` is a second-stage decoded export set and is useful, but only partially complete.

Confirmed outputs:

- `CollisionPacks\WCollisionPack.Z0.S*.obj`
- `RoadNetworks\WRoadNetwork.obj`
- `ScenerySections\SECTION_*.dae`

Important validated behavior:

- `ScenerySections` DAEs preserve the real section number internally through node IDs such as `S1001_0x0001`
- the scenery DAE for section `1001` is `SECTION_J1.dae`
- those DAEs reference expected solid files using stable filenames such as:
  - `0x05087FFF.A1.XOS_TIRESHOPPOSTB_1B_00.dae`
  - `0x6B95C282.A1.XOS_PETROPIPE_2B_00.dae`

Important limitation:

- the current `modelulator` run did not actually emit those per-solid DAE files
- only `14` tiny `Solids\eSolidPalette.*.dae` files were written, and they are effectively empty stubs
- the run summary shows `4681` successes and `1996` errors, so this export set is not complete enough to replace raw parsing

Practical implication:

- `modelulator` is already useful as a decoded reference for:
  - section instance transforms
  - road network geometry
  - collision geometry
- but it is not yet a complete world-geometry exporter in the current workspace
- the raw `CarbonTrackConverter.py` manifest remains the authoritative structural source

## What NFSMWMapLoader Contributes

`D:\Repos\Games\Maps\NFSMWMapLoader\NFSMWMapLoader.asm` is valuable as a design reference for an external runtime loader.

Focused disassembly windows confirm:

- `"MapModel"` and `"TexturePath"` are read together,
- `"MapCollision"` and `"Path"` are read together,
- config flags include:
  - `"CollisionsEnabled"`
  - `"GenerateBarriers"`
  - `"ShowCollisions"`
  - `"DontUnloadCulledModels"`
- stats/debug output include:
  - `"Collision hooks "`
  - `"Models loaded: "`
  - `"Textures loaded: "`
  - `"Meshes drawn: "`
- imports / RTTI confirm use of:
  - `D3DXCreateTextureFromFileExA`
  - `btBvhTriangleMeshShape`
  - `btTriangleMesh`
  - `BulletCol::CustomRigidBody_WithMaterial`
  - `btCollisionWorld::RayResultCallback`

Important implication:

- the external loader is doing four jobs:
  - model loading,
  - texture loading,
  - collision generation,
  - runtime collision query integration.

This makes it a good reference for *responsibility split*, not a direct drop-in for SR3.

## Real Implementation Breakdown

The practical full-port pipeline should be:

### Stage 0: Choose one validation track

Do not begin with full city support.

Pick one track that is:

- self-contained,
- visually representative,
- not the hardest streaming case,
- and useful for validating road path extraction.

Deliverable:

- one named Carbon region / event target with source asset list checked in notes.

### Stage 1: Container extraction

Implement or script:

- `BUN` / `LZC` / stream section extraction,
- region bundle unpack,
- section table export,
- basic metadata dump.

Primary references:

- `Binarius` `CarbonStream.cs`
- `BinBlockID.TrackStreamingSections`
- IDA `TrackStreamer::FindSection`

Deliverables:

- extracted section payloads,
- exported section metadata table,
- manifest of region assets and file roles.

### Stage 2: Content classification

For each extracted asset blob, determine whether it is:

- geometry,
- textures,
- materials,
- collision,
- path / zone / barrier data,
- road-network data,
- preview / FE data.

Primary references:

- `hyperlinked` chunk IDs
- Binarius Carbon class library
- IDA loader ownership

Deliverables:

- chunk-to-content map,
- one documented specimen of each required blob type.

### Stage 3: Geometry conversion

Build an offline converter from Carbon geometry payloads into an interchange format.

Recommended target interchange:

- `glTF`, `FBX`, or Blender scene import first,
- then a second conversion into SR3 mesh/track authoring format.

Do not try to emit SR3 runtime data directly on the first pass.

Deliverables:

- one Carbon section or one complete test track visible in Blender,
- mesh hierarchy preserved well enough to inspect roads, props, and major landmarks.

### Stage 4: Texture and material conversion

Build a material conversion layer that:

- extracts Carbon textures,
- converts them to standard formats,
- maps Carbon shader/material meaning into SR3/Ogre PBS equivalents,
- records unsupported features for manual cleanup.

Use:

- Binarius texture / material classes,
- texture container knowledge from Carbon class code,
- Blender or offline material inspection as validation.

Deliverables:

- diffuse / normal / mask extraction pipeline,
- first usable SR3 material remap table,
- one track rendered with acceptable placeholder materials.

### Stage 5: Collision conversion

Treat collision as a separate pipeline, not as a side effect of render mesh conversion.

Two possible outputs:

- offline-generated triangle collision mesh,
- or simplified authored collision derived from imported geometry.

Use:

- IDA `LoaderWCollisionPack`
- IDA `WCollisionAssets::SetExclusionFlags`
- NFSMWMapLoader Bullet evidence
- Binarius `Collision.cs`

Deliverables:

- one imported track with drivable collision,
- documented distinction between visual mesh and collision mesh.

### Stage 6: Road spline and gameplay metadata extraction

This is the critical step for SR3 gameplay quality.

Extract or reconstruct:

- main route centerline,
- lane counts / directionality,
- track-path points,
- track-path lanes,
- track-path zones,
- track-path barriers,
- engage / start / finish positions.

Use:

- `hyperlinked` `track_path.hpp`
- chunk IDs `0x34148`, `0x34149`, `0x3414A`, `0x3414D`
- IDA `DALWorldMap::GetTrackEngagePos`
- IDA `WRoadNetwork::*`

Deliverables:

- one SR3-friendly spline or route graph,
- zone / barrier import format,
- AI-ready route skeleton for one test track.

### Stage 7: SR3 integration

Only after extraction/conversion works should SR3 integration start.

Integration targets:

- track database entry,
- preview/minimap metadata,
- imported geometry and materials,
- imported collision,
- imported route spline,
- imported Carbon-style zones/barriers.

Deliverables:

- one playable imported track in SR3,
- one AI pass using imported route data,
- one event mode validation on imported content.

### Stage 8: Optional runtime section streaming

This is the last step, not the first.

Only attempt it if the project later needs:

- large world regions,
- Carbon-like live section swapping,
- topology/scenery group toggles,
- open-world traversal beyond authored race tracks.

Deliverables:

- a dedicated streaming prototype,
- not part of the first imported-track milestone.

## What Should Be Reused vs Rewritten

### Reuse directly where possible

- Binarius extraction and repack logic for Carbon stream sections
- Binarius metadata and class layouts
- hyperlinked chunk IDs, struct layouts, and manager semantics
- ida-pro-mcp findings for ownership and function behavior

### Use as design references, not direct code

- NFSMWMapLoader runtime loader structure
- Bullet collision concepts from the map-loader
- Carbon runtime streamer behavior

### Rewrite for SR3

- geometry conversion into SR3-compatible authored track data
- material mapping into Ogre/SR3 rendering
- collision export path compatible with SR3 physics/content expectations
- AI route integration into SR3 gameplay systems
- editor/import tooling around SR3 data formats

## Recommended Toolchain

The realistic toolchain is:

- Binarius or custom scripts for extraction,
- custom parsers for chunk classification,
- current `modelulator` exports for decoded scenery/collision/road reference,
- Blender plus NFS-oriented plugins or custom import scripts for geometry inspection,
- custom exporters for SR3 track data,
- SR3-side importer/integration scripts and data loaders.

This is not a pure reverse-engineering task and not a pure art task.

It is a tooling pipeline task.

## Recommended Milestones

### Milestone A: Extraction proven

- extract one region and one stream set,
- dump section metadata,
- classify core blob types.

### Milestone B: Geometry visible

- load one Carbon section or one small track into Blender,
- confirm orientation, scale, and hierarchy.

### Milestone C: Collision and route usable

- get drivable collision,
- get one route spline and start/finish positions.

### Milestone D: First playable imported SR3 track

- imported geometry,
- placeholder materials,
- collision,
- AI route,
- event start/end flow.

### Milestone E: Fidelity pass

- texture/material quality,
- zone/barrier semantics,
- event-specific cameras and triggers,
- optional minimap/preview integration.

## Main Risks

- Carbon geometry payload format may still require additional reverse engineering before robust conversion.
- Section contents may mix render, collision, and auxiliary data in ways that need more classification work.
- Carbon material semantics will not map one-to-one to SR3's Ogre PBS pipeline.
- Collision data may require a separate authoring path even if render geometry imports correctly.
- `WRoadNetwork` behavior is richer than track-path data alone, so AI quality may be limited until route extraction is mature.
- full live section streaming is much more expensive than one imported authored track.

## Recommended Project Stance

Treat full Carbon track porting as:

- a late phase,
- a tooling-heavy offline import project,
- and a per-track effort measured in weeks to months until the pipeline matures.

Do not block earlier Carbon adaptation work on it.

The correct order remains:

1. Carbon handling and presentation,
2. Carbon track metadata / preview / zone usage,
3. Carbon event and career shell,
4. one imported validation track,
5. only much later optional runtime streaming or open-world reuse.

## Immediate Next Actions If This Starts

1. Use Binarius `CarbonStream.cs` as the baseline for a dedicated extraction script or wrapper.
2. Keep `CarbonTrackConverter.py` as the authoritative raw-section manifest generator.
3. Use `modelulator\ScenerySections\SECTION_*.dae` plus `WRoadNetwork.obj` and `CollisionPacks\*.obj` as secondary validation assets.
4. Repair or replace the missing per-solid `modelulator` export path.
5. Prove geometry inspection in Blender before writing SR3 import code.
6. Extract track-path points / lanes / zones / barriers for the same target.
7. Only then start SR3-side geometry, collision, and AI integration.
