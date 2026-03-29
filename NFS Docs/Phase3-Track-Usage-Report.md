# SR3 Phase 3: Track Usage Foundation - Implementation Report

## Overview

Phase 3 implements Carbon-style track metadata and zone vocabulary to enable Carbon-feeling track usage without full runtime streaming. This phase is split into three layers:

1. **FE track metadata and previews** ✅
2. **Track-path gameplay zones and barriers** ✅
3. **Runtime streamed sections** (deferred to later phase)

## Files Created

### Core Implementation

| File | Lines | Description |
|------|-------|-------------|
| `src/game/data/TrackMetadata.h` | 179 | Track metadata structures |
| `src/game/data/TrackMetadata.cpp` | 560 | XML load/save, database management |
| `src/game/TrackZones.h` | 156 | Runtime zone trigger system |
| `src/game/TrackZones.cpp` | 412 | Zone effects and callbacks |
| `src/game/TrackPreview.h` | 134 | FE preview/minimap system |
| `src/game/TrackPreview.cpp` | 232 | Preview rendering (stub) |

### Data Files

| File | Description |
|------|-------------|
| `data/tracks/TrackDatabase.xml` | Track database root |
| `data/tracks/Test1-Flat.xml` | Sample track with zones |

### Tools

| File | Description |
|------|-------------|
| `NFS Docs/CarbonTrackParser.py` | Basic Carbon parser |
| `NFS Docs/CarbonTrackParser_Enhanced.py` | Enhanced binary parser |

### Documentation

| File | Description |
|------|-------------|
| `NFS Docs/Slice3-Track-Usage-Plan.md` | Implementation plan |
| `NFS Docs/TrackInfo_Usage.md` | Usage guide |

---

## Phase 3.1: Track Metadata Structure

### TrackMetadata Class

```cpp
class TrackMetadata
{
    // Core identity
    std::string id;              // "L5RA_01", "Test1-Flat"
    std::string displayName;     // "Casino Tower"
    std::string artName;         // Preview asset name
    std::string region;          // "CASINOTOWN"
    
    // FE camera
    MATHVECTOR<float,3> engagePos;
    float engageYaw;
    
    // Minimap assets
    std::string minimapLocked;
    std::string minimapUnlocked;
    std::string trackMap;
    
    // Stats
    float length;
    int difficulty;
    int eventTypes;  // Bitmask
    
    // Progression
    bool isUnlocked;
    int districtId;
    
    // Gameplay data
    std::vector<TrackZone> zones;
    std::vector<TrackBarrier> barriers;
};
```

### Track Zone Types (15 Carbon Types)

```cpp
enum TrackZoneType
{
    ZONE_RESET = 0,
    ZONE_GUIDED_RESET,
    ZONE_TUNNEL,
    ZONE_OVERPASS,
    ZONE_STREAMER_PREDICTION,
    ZONE_GARAGE,              // Garage entry/exit
    ZONE_TRAFFIC_PATTERN,     // Traffic behavior
    ZONE_DYNAMIC,
    ZONE_NEIGHBORHOOD,
    ZONE_JUMP_CAMERA,         // 🎥 Jump camera trigger
    ZONE_NO_COP_SPAWN,
    ZONE_PURSUIT_START,
    ZONE_HIGHWAY,
    ZONE_CANYON_DROP,         // ⚠️ Fail zone
    ZONE_VERTIGO_CAMERA,      // 🎥 Vertigo camera
    ZONE_COUNT
};
```

### Track Barrier Structure

```cpp
struct TrackBarrier
{
    MATHVECTOR<float,3> start;
    MATHVECTOR<float,3> end;
    float height;
    
    int groupKey;         // For enable/disable
    bool playerOnly;
    int handedness;       // -1=left, 0=both, 1=right
    bool enabled;
};
```

### XML Format Example

