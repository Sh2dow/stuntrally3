# Carbon W2C Track Porting Guide

## Overview

This guide covers extracting NFS Carbon track data and converting it to SR3 format using:
- **Binarius** - BUN file extraction
- **hyperlinked** - Chunk structure reference
- **ida-pro-mcp** - Live structure verification
- **Custom converters** - Format conversion

---

## Carbon Track File Structure

### Directory Layout
```
TRACKS/
├── L5RA/                    # Casino Tower district
│   ├── TrackMaps.bin        # FE preview map
│   ├── MINI_MAP_CASINOTOWN_LOCKED.bin
│   ├── MINI_MAP_CASINOTOWN_UNLOCKED.bin
│   └── TroughBoundary.bin   # Track boundaries
├── L5RA.BUN                 # Main district bundle
├── STREAML5RA.BUN           # Streaming sections
└── HotPositionL5RA.HOT      # Camera positions
```

### BUN Chunk IDs (from hyperlinked)

| ID | Name | Size | Description |
|----|------|------|-------------|
| `0x5C` | `track_streaming_sections` | 92 bytes/record | Section geometry |
| `0x5D` | `track_streaming_infos` | 64 bytes/record | Section metadata |
| `0x5E` | `track_streaming_barriers` | 48 bytes/record | Barrier definitions |
| `0x5F` | `track_streaming_discs` | 32 bytes/record | Disc bundle refs |
| `0x80034147` | `track_path_manager` | Variable | Racing line, zones |
| `0x00034148` | `track_path_points` | 16 bytes/point | Path 3D points |
| `0x00034149` | `track_path_lanes` | 24 bytes/lane | Lane definitions |
| `0x0003414A` | `track_path_zones` | 32 bytes/zone | Zone triggers |
| `0x0003414D` | `track_path_barriers` | 40 bytes/barrier | Path barriers |
| `0x0E` | `texture_dict` | Variable | Texture archive |
| `0x13` | `model_dict` | Variable | Model archive |
| `0x1A` | `collision_pack` | Variable | Collision meshes |

---

## Step 1: Extract BUN Files with Binarius

### Binarius Usage

**Location:** `d:\Repos\Games\Binarius\CarbonStream.cs`

**Extract District Bundle:**
```bash
cd d:\Repos\Games\Binarius
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA_extracted\"
```

**Extract Streaming Bundle:**
```bash
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\STREAML5RA.BUN" "output\STREAML5RA_extracted\"
```

**Expected Output:**
```
output/L5RA_extracted/
├── 0x5C_track_streaming_sections.bin
├── 0x5D_track_streaming_infos.bin
├── 0x5E_track_streaming_barriers.bin
├── 0x0E_texture_dict.bin
├── 0x13_model_dict.bin
├── 0x1A_collision_pack.bin
└── manifest.json
```

---

## Step 2: Parse Section Records (0x5C)

### Section Record Structure (from IDA)

**Confirmed at `TrackStreamer::FindSection` (0x799ED0):**
- Record size: **92 bytes**
- Linear search through array
- Keyed by 16-bit section number

**Structure:**
```cpp
struct SectionRecord {
    uint16_t section_number;      // 0x00 - Section ID
    uint16_t flags;               // 0x02 - Flags
    uint32_t chunk_offset;        // 0x04 - Offset in BUN
    uint32_t chunk_size;          // 0x08 - Compressed size
    float bounds_min[3];          // 0x0C - Bounding box min
    float bounds_max[3];          // 0x18 - Bounding box max
    uint32_t num_models;          // 0x24 - Model count
    uint32_t model_offset;        // 0x28 - Model array offset
    uint32_t num_textures;        // 0x2C - Texture count
    uint32_t texture_offset;      // 0x30 - Texture offset
    uint32_t collision_id;        // 0x34 - Collision reference
    uint8_t padding[0x5C - 0x38]; // 0x38-0x5C - Reserved
};
```

