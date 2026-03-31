# Carbon W2C Track Porting - Quick Start

## Tools Ready

You have everything needed to port Carbon tracks:

| Tool | Location | Purpose |
|------|----------|---------|
| **Binarius** | `d:\Repos\Games\Binarius` | Extract `.BUN` files |
| **hyperlinked** | `d:\Repos\Games\NFSC\hyperlinked` | Chunk definitions |
| **ida-pro-mcp** | Active | Live structure verification |
| **CarbonTrackExtractor.py** | `tools/` | Parse sections/zones |
| **CarbonModelConverter.py** | `tools/` | Convert models/textures |

---

## Step-by-Step: Port First Track (L5RA - Casino Tower)

### Step 1: Extract with Binarius

```bash
cd d:\Repos\Games\Binarius

# Extract district bundle
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA_raw\"

# Extract streaming sections
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\STREAML5RA.BUN" "output\STREAML5RA_raw\"
```

**Expected Output:**
```
output/L5RA_raw/
├── 0x5C_track_streaming_sections.bin
├── 0x5D_track_streaming_infos.bin
├── 0x0E_texture_dict.bin
├── 0x13_model_dict.bin
├── 0x1A_collision_pack.bin
└── manifest.json
```

---

### Step 2: Parse Sections & Zones

```bash
cd d:\Repos\Games\OTHER GAMES\stuntrally3

python tools\CarbonTrackExtractor.py "output\L5RA_raw" "data\tracks\CasinoTower\"
```

**Output:**
```
data/tracks/CasinoTower/
├── inventory.json      # Chunk inventory
├── sections.json       # Section manifest
├── zones.xml           # Zone triggers
├── barriers.xml        # Barriers
└── track.ini           # Track metadata
```

---

### Step 3: Convert Models & Textures

```bash
python tools\CarbonModelConverter.py "output\L5RA_raw" "data\tracks\CasinoTower\"
```

**Output:**
```
data/tracks/CasinoTower/
├── track.mesh          # Converted geometry
├── materials.ini       # Material definitions
└── textures/           # PNG textures
    ├── tex_00000001.png
    ├── tex_00000002.png
    └── ...
```

---

### Step 4: Generate Collision

**TODO:** Create collision converter (see `Carbon-W2C-Track-Porting-Guide.md` Step 5)

For now, generate simple collision from mesh bounds.

---

### Step 5: Test in SR3

1. **Update career_tracks.xml:**
```xml
<district id="1" name="Casino Tower">
    <event type="sprint" track="CasinoTower" laps="1" difficulty="1"/>
    <boss name="Wolf" car="R3" track="CasinoTower" difficulty="3"/>
</district>
```

2. **Launch SR3**

3. **Open Career Window**
   - Main Menu → Career
   - Should see "1. Downtown" (Casino Tower)

4. **Select District**
   - Click district button
   - Should show available events

5. **Start Race**
   - Select sprint event
   - Should load CasinoTower track

---

## File Structure Reference

### Carbon BUN Chunks

| ID | Name | Size | Used For |
|----|------|------|----------|
| `0x5C` | `track_streaming_sections` | 92 bytes/record | Track geometry |
| `0x5D` | `track_streaming_infos` | 64 bytes/record | Section metadata |
| `0x0E` | `texture_dict` | Variable | Textures (DXT) |
| `0x13` | `model_dict` | Variable | Models (vertices/indices) |
| `0x1A` | `collision_pack` | Variable | Collision meshes |
| `0x0003414A` | `track_path_zones` | 32 bytes/zone | Zone triggers |
| `0x0003414D` | `track_path_barriers` | 40 bytes/barrier | Barriers |

### SR3 Track Format

```
data/tracks/<TrackName>/
├── track.ini           # Metadata (name, length, difficulty)
├── track.mesh          # Geometry (custom SR3 format)
├── track.col           # Collision (Bullet format)
├── zones.xml           # Zone triggers (jump, canyon, vertigo)
├── barriers.xml        # Barrier definitions
├── materials.ini       # Material assignments
└── textures/           # PNG textures
```

