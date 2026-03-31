# Carbon Track Extraction with AssetDumper

## Tool Location
`d:\Repos\Games\NFS-ModTools\AssetDumper`

## Supported Games
AssetDumper supports multiple NFS games including Carbon.

## Usage

### Export Carbon Track Bundles

```bash
cd d:\Repos\Games\NFS-ModTools\AssetDumper

# Export L5RA (Casino Tower) district
dotnet run -- export -g Carbon -o "output\L5RA_export" "D:\Games\NFSC Redux\TRACKS\L5RA.BUN"

# Export streaming sections
dotnet run -- export -g Carbon -o "output\STREAML5RA_export" "D:\Games\NFSC Redux\TRACKS\STREAML5RA.BUN"

# Export all track bundles at once
dotnet run -- export -g Carbon -o "output\AllTracks" "D:\Games\NFSC Redux\TRACKS\*.BUN"
```

### Command Line Options

```
-g, --game          The game that the bundle files come from (Carbon)
-o, --output        The directory to export files to
--obj-mode          Object export mode:
                      ExportAll - Export all models individually
                      ExportPacks - Export model packs combined
                      ExportScenerySections - Export scenery sections
--scene-format      Export format: Collada (.dae) or Fbx (.fbx)
--export-lights     Export lights (only valid with ExportScenerySections)
```

### Recommended Settings for Track Export

```bash
# Export as FBX with scenery sections (best for tracks)
dotnet run -- export -g Carbon -o "output\L5RA" --obj-mode ExportScenerySections --scene-format Fbx "D:\Games\NFSC Redux\TRACKS\L5RA.BUN"

# Export as Collada (open format, easier to parse)
dotnet run -- export -g Carbon -o "output\L5RA" --obj-mode ExportScenerySections --scene-format Collada "D:\Games\NFSC Redux\TRACKS\L5RA.BUN"
```

## Expected Output

```
output/L5RA/
├── textures/
│   ├── 0x12345678_texturename.dds
│   ├── 0x87654321_another.dds
│   └── ...
├── 1.dae (or 1.fbx)      # Scenery section 1
├── 2.dae                 # Scenery section 2
├── 3.dae                 # Scenery section 3
└── ...
```

## Conversion Pipeline

### Step 1: Extract with AssetDumper
```bash
dotnet run -- export -g Carbon -o "output\L5RA" --obj-mode ExportScenerySections --scene-format Collada "D:\Games\NFSC Redux\TRACKS\L5RA.BUN"
```

### Step 2: Convert FBX/Collada to SR3 Format
Create converter tool to:
- Parse FBX/Collada scene
- Extract vertex data
- Extract indices
- Extract UV coordinates
- Convert textures (DDS → PNG)
- Generate SR3 `.mesh` format

### Step 3: Generate SR3 Track Files
- `track.ini` - Track metadata
- `track.mesh` - Converted geometry
- `track.col` - Collision (generate from mesh)
- `zones.xml` - Zone triggers (from Carbon zone data)
- `textures/` - Converted textures

## Advantages Over Binarius

1. **Purpose-built for NFS** - Understands NFS chunk formats
2. **Multiple game support** - Works with Carbon, MW, Underground, etc.
3. **Standard formats** - Exports to FBX/Collada (industry standard)
4. **Texture export** - Automatically extracts all textures
5. **Scene graph** - Preserves hierarchy and transforms
6. **Active development** - Part of NFS-ModTools suite

## Next Steps

1. **Test AssetDumper on L5RA.BUN**
   ```bash
   dotnet run -- export -g Carbon -o "output\L5RA_test" --obj-mode ExportScenerySections "D:\Games\NFSC Redux\TRACKS\L5RA.BUN"
   ```

2. **Verify output**
   - Check if FBX/Collada files are valid
   - Verify textures exported correctly
   - Check scene structure

3. **Create FBX/Collada to SR3 converter**
   - Parse FBX using FBXSharp (already in NFS-ModTools)
   - Extract geometry to SR3 format
   - Generate collision from mesh

4. **Integrate with SR3**
   - Add FBX loader to SR3
   - OR convert offline and use converted files

## Code References

From `ExportBundleCommand.cs`:
- Exports scenery sections with transforms
- Handles texture packs
- Supports morph targets
- Can export lights (for Carbon track lighting)

Key classes:
- `SceneExport` - Scene graph
- `GeometryBuilder` - Mesh export
- `TextureBuilder` - Texture export
- `SolidObject` - Model data

## Alternative: Use FBX Directly

Since AssetDumper exports FBX, we could:
1. Export all Carbon tracks to FBX
2. Use FBX SDK to read in SR3
3. Convert to SR3 format at runtime or offline

This might be easier than parsing raw BUN chunks!
