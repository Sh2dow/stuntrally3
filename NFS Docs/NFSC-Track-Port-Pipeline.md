# NFS Carbon Track Porting Pipeline

## Available Resources

### Tools & Codebases
| Resource | Location | Purpose |
|----------|----------|---------|
| **Binarius** | `d:\Repos\Games\Binarius` | Carbon `.BUN` extractor/repacker (`CarbonStream.cs`) |
| **NFSMWMapLoader** | `d:\Repos\Games\Maps\NFSMWMapLoader\NFSMWMapLoader.asm` | Runtime model/texture/collision loading reference |
| **hyperlinked** | `d:\Repos\Games\NFSC\hyperlinked` | Carbon chunk definitions, managers, enums |
| **ida-pro-mcp** | Active | Live Carbon.exe reverse engineering |
| **CarbonTrackParser_Enhanced.py** | `NFS Docs/` | Extracts track metadata, streaming sections |

---

## Phase 1: Carbon Track Metadata (✅ Already Implemented)

**Status:** Complete - extracts FE data without full geometry

**What Works:**
- ✅ `TrackMaps.bin` inventory (FE preview maps)
- ✅ `MINI_MAP_*.bin` (locked/unlocked minimaps)
- ✅ `TroughBoundary.bin` (track boundaries)
- ✅ Track path zones from `track_path_*` chunks
- ✅ Streaming section records from `TRACKS\<REGION>.BUN`

**Output:**
- `career_tracks.xml` - Career event assignments
- Track metadata (name, region, difficulty, length)

**Next:** Move to geometry extraction

---

## Phase 2: Carbon Track Geometry Extraction

### Step 2.1: Extract BUN Contents with Binarius

**Tool:** `Binarius/CarbonStream.cs`

**Process:**
```bash
# Extract region bundle
CarbonStream.exe extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA\"

# Extract streaming sections
CarbonStream.exe extract "D:\Games\NFSC Redux\TRACKS\STREAML5RA.BUN" "output\STREAML5RA\"
```

**Output Files:**
```
output/L5RA/
├── 0x5C_section_records.bin    # Section geometry records
├── 0x0E_texture_dict.bin       # Textures
├── 0x13_model_dict.bin         # 3D models
├── 0x1A_collision.bin          # Collision meshes
└── metadata.json               # Chunk inventory
```

**Key Chunk IDs (from hyperlinked):**
| ID | Name | Description |
|----|------|-------------|
| `0x5C` | `track_streaming_sections` | Track geometry sections |
| `0x5D` | `track_streaming_infos` | Section metadata |
| `0x5E` | `track_streaming_barriers` | Barrier definitions |
| `0x5F` | `track_streaming_discs` | Disc bundle refs |
| `0x80034147` | `track_path_manager` | Racing line, zones |
| `0x0E` | `texture_dict` | Texture dictionary |
| `0x13` | `model_dict` | Model dictionary |
| `0x1A` | `collision_pack` | Collision meshes |

---

### Step 2.2: Parse Section Records

**Reference:** `ida-pro-mcp` + `hyperlinked/src/hyperlib/chunk/track_streaming.hpp`

**Section Record Structure (0x5C):**
```cpp
struct SectionRecord {
    uint32_t section_id;      // Unique section ID
    uint32_t chunk_offset;    // Offset in BUN
    uint32_t chunk_size;      // Compressed size
    float bounds_min[3];      // Bounding box min
    float bounds_max[3];      // Bounding box max
    uint32_t num_models;      // Model count
    uint32_t model_offset;    // Model array offset
};
```

**IDA Confirmation:**
```
TrackStreamer::FindSection at 0x799ED0
- Linear search through 92-byte records
- Keyed by 16-bit section number
```

**Action:**
1. Use `CarbonStream` to extract raw section data
2. Parse section records using structure above
3. Extract model references for each section

---

### Step 2.3: Extract Models & Textures

**Reference:** `NFSMWMapLoader.asm` - Model loading routines

**Model Structure (0x13):**
```cpp
struct ModelRecord {
    uint32_t model_id;
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t texture_id;
    uint32_t material_flags;
};
```

**Texture Structure (0x0E):**
```cpp
struct TextureRecord {
    uint32_t texture_id;
    uint32_t width;
    uint32_t height;
    uint32_t format;  // DXT1, DXT3, DXT5
    uint32_t data_offset;
    uint32_t data_size;
};
```

