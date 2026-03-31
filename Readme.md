![License](https://flat.badgen.net/github/license/stuntrally/stuntrally3)

## Links

### Main
This is an experimental attempt to push Stunt Rally 3 toward NFS Carbon.

---

## Current Status

The track-port tooling is currently in research mode, not full conversion mode.

There is now one primary entry point for collecting the required Carbon track data:
- `tools/CarbonUnifiedToolkit.py`
- `tools/RunCarbonUnifiedToolkit.ps1`

It merges:
- raw Binarius section inventory
- modelulator section/solid export
- AssetDumper section DAEs
- optional PNG texture conversion output

into one normalized output bundle with:
- `unified_manifest.json`
- patched textured section DAEs
- local linked/copied texture files
- raw-inventory side data

Confirmed from `Binarius`, `hyperlinked`, and local raw section analysis:
- `A*` streamed files are `GeometryPack` sections that contain `ScenerySection` chunk trees.
- `Y*` streamed files are `TPKBlocks` / texture-pack payloads.
- `Z0` is a `WCollisionPack` stream.
- the region `L5RA.BUN` contains track metadata such as `TrackStreamingSections`, `TrackPathManager`, `WorldRoadNetwork`, and related FE/runtime data.
- `D:\Repos\Games\Binarius\Binary\output\modelulator` proves there is already a partial decoded export path:
  - `CollisionPacks\*.obj` exports world collision per section
  - `RoadNetworks\WRoadNetwork.obj` exports the road network as geometry
  - `ScenerySections\*.dae` exports section instance transforms and references solid keys by filename
  - `Solids\*.dae` now exports the referenced solid geometry split by Carbon solid key
  - textures still do not come directly from modelulator in this workflow, so textured section DAEs currently come from AssetDumper plus copied PNGs

What is not solved yet:
- decoding Carbon world solids into exportable mesh data
- converting Carbon collision packs into SR3 collision
- rebuilding SR3 heightmaps/road splines from real Carbon geometry

The old assumption that `A*/DATA.BIN` is a flat stream of 32-byte vertices was wrong.

---

## Tool Status

| Tool | Status | Purpose |
|------|--------|---------|
| `tools/RunCarbonSectionBlendAll.ps1` | Process-all viewer | One-command wrapper that resolves all bridge-backed or all unified sections and feeds them into the batch Blender builder |
| `tools/RunCarbonSectionBlendBatch.ps1` | Batch viewer | Builds `.blend` files for multiple Carbon sections in one pass |
| `tools/RunCarbonSectionBlend.ps1` | Primary viewer | Builds a ready-to-open `.blend` for one Carbon section by importing the textured AssetDumper DAE and the matching modelulator scenery section |
| `tools/RunCarbonValidationPack.ps1` | Validation pack builder | Merges a chosen section list into one local-only Blender/Collada validation pack for SR3 import experiments |
| `tools/RunCarbonValidationPackParallel.ps1` | Parallel validation builder | Splits a large section set into chunked packs and runs multiple Blender workers in parallel |
| `tools/RunCarbonMergeValidationPacks.ps1` | Merged slice builder | Merges selected chunked validation packs back into one larger candidate slice |
| `tools/RunCarbonRoadNetworkOverlay.ps1` | Road overlay helper | Opens an existing validation/merged pack and overlays `WRoadNetwork.obj` into a dedicated collection |
| `tools/RunCarbonUnifiedToolkit.ps1` | Primary wrapper | One-command launcher with local default paths for the current Carbon workflow |
| `tools/CarbonUnifiedToolkit.py` | Primary | One entry point that merges raw Binarius sections, modelulator output, AssetDumper DAEs, and optional PNG textures into one normalized bundle |
| `tools/CarbonTrackConverter.py` | Research | Inventories chunked Carbon streaming sections and writes a JSON manifest |
| `tools/SR3TrackBuilder.py` | Conditional | Builds SR3 terrain files only from a real decoded `SR3M` mesh |
| `NFS Docs/CarbonTrackParser_Enhanced.py` | Research | Parses FE track metadata, section manifests, and region bundle information |
| `tools/CarbonTrackConverter` (C#) | Prototype | Old converter path based on invalid raw-vertex assumptions; do not trust for geometry export |

---

## Primary Workflow

Use the unified toolkit first. The other tools now feed into it.

Simplest form:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonUnifiedToolkit.ps1
```

That uses the current local defaults for:
- `D:\Repos\Games\Binarius\Binary\output\L5RA_raw`
- `D:\Games\NFSC Redux\TRACKS\L5RA.BUN`
- `D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator`
- `D:\Repos\Games\NFS-ModTools\AssetDumper\output\STREAML5RA_test`
- `D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator\Textures\STREAML5RA.BUN_PNG`
- output `f:\NFS\NFSC_Mods\W2C\Blender`

```powershell
python tools\CarbonUnifiedToolkit.py `
  --track-id L5RA `
  --out "f:\NFS\NFSC_Mods\W2C\Blender" `
  --raw-input "D:\Repos\Games\Binarius\Binary\output\L5RA_raw" `
  --bun "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" `
  --modelulator "D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator" `
  --assetdumper "D:\Repos\Games\NFS-ModTools\AssetDumper\output\STREAML5RA_test" `
  --png-textures "D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator\Textures\STREAML5RA.BUN_PNG"
```

This writes:
- `L5RA_unified\unified_manifest.json`
- `L5RA_unified\section_solid_bridge.json`
- `L5RA_unified\section_solid_bridge.csv`
- `L5RA_unified\raw_inventory\L5RA.section_manifest.json`
- `L5RA_unified\asset_sections_textured\*.dae`
- `L5RA_unified\textures\*`
- `L5RA_unified\README.txt`

Validated on current `L5RA` data:
- `2171` unified sections
- `2171` patched AssetDumper DAEs
- `3763` materialized textures
- `1891` bridge sections covering `54966` resolved solid references

The bridge outputs add the missing join between Carbon sections and modelulator solids:
- `section_solid_bridge.json` stores the exact `Solids\*.dae` files instanced by each section
- `section_solid_bridge.csv` gives a quick summary with local-vs-external solid counts per section

To inspect one section directly in Blender:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonSectionBlend.ps1 -SectionNumber 1001
```

This writes a `.blend` under:
- `f:\NFS\NFSC_Mods\W2C\Blender\blender_sections`

It imports:
- the textured section DAE from `asset_sections_textured`
- the matching modelulator scenery section

and stores them in separate collections for side-by-side inspection.

The modelulator side is also classified into:
- `Modelulator_Local`
- `Modelulator_External`
- `Modelulator_Unclassified`

based on the section-to-solid bridge.

To build multiple sections in one pass:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonSectionBlendBatch.ps1 -SectionNumbers 1001,1002 -HideModelulatorExternal
```

To process all bridge-backed sections without manually building a list:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonSectionBlendAll.ps1 -Source Bridge -HideModelulatorExternal
```

To process every unified section instead:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonSectionBlendAll.ps1 -Source Unified -HideModelulatorExternal
```

To build one merged local-only validation pack from a chosen section set:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonValidationPack.ps1 -SectionNumbers 1001,1002 -HideAssetDumperReference
```

This writes a pack directory under:
- `f:\NFS\NFSC_Mods\W2C\Blender\validation_packs`

Each pack contains:
- `<pack>.blend`
- `<pack>_local_only.dae`
- `pack_manifest.json`

The `.blend` keeps:
- `Validation_Local` collections containing only modelulator solids that match the raw local-solid bridge
- optional hidden `Validation_AssetReference` collections for textured side-by-side inspection

The exported local-only Collada is the first practical SR3-facing validation artifact from the unified toolkit.

For large runs, use the parallel wrapper instead of one giant pack:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonValidationPackParallel.ps1 -Source Unified -BasePackName sections_all -ChunkSize 50 -MaxParallel 4 -HideAssetDumperReference
```

This writes chunked packs under:
- `f:\NFS\NFSC_Mods\W2C\Blender\validation_packs_parallel\<BasePackName>`

Each worker chunk gets:
- its own pack directory
- its own log under `logs\`
- a final `parallel_manifest.json` summary for the whole run

To merge selected chunk packs back into one larger slice:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonMergeValidationPacks.ps1 -ChunkIndices 1,2,3 -PackName slice_0001_0003
```

This restores each chunk pack to world space using its saved recenter offset, then creates one merged `.blend` and optional local-only `.dae`.

To overlay the exported Carbon road network onto an existing pack:

```powershell
powershell -ExecutionPolicy Bypass -File tools\RunCarbonRoadNetworkOverlay.ps1 -BlendPath "f:\NFS\NFSC_Mods\W2C\Blender\validation_packs_parallel\sections_bridge\merged\slice_0001_0003\slice_0001_0003.blend"
```

This writes a sibling `_roads.blend` file with the imported `WRoadNetwork.obj` in the `Carbon_RoadNetwork` collection.

Current limitation:
- `70` texture refs remain unresolved in the current `L5RA` run and stay recorded in `unified_manifest.json`
- modelulator solids remain the geometry-authoritative export, while textured section DAEs currently come from AssetDumper

---

## What The Raw Inventory Tool Does Now

`tools/CarbonTrackConverter.py` no longer writes a fake `track.mesh`.

It now:
- scans every streamed `DATA.BIN` section under a Binarius output directory
- parses the Carbon chunk tree
- classifies sections as geometry pack, texture pack, collision pack, or other
- parses `GeometryPack` internals down to:
  - `solid_list_container` / `solid_list_header`
  - per-solid `solid_container` / `solid`
  - nested `solid_platform_info`, `solid_mesh_entries`, `solid_index_buffer`, `solid_vertex_buffer`, and `solid_material_name`
- resolves `scenery_infos` solid keys against section-local solids
- reports per-solid mesh metadata such as:
  - solid key and name
  - poly/submesh counts
  - vertex/index buffer byte sizes
  - material names
- optionally inventories nearby `.BUN` files
- writes `<track_id>.section_manifest.json`

Use `--full-geometry-details` only when you explicitly want the full per-solid dump. The default report is still large, but much more manageable than the full-detail variant.

This is the correct baseline for further reverse engineering.

---

## Quick Start

### 1. Extract a Carbon track

Use Binarius to extract the region and stream bundles:

```powershell
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA_region"
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\STREAML5RA.BUN" "output\L5RA_raw"
```

### 2. Inventory the streamed sections

```powershell
python tools\CarbonTrackConverter.py "output\L5RA_raw" "data\tracks\L5RA" L5RA
```

This writes:
- `data/tracks/L5RA/L5RA.section_manifest.json`
- `data/tracks/L5RA/README.txt`

### 3. Use the report for reverse engineering

The manifest tells you which files are:
- geometry sections
- texture sections
- collision sections

It does not produce a usable SR3 mesh yet.

If you need the rawest possible geometry evidence, there is now also a partial decoded export set here:

```text
D:\Repos\Games\Binarius\Binary\output\modelulator
```

Use it as a validation/reference source, not as the sole pipeline.

---

## Why Full Geometry Export Is Still Missing

The streamed Carbon world path is more complex than a raw vertex buffer:
- `GeometryPack` holds chunked scenery data, not a single model blob
- `scenery_infos` reference solid keys rather than embedding one flat mesh
- those solid keys do resolve to real local `0x80134010 -> 0x00134011` solid records in many sections
- `hyperlinked` shows solids use `mesh_entry`, `file_vertex_buffer`, shader-driven strides, and a `compressed_verts` flag
- the renderer creates GPU vertex buffers from already-prepared runtime vertex buffers

That means the missing step is not “read 32-byte vertices”. The missing step is:
1. load Carbon chunk trees correctly
2. resolve scenery infos to shared solids
3. understand or reproduce solid/mesh vertex buffer preparation
4. then export decoded geometry

`modelulator` already gives a partial decoded export path for scenery sections, road networks, and collision, but its current `Solids` output is incomplete in this workspace.

---

## SR3 Builder

`tools/SR3TrackBuilder.py` is still useful later, but only after a real decoded SR3 mesh exists.

It expects:
- `track.mesh` with `SR3M` magic
- valid decoded positions
- real world-space bounds

It will now reject placeholder or invalid mesh data instead of silently building garbage heightmaps.

---

## Recommended Next Steps

1. validate 5-10 representative merged packs in Blender and confirm section joins, scale, and orientation
2. use `modelulator\RoadNetworks\WRoadNetwork.obj` and `CollisionPacks\*.obj` against those packs to derive a first driveable-road reference
3. pick one validation pack and convert it into an SR3-facing intermediate mesh
4. only then feed that mesh into `tools/SR3TrackBuilder.py`

---

## L5RA Analysis Results

Verified on `D:\Repos\Games\Binarius\Binary\output\L5RA_raw`:

```
Section count: 2831
  - geometry_pack: 2253
  - texture_pack: 574
  - collision_pack: 1
  - emitter_system: 1
  - light_source_pack: 1
  - event_trigger_pack: 1
```

**Sample scenery objects from A1:**
- `XOS_BARRELYELLOW_1B_00` - Street barriers
- `XOS_TIREPILEA_1B_00` - Tire stacks
- `XOS_TRAFFICCONEA_1A_00` - Traffic cones
- `XSS_MEDIANPOLEA_1A_00` - Median poles
- `XOS_STREETLIGHTD_1B_00` - Street lights
- `XOS_BUMPERORANGE_1B_00` - Crash barriers
- `XOS_TIRESHOPPOSTB_1B_00` - Tire shop posts

**A1 ScenerySection breakdown:**
- 77 scenery_infos
- 370 scenery_instances
- 369 tree nodes
- 370 light_texture_collections

Output files:
- `d:\Repos\Games\Binarius\Binary\output\L5RA_data\tracks\L5RA.section_manifest.json`
- `d:\Repos\Games\Binarius\Binary\output\L5RA_data\tracks\README.txt`

---

## Useful Repos

- `D:\Repos\Games\NFSC\hyperlinked`
- `D:\Repos\Games\NFSC\nfsc-sdk`
- `D:\Repos\Games\OGVI`
- `D:\Repos\Games\Binarius`

Related docs are under `NFS Docs/`.

---

## License

Same as Stunt Rally 3. See the main project license.