**Parser Code:**
```python
def parse_section_record(data):
    section = struct.unpack('<HHIIIII IIIII 24x', data)
    return {
        'section_number': section[0],
        'flags': section[1],
        'chunk_offset': section[2],
        'chunk_size': section[3],
        'bounds_min': section[4:7],
        'bounds_max': section[7:10],
        'num_models': section[10],
        'model_offset': section[11],
        'num_textures': section[12],
        'texture_offset': section[13],
        'collision_id': section[14],
    }
```

---

## Step 3: Extract Models (0x13)

### Model Record Structure

**From NFSMWMapLoader.asm reference:**
```cpp
struct ModelRecord {
    uint32_t model_id;          // 0x00
    uint32_t vertex_offset;     // 0x04
    uint32_t vertex_count;      // 0x08
    uint32_t index_offset;      // 0x0C
    uint32_t index_count;       // 0x10
    uint32_t texture_id;        // 0x14
    uint32_t material_flags;    // 0x18
    float bounding_sphere[4];   // 0x1C - xyz + radius
};  // Size: 32 bytes
```

### Vertex Format

**Carbon Vertex (typical):**
```cpp
struct CarbonVertex {
    float position[3];      // 12 bytes
    float normal[3];        // 12 bytes
    float uv[2];            // 8 bytes
    // Total: 32 bytes
};
```

**SR3 Vertex Format:**
```cpp
struct SR3Vertex {
    float position[3];      // 12 bytes
    float normal[3];        // 12 bytes
    float uv[2];            // 8 bytes
    float tangent[4];       // 16 bytes (need to generate)
    // Total: 48 bytes
};
```

**Conversion:**
1. Read Carbon vertices (32 bytes each)
2. Calculate tangents from normal/UV
3. Write SR3 vertices (48 bytes each)

---

## Step 4: Extract Textures (0x0E)

### Texture Record Structure

```cpp
struct TextureRecord {
    uint32_t texture_id;    // 0x00
    uint32_t width;         // 0x04
    uint32_t height;        // 0x08
    uint32_t format;        // 0x0C - DXT1=0, DXT3=1, DXT5=2
    uint32_t data_offset;   // 0x10
    uint32_t data_size;     // 0x14
};  // Size: 24 bytes
```

### Texture Conversion

**DXT Decompression:**
```python
from PIL import Image
import struct

def decompress_dxt(data, width, height, format):
    if format == 0:  # DXT1
        return decompress_dxt1(data, width, height)
    elif format == 1:  # DXT3
        return decompress_dxt3(data, width, height)
    elif format == 2:  # DXT5
        return decompress_dxt5(data, width, height)
    
    # Save as PNG
    img = Image.fromarray(pixels, 'RGBA')
    img.save(f'texture_{texture_id}.png')
```

---

## Step 5: Extract Collision (0x1A)

### Collision Record Structure

```cpp
struct CollisionRecord {
    uint32_t collision_id;    // 0x00
    uint32_t type;            // 0x04 - 1=mesh, 2=box, 3=sphere
    uint32_t data_offset;     // 0x08
    uint32_t data_size;       // 0x0C
    float bounds[6];          // 0x10 - min XYZ, max XYZ
};  // Size: 40 bytes
```

### Mesh Collision Conversion

**Carbon Triangle Mesh:**
```cpp
struct TriangleMesh {
    uint32_t num_triangles;
    float triangles[num_triangles][3][3];  // [tri][vertex][xyz]
};
```

**SR3 Bullet Conversion:**
```cpp
btTriangleMesh* triangleMesh = new btTriangleMesh();

for (auto& tri : carbonMesh.triangles)
{
    btVector3 v0(tri[0][0], tri[0][1], tri[0][2]);
    btVector3 v1(tri[1][0], tri[1][1], tri[1][2]);
    btVector3 v2(tri[2][0], tri[2][1], tri[2][2]);
    
    triangleMesh->addTriangle(v0, v1, v2);
}

btCollisionShape* shape = new btBvhTriangleMeshShape(
    triangleMesh, true, true);
```

---

## Step 6: Extract Track Path (Zones)