```xml
<?xml version="1.0" encoding="UTF-8"?>
<track id="Test1-Flat" displayName="Test Flat Track 1" 
       artName="test_flat_01" region="SR3_DEFAULT">
    
    <engagePos pos="10, 5, -10" yaw="45"/>
    
    <minimap locked="data/tracks/Test1-Flat/minimap_locked.png" 
             unlocked="data/tracks/Test1-Flat/minimap_unlocked.png"/>
    
    <stats length="1250.5" difficulty="2" eventTypes="7"/>
    
    <progression isUnlocked="true" districtId="0"/>
    
    <zones>
        <zone type="jump_camera" pos="50, 0, 100" radius="15" group="1">
            <param key="cameraAngle" value="45"/>
            <param key="duration" value="3.0"/>
        </zone>
        
        <zone type="canyon_drop" pos="120, 0, 80" radius="20" length="30" group="2">
            <param key="failHeight" value="50"/>
        </zone>
    </zones>
    
    <barriers>
        <barrier start="5, -10, 0" end="5, 10, 0" height="3" 
                 group="10" playerOnly="true" enabled="false"/>
    </barriers>
</track>
```

---

## Phase 3.2: Zone Vocabulary Integration

### TrackZoneManager

```cpp
class TrackZoneManager
{
    // Initialize from track metadata
    void Init(const TrackMetadata* track);
    
    // Update zones (call each frame)
    void Update(float dt, const Ogre::Vector3& carPos, 
                const Ogre::Vector3& carVel);
    
    // Zone queries
    bool IsInZone(TrackZoneType type) const;
    TrackZoneTrigger* GetActiveZone(TrackZoneType type);
    
    // Barrier control
    void EnableBarrierGroup(int groupKey);
    void DisableBarrierGroup(int groupKey);
    
    // Callbacks (override for effects)
    virtual void OnZoneEnter(TrackZoneTrigger* zone);
    virtual void OnZoneExit(TrackZoneTrigger* zone);
    virtual void OnZoneStay(TrackZoneTrigger* zone, float time);
};
```

### Zone Trigger Detection

```cpp
// In game update loop
zoneManager->Update(dt, carPosition, carVelocity);

if (zoneManager->IsInZone(ZONE_CANYON_DROP))
{
    // Check if player fell below fail height
    if (carPosition.y < failThreshold)
        ResetCar();
}

if (zoneManager->IsInZone(ZONE_JUMP_CAMERA))
{
    // Trigger special camera angle
    camera->SetJumpCamera();
}
```

### Barrier Group Control

```cpp
// Open start gate when race begins
zoneManager->DisableBarrierGroup(10);

// Enable shortcut barriers
if (shortcutUnlocked)
    zoneManager->EnableBarrierGroup(5);
else
    zoneManager->DisableBarrierGroup(5);
```

---

## Phase 3.3: FE Integration

### TrackMinimap Class

```cpp
class TrackMinimap
{
    // Generate minimap from track data
    bool Generate(const TrackMetadata* track, const std::string& outputPath);
    
    // Load existing minimap
    Ogre::TextureGpu* LoadMinimap(const std::string& path);
    
    // Get cached minimap
    Ogre::TextureGpu* GetMinimap(const TrackMetadata* track, bool locked);
};
```

### TrackPreviewRenderer

```cpp
class TrackPreviewRenderer
{
    // Initialize with scene manager
    bool Initialize(Ogre::SceneManager* sceneMgr);
    
    // Render 3D preview
    Ogre::TextureGpu* RenderPreview(const TrackMetadata* track);
    
    // Set camera position
    void SetCameraPosition(const Ogre::Vector3& pos, 
                          const Ogre::Vector3& target);
};
```

### TrackInfoPanel

```cpp
struct TrackInfoPanel
{
    std::string trackName;
    std::string region;
    std::string author;
    
    float trackLength;
    int difficulty;
    int eventTypes;
    
    // Formatted display strings
    std::string lengthStr;        // "2.5 km"
    std::string difficultyStr;    // "★★★☆☆"
    std::vector<std::string> eventIcons;
    
    void Populate(const TrackMetadata* track);
};
```

