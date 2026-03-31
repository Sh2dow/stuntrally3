# NFS Carbon Adaptation Docs

This folder collects the first planning pass for pushing SR3 toward a stronger `Need for Speed: Carbon` feel.

- `Carbon-Reference-Inventory.md`
  What the local Carbon reverse-engineering repos already expose.
- `SR3-Carbon-Implementation-Plan.md`
  The SR3 gap analysis, priorities, and phased implementation roadmap.
- `SR3-Carbon-HDR-Luminance-Fix-Plan.md`
  The long-term plan for replacing the temporary HDR luminance placeholder with a real auto-exposure pipeline.
- `NFSC-Track-Usage-Research.md`
  Research notes on how Carbon uses `TRACKS`, `TrackInfo`, `DALWorldMap`, `TrackStreamer`, `track_path`, and `WRoadNetwork`.
- `NFSC-Track-Port-Research.md`
  Late-stage research for full Carbon track import into SR3: bundle extraction, geometry/material conversion, collision, and AI route data.
- `..\tools\CarbonUnifiedToolkit.py`
  Primary collection entry point. It merges raw Binarius sections, modelulator output, AssetDumper section DAEs, and optional PNG textures into one normalized bundle with a single manifest.
- `..\tools\RunCarbonUnifiedToolkit.ps1`
  Thin wrapper around the unified toolkit with local default paths, so the normal workflow becomes one command instead of a long manual argument list.
- `..\tools\RunCarbonSectionBlend.ps1`
  Builds a Blender `.blend` for one Carbon section from the unified bundle, using the local `Blender K-Cycles` install.
- `..\tools\RunCarbonSectionBlendBatch.ps1`
  Batch wrapper for generating `.blend` files for several Carbon sections in one pass.
- `..\tools\RunCarbonSectionBlendAll.ps1`
  One-command wrapper for generating `.blend` files for all bridge-backed sections or all unified sections.
- `..\tools\RunCarbonValidationPack.ps1`
  Merges a chosen section list into one local-only validation pack for SR3-facing Blender and Collada tests.
- `..\tools\CarbonTrackConverter.py`
  Current raw-section parser for Binarius stream dumps. It classifies `GeometryPack`, `TPKBlocks`, and `WCollisionPack` content, resolves local solid keys, and writes a manifest instead of a fake mesh.
- `Implamentation Plan Unsorted.md`
  The now-sorted execution roadmap that merges handling, presentation, track usage, events, AI, progression, and late-stage streaming priorities.

Status on March 29, 2026:

- `ida-pro-mcp` is exposed in this Codex session and resource reads are working.
- Active IDB: `D:\Development\Debug_symbols\IDB_PC\NFSCarbon-v1.4\NFSCarbon-v1.4.i64`
- Module: `NFSC.exe`, base `0x400000`

Additional local Carbon asset source:

- `D:\Games\NFSC Redux`
- `D:\Games\NFSC PS3`
- `D:\Repos\Games\Binarius\Binary\output\modelulator`

Recommended working flow now:

1. Generate or refresh source exports with Binarius, modelulator, and AssetDumper.
2. Run `..\tools\CarbonUnifiedToolkit.py` once for the target region.
3. Use `section_solid_bridge.json` / `section_solid_bridge.csv` to connect Carbon sections to concrete `modelulator\Solids\*.dae`.
4. Use `..\tools\RunCarbonSectionBlend.ps1` when a section needs direct visual inspection in Blender.
5. Use `..\tools\RunCarbonSectionBlendBatch.ps1` when several sections should be staged for review at once.
6. Use `..\tools\RunCarbonSectionBlendAll.ps1` when the whole bridge-backed set or the whole unified set should be staged automatically.
7. Use `..\tools\RunCarbonValidationPack.ps1` when a chosen section set should become one local-only validation pack for SR3-facing import tests.
8. Treat the unified output bundle as the working dataset for research, validation, and future conversion code.