### Zone Record Structure (from track_path.hpp)

```cpp
struct ZoneRecord {
    uint32_t zone_type;       // 0x00 - See ZONE_TYPES enum
    float position[3];        // 0x04 - Zone center
    float radius;             // 0x10 - Trigger radius
    uint32_t flags;           // 0x14 - Enable/disable flags
    uint32_t params[4];       // 0x18 - Zone-specific params
};  // Size: 40 bytes
```

### Zone Types Enum

```cpp
enum ZoneType {
    ZONE_RESET = 0,
    ZONE_GUIDED_RESET,
    ZONE_TUNNEL,
    ZONE_OVERPASS,
    ZONE_STREAMER_PREDICTION,
    ZONE_GARAGE,
    ZONE_TRAFFIC_PATTERN,
    ZONE_DYNAMIC,
    ZONE_NEIGHBORHOOD,
    ZONE_JUMP_CAMERA,       // 🎯 Jump camera trigger
    ZONE_NO_COP_SPAWN,
    ZONE_PURSUIT_START,
    ZONE_HIGHWAY,
    ZONE_CANYON_DROP,       // 🎯 Fail zone
    ZONE_VERTIGO_CAMERA     // 🎯 Vertigo camera
};
```

### SR3 Zone Conversion

```python
def convert_zone(carbon_zone):
    sr3_zone = {
        'type': ZONE_TYPES[carbon_zone['type']],
        'position': carbon_zone['position'],
        'radius': carbon_zone['radius'],
        'params': carbon_zone['params'],
    }
    
    # Map Carbon zone types to SR3
    if carbon_zone['type'] == ZONE_JUMP_CAMERA:
        sr3_zone['type'] = 'jump_camera'
        sr3_zone['params'] = {
            'cameraAngle': carbon_zone['params'][0],
            'duration': carbon_zone['params'][1],
        }
    elif carbon_zone['type'] == ZONE_CANYON_DROP:
        sr3_zone['type'] = 'canyon_drop'
        sr3_zone['params'] = {
            'failHeight': carbon_zone['params'][0],
        }
    elif carbon_zone['type'] == ZONE_VERTIGO_CAMERA:
        sr3_zone['type'] = 'vertigo_camera'
        sr3_zone['params'] = {
            'cameraTilt': carbon_zone['params'][0],
        }
    
    return sr3_zone
```

---

## Step 7: Generate SR3 Track Format

### SR3 Track Directory Structure

```
data/tracks/CasinoTower/
├── track.ini           # Track metadata
├── track.mesh          # Converted geometry
├── track.col           # Collision mesh
├── zones.xml           # Zone triggers
├── barriers.xml        # Barrier definitions
├── sections.json       # Section manifest
└── textures/           # Converted textures
    ├── wall_01.png
    ├── road_01.png
    └── ...
```

### track.ini Format

```ini
[track]
name = Casino Tower
length = 3200
difficulty = 3

[events]
sprint = 1
circuit = 1
canyon = 1

[carbon]
original_id = L5RA
district = CasinoTown
sections = 12
```

### zones.xml Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<zones>
    <zone type="jump_camera" pos="125.5,10.2,50.3" radius="15.0">
        <param key="cameraAngle" value="45"/>
        <param key="duration" value="3.0"/>
    </zone>
    
    <zone type="canyon_drop" pos="200.1,5.5,80.2" radius="20.0">
        <param key="failHeight" value="50"/>
    </zone>
    
    <zone type="vertigo_camera" pos="350.8,15.0,150.5" radius="10.0">
        <param key="cameraTilt" value="60"/>
    </zone>