---

## Known Structures

### Section Record (92 bytes) - from IDA
```
0x00: uint16 section_number
0x02: uint16 flags
0x04: uint32 chunk_offset
0x08: uint32 chunk_size
0x0C: float bounds_min[3]
0x18: float bounds_max[3]
0x24: uint32 num_models
0x28: uint32 model_offset
0x2C: uint32 num_textures
0x30: uint32 texture_offset
0x34: uint32 collision_id
0x38-0x5C: padding
```

### Model Record (32 bytes) - from NFSMW reference
```
0x00: uint32 model_id
0x04: uint32 vertex_offset
0x08: uint32 vertex_count
0x0C: uint32 index_offset
0x10: uint32 index_count
0x14: uint32 texture_id
0x18: uint32 material_flags
0x1C: float bounding_sphere[4]
```

### Texture Record (24 bytes)
```
0x00: uint32 texture_id
0x04: uint32 width
0x08: uint32 height
0x0C: uint32 format (0=DXT1, 1=DXT3, 2=DXT5)
0x10: uint32 data_offset
0x14: uint32 data_size
```

### Zone Record (32 bytes) - from hyperlinked
```
0x00: uint32 zone_type (see ZONE_TYPES enum)
0x04: float position[3]
0x10: float radius
0x14: uint32 flags
0x18: uint32 params[4]
```

**ZONE_TYPES:**
```python
0: 'reset'
1: 'guided_reset'
2: 'tunnel'
3: 'overpass'
4: 'streamer_prediction'
5: 'garage'
6: 'traffic_pattern'
7: 'dynamic'
8: 'neighborhood'
9: 'jump_camera'       # 🎯
10: 'no_cop_spawn'
11: 'pursuit_start'
12: 'highway'
13: 'canyon_drop'      # 🎯
14: 'vertigo_camera'   # 🎯
```

---

## Troubleshooting

### Binarius Extraction Fails
- Check BUN file exists
- Verify Binarius built successfully
- Try different Carbon track (L5RA, L3RA, etc.)

### Structure Mismatch
- Use `ida-pro-mcp` to verify structure sizes
- Check `hyperlinked` for latest definitions
- Adjust parser structures in Python tools

### Model Conversion Issues
- Verify vertex format (may differ per track)
- Check texture format codes
- Use IDA to examine actual model loading code

### SR3 Doesn't Load Track
- Check `track.ini` format
- Verify `track.mesh` header
- Check file paths in `materials.ini`
- Review SR3 log for errors

---

## Next Steps After First Track

Once L5RA (Casino Tower) works:

1. **Batch Process All Districts:**
   ```bash
   for district in L5RA L3RA L1RA L4RA L2RA; do
       # Extract
       dotnet run -- extract "TRACKS\$district.BUN" "output\$district\"
       
       # Parse
       python CarbonTrackExtractor.py "output\$district" "data\tracks\$district\"
       
       # Convert
       python CarbonModelConverter.py "output\$district" "data\tracks\$district\"
   done
   ```

2. **Update Career:**
   - Assign each district to career tier
   - Set appropriate difficulty/rewards
   - Configure boss battles

3. **Test Full Career:**
   - Start from district 1
   - Complete all events
   - Unlock all districts
   - Defeat all bosses

---

## Documentation

- **`Carbon-W2C-Track-Porting-Guide.md`** - Complete technical guide
- **`NFSC-Track-Usage-Research.md`** - Carbon track architecture
- **`SR3-Carbon-Implementation-Plan.md`** - Overall project plan
- **`Career-Status-NextSteps.md`** - Career mode status

---

## Ready to Start?

**First command:**
```bash
cd d:\Repos\Games\Binarius
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA_raw\"
```

Then we can verify the extraction and adjust parsers as needed!