**Conversion Process:**
1. Extract vertex/index buffers from BUN
2. Convert vertex format (Carbon → SR3)
   - Carbon: Position (3x float), Normal (3x float), UV (2x float)
   - SR3: Position (3x float), Normal (3x float), UV (2x float), Tangent (4x float)
3. Decompress textures (DXT → PNG)
4. Generate SR3 mesh format

---

### Step 2.4: Extract Collision

**Reference:** `NFSMWMapLoader.asm` - Bullet collision generation

**Collision Structure (0x1A):**
```cpp
struct CollisionRecord {
    uint32_t collision_id;
    uint32_t type;  // 1=mesh, 2=box, 3=sphere
    uint32_t data_offset;
    float bounds[6];
};
```

**Conversion:**
- **Mesh collision:** Extract triangle mesh → Convert to Bullet `btBvhTriangleMeshShape`
- **Box collision:** Extract min/max → Convert to `btBoxShape`
- **Sphere collision:** Extract center/radius → Convert to `btSphereShape`

**SR3 Integration:**
```cpp
// In SR3 Road loader
btCollisionShape* shape = new btBvhTriangleMeshShape(
    triangleMesh, true, true);
```

---

### Step 2.5: Extract Road/Path Data

**Reference:** `hyperlinked/src/hyperlib/streamer/track_path.hpp`

**Track Path Chunks:**
| Chunk ID | Name | Data |
|----------|------|------|
| `0x00034148` | `track_path_points` | 3D points along racing line |
| `0x00034149` | `track_path_lanes` | Lane definitions |
| `0x0003414A` | `track_path_zones` | Zone triggers (jump, canyon, etc.) |
| `0x0003414D` | `track_path_barriers` | Barrier enable/disable |

**Zone Types (from track_path.hpp):**
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
    ZONE_JUMP_CAMERA,      // 🎯 Jump camera trigger
    ZONE_NO_COP_SPAWN,
    ZONE_PURSUIT_START,
    ZONE_HIGHWAY,
    ZONE_CANYON_DROP,      // 🎯 Fail zone
    ZONE_VERTIGO_CAMERA    // 🎯 Vertigo camera
};
```

**SR3 Integration:**
```cpp
// Convert Carbon zones to SR3 TrackZones
for (const auto& carbonZone : carbonZones)
{
    TrackZone sr3Zone;
    sr3Zone.position = carbonZone.position;
    sr3Zone.radius = carbonZone.radius;
    
    switch (carbonZone.type)
    {
        case ZONE_JUMP_CAMERA:
            sr3Zone.type = ZONE_JUMP_CAMERA;
            break;
        case ZONE_CANYON_DROP:
            sr3Zone.type = ZONE_CANYON_DROP;
            break;
        case ZONE_VERTIGO_CAMERA:
            sr3Zone.type = ZONE_VERTIGO_CAMERA;
            break;
    }
    
    track.zones.push_back(sr3Zone);
}
```

---

## Phase 3: SR3 Track Format Conversion

### SR3 Track Structure

**Existing SR3 Format:**
```
data/tracks/<TrackName>/
├── track.ini          # Track metadata
├── track.mesh         # Road geometry
├── track.col          # Collision mesh
├── textures/          # Track textures
└── objects.ini        # Scenery objects
```

### Conversion Pipeline

```
Carbon BUN Files
    ↓ (Binarius extract)
Raw Carbon Assets
    ↓ (Custom converter)
SR3 Format Files
    ↓ (SR3 loader)
In-Game Track
```

### Converter Tool Structure

**Create:** `tools/CarbonTrackConverter/`

**Files:**
```
CarbonTrackConverter/
├── CarbonTrackConverter.cpp    # Main converter
├── BunExtractor.h/cpp          # BUN file parsing
├── ModelConverter.h/cpp        # Model format conversion
├── TextureConverter.h/cpp      # DXT → PNG conversion
├── CollisionConverter.h/cpp    # Collision mesh conversion
└── RoadExtractor.h/cpp         # Road/path extraction
```

**Usage:**
```bash
CarbonTrackConverter.exe "D:\Games\NFSC Redux\TRACKS\L5RA" "data/tracks/CasinoTower/"
```

**Output:**
```
data/tracks/CasinoTower/
├── track.ini
├── track.mesh
├── track.col
├── textures/
│   ├── wall_01.png
│   ├── road_01.png
│   └── ...
└── objects.ini
```

---

## Phase 4: SR3 Integration

### Step 4.1: Update Career Tracks XML

**File:** `data/career/career_tracks.xml`

**Update with real Carbon tracks:**
```xml
<district id="1" name="Downtown" requiredRep="0" requiredBosses="0">
    <events>
        <event id="downtown_1" type="sprint" track="L5RA_Section1" laps="1" difficulty="1"/>
        <event id="downtown_2" type="sprint" track="L5RA_Section2" laps="1" difficulty="2"/>
        <event id="downtown_3" type="circuit" track="L5RA_Full" laps="2" difficulty="2"/>
    </events>
    <boss name="Wolf" car="R3" track="L5RA_Full" difficulty="3"/>