---

## Phase 3.4: Carbon Parser Enhancement

### Enhanced Binary Parser

```python
class CarbonTrackParser:
    def parse_region(self, region: str) -> CarbonTrackInfo:
        # Parse TrackMaps.bin
        # Parse MINI_MAP_*.bin
        # Parse TroughBoundary.bin (zones/barriers)
        
    def _parse_boundary_file(self, file: Path, track: CarbonTrackInfo):
        # Read zone count
        # Read zones (type, position, radius, length, group)
        # Read barriers (start, end, height, group, flags)
        
    def export_to_sr3(self, output_dir: str, track: CarbonTrackInfo):
        # Export to SR3 TrackMetadata XML format
```

### Usage

```bash
# Parse single region
python CarbonTrackParser_Enhanced.py \
    "D:\Games\NFSC Redux\TRACKS" \
    "data\tracks" L5RA

# Parse all regions
python CarbonTrackParser_Enhanced.py \
    "D:\Games\NFSC Redux\TRACKS" \
    "data\tracks"
```

---

## Integration Guide

### 1. Initialize Track Database

```cpp
// In game initialization
TrackDatabase& db = TrackDatabase::Get();
db.Load("data/tracks/");

// Get track metadata
const TrackMetadata* track = db.GetTrack("Test1-Flat");
if (track)
{
    // Access track info
    std::string name = track->displayName;
    float length = track->length;
}
```

### 2. Initialize Zone Manager

```cpp
// Create zone manager
TrackZoneManager* zoneManager = new TrackZoneManager();
zoneManager->Init(track);

// In game update loop
zoneManager->Update(dt, carPosition, carVelocity);

// Check zone state
if (zoneManager->IsInZone(ZONE_DRIFT))
{
    // Apply drift scoring multiplier
    driftScore *= 1.5f;
}
```

### 3. Control Barriers

```cpp
// Open/close barriers during race
zoneManager->DisableBarrierGroup(10);  // Open start gate

// Check barrier state
if (!zoneManager->IsBarrierEnabled(5))
{
    // Shortcut is open
}
```

### 4. FE Track Selection

```cpp
// Get available tracks
auto tracks = TrackDatabase::Get().GetAllTracks();

for (const auto* track : tracks)
{
    if (!track->isUnlocked)
        continue;  // Skip locked tracks
    
    // Display track card
    TrackInfoPanel panel;
    panel.Populate(track);
    
    ShowTrackCard(panel.trackName, panel.lengthStr, 
                  panel.difficultyStr);
}
```

---

## Known Limitations

1. **No Runtime Carbon Bundle Loading**
   - TrackMaps.bin, minimap .bin files are referenced but not loaded
   - Need separate importer for actual asset loading

2. **Zone Triggers are Basic**
   - Simple sphere-based detection
   - No complex trigger volumes or path-based zones

3. **Barrier Topology Not Integrated**
   - Barriers are data-only
   - Need integration with physics/collision system

4. **Preview Rendering is Stub**
   - TrackPreviewRenderer returns nullptr
   - Full Ogre 2.x integration needed

5. **No Visible Section Support**
   - Carbon's visible section manager not implemented
   - Deferred to full streaming phase

---

## Next Steps

### Immediate (Phase 3.x)
- [ ] Full minimap texture generation
- [ ] 3D track preview rendering
- [ ] Zone trigger visualization
- [ ] Barrier collision integration

### Future Phases
- **Phase 6**: Career and World Map Shell
- **Phase 9**: Full Carbon Streaming/Open World

---

## References

- `NFS Docs/NFSC-Track-Usage-Research.md` - Carbon track architecture
- `NFS Docs/Slice3-Track-Usage-Plan.md` - Original implementation plan
- `hyperlinked/src/hyperlib/streamer/track_path.hpp` - Carbon zone types
- `hyperlinked/src/hyperlib/chunk.hpp` - Carbon chunk IDs