</zones>
```

---

## Step 8: SR3 Loader Integration

### Add Carbon Track Loader

**File:** `src/game/GuiCom_Track.cpp` (or new `CarbonTrackLoader.cpp`)

```cpp
bool LoadCarbonTrack(const std::string& trackPath)
{
    // 1. Load track.ini metadata
    CONFIGFILE cf;
    if (!cf.Load(trackPath + "/track.ini"))
        return false;
    
    // 2. Load converted mesh
    if (!LoadMesh(trackPath + "/track.mesh"))
        return false;
    
    // 3. Load collision
    if (!LoadCollision(trackPath + "/track.col"))
        return false;
    
    // 4. Load zones
    if (!LoadTrackZones(trackPath + "/zones.xml"))
        return false;
    
    // 5. Load textures
    if (!LoadTextures(trackPath + "/textures/"))
        return false;
    
    return true;
}
```

### Update Career Track Assignment

**File:** `data/career/career_tracks.xml`

```xml
<district id="1" name="Casino Tower" requiredRep="0" requiredBosses="0">
    <events>
        <event id="casino_sprint" type="sprint" track="CasinoTower_Section1" 
               laps="1" difficulty="1" rewardCash="300" rewardRep="50"/>
        <event id="casino_circuit" type="circuit" track="CasinoTower_Full" 
               laps="2" difficulty="2" rewardCash="600" rewardRep="120"/>
    </events>
    <boss name="Wolf" car="R3" track="CasinoTower_Full" 
          difficulty="3" rewardCash="2000" rewardRep="500"/>
</district>
```

---

## Implementation Checklist

### Phase 1: Extraction Tools (Week 1)
- [ ] Test Binarius extraction on L5RA.BUN
- [ ] Verify section record structure (92 bytes)
- [ ] Parse model dictionary (0x13)
- [ ] Parse texture dictionary (0x0E)
- [ ] Parse collision pack (0x1A)
- [ ] Parse track path zones (0x0003414A)

### Phase 2: Converters (Week 2)
- [ ] Model format converter (Carbon → SR3)
- [ ] Texture converter (DXT → PNG)
- [ ] Collision converter (Carbon mesh → Bullet)
- [ ] Zone converter (Carbon → SR3 XML)
- [ ] Generate SR3 track.ini

### Phase 3: Integration (Week 3)
- [ ] Add Carbon track loader to SR3
- [ ] Test L5RA (Casino Tower) in SR3
- [ ] Verify collision works
- [ ] Verify zone triggers work
- [ ] Update career_tracks.xml

### Phase 4: Production (Week 4+)
- [ ] Batch convert all 10 districts
- [ ] Test each track in career mode
- [ ] Fix any conversion issues
- [ ] Optimize section streaming (optional)

---

## Key Code References

### From hyperlinked:
- `src/hyperlib/chunk/track_streaming.hpp` - Section chunk definitions
- `src/hyperlib/streamer/track_path.hpp` - Zone types and structures
- `src/hyperlib/streams/world_stream.hpp` - World streaming

### From ida-pro-mcp:
```
TrackStreamer::FindSection at 0x799ED0
- Linear search through 92-byte records
- Keyed by 16-bit section number

TrackStreamer::DetermineStreamingSections
- Section selection logic

WRoadNetwork::ResetRaceSegments at 0x7EAF90
- Road network setup
```

### From NFSMWMapLoader.asm:
- Model loading routines
- Texture decompression
- Bullet collision generation
- Collision query hooks

---

## Testing Pipeline

```bash
# 1. Extract with Binarius
cd d:\Repos\Games\Binarius
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA\"

# 2. Parse with CarbonTrackExtractor
cd d:\Repos\Games\OTHER GAMES\stuntrally3
python tools\CarbonTrackExtractor.py "output\L5RA" "data\tracks\CasinoTower\"

# 3. Convert models/textures
python tools\ConvertCarbonModels.py "data\tracks\CasinoTower\"

# 4. Test in SR3
# Launch SR3, select career, select Casino Tower district
```

---

## Next Action

**Start with Step 1:** Test Binarius extraction on L5RA.BUN and verify the extracted data matches the structures documented here.

```bash
cd d:\Repos\Games\Binarius
# Run extraction
dotnet run -- extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA_test\"

# Check output files
dir output\L5RA_test\
```

Then we can verify the structures and adjust the parsers as needed.