</district>
```

### Step 4.2: Track Loading in SR3

**Update:** `src/game/GuiCom_Track.cpp` or similar track loader

**Add Carbon track loader:**
```cpp
bool LoadCarbonTrack(const std::string& carbonTrackPath)
{
    // 1. Load converted track assets
    if (!LoadMesh(carbonTrackPath + "/track.mesh"))
        return false;
    
    // 2. Load collision
    if (!LoadCollision(carbonTrackPath + "/track.col"))
        return false;
    
    // 3. Load textures
    LoadTextures(carbonTrackPath + "/textures/");
    
    // 4. Setup track zones from Carbon data
    LoadTrackZones(carbonTrackPath + "/zones.xml");
    
    return true;
}
```

---

## Phase 5: Optional - Runtime Section Streaming

**Only implement after full track import works**

**Reference:** `hyperlinked/src/hyperlib/streamer/track_streaming.hpp`

**Streaming Manager:**
```cpp
class CarbonTrackStreamer
{
public:
    void LoadSection(int sectionId);
    void UnloadSection(int sectionId);
    void Update(float dt, const Vector3& playerPos);
    
private:
    std::vector<StreamingSection> sections;
    std::vector<int> loadedSections;
};
```

**SR3 Integration:**
- Replace SR3's full-track loading with section-based loading
- Load/unload sections based on player position
- Manage section transitions

---

## Implementation Priority

### Immediate (Week 1-2):
1. ✅ Use `CarbonTrackParser_Enhanced.py` to extract metadata
2. ✅ Create `career_tracks.xml` with Carbon track assignments
3. ⏳ Implement basic BUN extraction with Binarius
4. ⏳ Convert 1 test track (L5RA - Casino Tower)

### Short-term (Week 3-4):
5. Build model/texture converter
6. Build collision converter
7. Integrate first Carbon track into SR3
8. Test career mode with real Carbon track

### Medium-term (Month 2):
9. Convert all 10 district tracks
10. Add road/path extraction
11. Implement zone triggers (jump, canyon, vertigo)
12. Full career mode with Carbon tracks

### Long-term (Month 3+):
13. Optional section streaming
14. Traffic route extraction
15. Pursuit route extraction

---

## Key Code References

### From hyperlinked:
- `src/hyperlib/chunk/track_streaming.hpp` - Section chunk definitions
- `src/hyperlib/streamer/track_path.hpp` - Zone types, path data
- `src/hyperlib/streams/world_stream.hpp` - World streaming

### From ida-pro-mcp:
- `TrackStreamer::FindSection` at `0x799ED0` - Section lookup
- `TrackStreamer::DetermineStreamingSections` - Section selection
- `WRoadNetwork::ResetRaceSegments` - Road network setup

### From NFSMWMapLoader.asm:
- Model loading routines
- Texture decompression
- Bullet collision generation
- Collision query hooks

---

## Next Actions

1. **Test Binarius extraction** on `L5RA.BUN`
2. **Verify section record structure** matches IDA
3. **Create converter tool skeleton** in `tools/CarbonTrackConverter/`
4. **Convert first test track** (L5RA Section 1)
5. **Load in SR3** and verify geometry/collision

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| BUN format changes | High | Use Binarius as reference, reverse with IDA |
| Vertex format mismatch | Medium | Analyze MW loader, test conversion |
| Texture format issues | Low | Use existing DXT decompressors |
| Collision generation | Medium | Reference MW loader Bullet code |
| Road path extraction | High | Use `track_path_*` chunks from hyperlinked |

---

## Success Criteria

- ✅ Can extract Carbon track sections from BUN files
- ✅ Can convert models to SR3 format
- ✅ Can generate collision meshes
- ✅ Can load converted track in SR3
- ✅ Career mode uses real Carbon tracks
- ✅ All 10 districts have working tracks
