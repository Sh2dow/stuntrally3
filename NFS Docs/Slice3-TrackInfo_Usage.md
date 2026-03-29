# SR3 Slice 3: Track Usage Foundation - Implementation Status

## ✅ Completed

### Phase 3.1: Track Metadata Structure

**Files Created:**
- `src/game/data/TrackInfo.h` - Core track metadata structures
- `src/game/data/TrackInfo.cpp` - Implementation
- `data/tracks/TrackDatabase.xml` - Track database root file
- `data/tracks/Test1-Flat.xml` - Sample track metadata

**Classes Implemented:**
1. **TrackZoneType** - Enum with all 15 Carbon zone types:
   - reset, guided_reset, tunnel, overpass, streamer_prediction
   - garage, traffic_pattern, dynamic, neighborhood
   - jump_camera, no_cop_spawn, pursuit_start, highway
   - canyon_drop, vertigo_camera

2. **TrackBarrier** - Barrier structure with:
   - Start/end positions
   - Height, group key, player-only flag
   - Handedness (left/right/both)
   - Enable/disable state

3. **TrackZone** - Zone structure with:
   - Zone type
   - Position, radius, length
   - Group key, enabled state
   - Custom parameters (key-value pairs)

4. **TrackInfo** - Track metadata container:
   - Core identity (id, displayName, artName, region)
   - Engage position (FE camera start)
   - Minimap assets (locked/unlocked)
   - Track stats (length, difficulty, eventTypes)
   - Progression state (isUnlocked, districtId)
   - Zones and barriers collections
   - XML load/save methods
   - Query methods (GetZonesByType, GetBarriersByGroup)

5. **TrackDatabase** - Singleton database manager:
   - Load/save track database
   - Get track by ID
   - Get tracks by region/district
   - Unlock/lock tracks
   - Enable/disable barrier groups

### Phase 3.4: Parser Tool

**Files Created:**
- `NFS Docs\CarbonTrackParser.py` - Standalone Carbon track parser

**Features:**
- Parses Carbon `TRACKS\<REGION>\` directories
- Extracts TrackMaps.bin references
- Extracts minimap file paths (locked/unlocked)
- Parses TroughBoundary.bin for zones/barriers (placeholder)
- Exports to SR3 XML format
- Creates TrackDatabase.xml

**Usage:**
```bash
python CarbonTrackParser.py "D:\Games\NFSC Redux\TRACKS" "data\tracks" L5RA
```

## 📋 Documentation

**Files Created:**
- `NFS Docs\Slice3-Track-Usage-Plan.md` - Full implementation plan
- `NFS Docs\TrackInfo_Usage.md` - This file (usage guide)

## 🔧 How to Use

### 1. Load Track Database in Game

```cpp
// In game initialization
TrackDatabase& db = TrackDatabase::Get();
db.Load("data/tracks/");

// Get track info
const TrackInfo* track = db.GetTrack("Test1-Flat");
if (track)
{
    // Access metadata
    std::string name = track->displayName;
    std::string region = track->region;
    
    // Access zones
    auto jumpZones = track->GetZonesByType(ZONE_JUMP_CAMERA);
    for (auto* zone : jumpZones)
    {
        // Trigger jump camera
        TriggerJumpCamera(zone->position, zone->params);
    }
    
    // Access barriers
    auto barriers = track->GetBarriersByGroup(1);
    for (auto* barrier : barriers)
    {
        barrier->enabled = false;  // Open barrier
    }
}
```

### 2. Create New Track Metadata

Create a new XML file in `data/tracks/`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<track id="MyTrack" displayName="My Custom Track" artName="my_track_01" region="MY_REGION">
    <engagePos pos="10, 5, -10" yaw="45"/>
    
    <minimap locked="data/tracks/MyTrack/minimap_locked.png" 
             unlocked="data/tracks/MyTrack/minimap_unlocked.png"/>
    
    <stats length="2500.0" difficulty="3" eventTypes="15"/>
    <progression isUnlocked="false" districtId="1"/>
    
    <zones>
        <zone type="canyon_drop" pos="100, 0, 200" radius="25" length="50" group="1">
            <param key="failHeight" value="100"/>
        </zone>
        
        <zone type="jump_camera" pos="150, 0, 180" radius="15" group="2">
            <param key="cameraAngle" value="60"/>
            <param key="duration" value="2.5"/>
        </zone>
    </zones>
    
    <barriers>
        <barrier start="0, -5, 0" end="0, 5, 0" height="3" group="10" enabled="false"/>
    </barriers>
</track>
```

Add to `TrackDatabase.xml`:
```xml
<track id="MyTrack" file="MyTrack.xml"/>
```

### 3. Parse Carbon Track Data

```bash
# Parse single region
python NFS Docs/CarbonTrackParser.py "D:\Games\NFSC Redux\TRACKS" "data\tracks" L5RA

# Parse all regions
python NFS Docs/CarbonTrackParser.py "D:\Games\NFSC Redux\TRACKS" "data\tracks"
```

## 🎯 Zone Type Usage Examples

### Jump Camera Zone
```xml
<zone type="jump_camera" pos="50, 0, 100" radius="15" group="1">
    <param key="cameraAngle" value="45"/>
    <param key="duration" value="3.0"/>
    <param key="fov" value="90"/>
</zone>
```

### Canyon Drop Fail Zone
```xml
<zone type="canyon_drop" pos="120, 0, 80" radius="20" length="30" group="2">
    <param key="failHeight" value="50"/>
    <param key="respawnPos" value="100, 0, 85"/>
</zone>
```

### Vertigo Camera Zone
```xml
<zone type="vertigo_camera" pos="200, 0, 150" radius="10" group="1">
    <param key="cameraTilt" value="60"/>
    <param key="duration" value="5.0"/>
</zone>
```

### Garage Zone
```xml
<zone type="garage" pos="0, 0, 0" radius="25" group="0">
    <param key="garageId" value="main_garage"/>
</zone>
```

### Traffic Pattern Zone
```xml
<zone type="traffic_pattern" pos="80, 0, 50" radius="100" group="0">
    <param key="trafficDensity" value="0.7"/>
    <param key="pattern" value="highway"/>
</zone>
```

## 🚧 Barrier Group Usage

Barrier groups allow enabling/disabling barriers dynamically:

```cpp
// In race controller
void RaceController::StartRace()
{
    // Open start gate
    TrackDatabase::Get().DisableBarrierGroup(currentTrackId, 10);
}

void RaceController::EnableShortcut(bool enabled)
{
    if (enabled)
        TrackDatabase::Get().DisableBarrierGroup(currentTrackId, 5);
    else
        TrackDatabase::Get().EnableBarrierGroup(currentTrackId, 5);
}
```

## 📊 Event Types Bitmask

The `eventTypes` field is a bitmask:
```cpp
enum EventType : int {
    EVENT_SPRINT = 1,      // 0b0001
    EVENT_CIRCUIT = 2,     // 0b0010
    EVENT_DRIFT = 4,       // 0b0100
    EVENT_CANYON = 8,      // 0b1000
    EVENT_PURSUIT = 16,    // 0b10000
};

// Example: Track allows sprint, circuit, and drift
track->eventTypes = EVENT_SPRINT | EVENT_CIRCUIT | EVENT_DRIFT;  // = 7
```

## 🔄 Integration Points

### With Career System
```cpp
// In CareerXml.h - extend CareerRace
class CareerRace
{
    // ... existing fields ...
    
    // New: Track metadata reference
    std::string trackId;
    const TrackInfo* trackInfo;  // Cached pointer
    
    void LoadTrackInfo()
    {
        trackInfo = TrackDatabase::Get().GetTrack(trackId);
    }
};
```

### With Road System
```cpp
// In Road_File.cpp - load zones from track metadata
void Road::LoadZones(const TrackInfo* track)
{
    for (const auto& zone : track->zones)
    {
        // Create road marker/trigger
        RoadMarker* marker = new RoadMarker();
        marker->type = ToRoadMarkerType(zone.type);
        marker->position = zone.position;
        marker->radius = zone.radius;
        markers.push_back(marker);
    }
}
```

### With GUI (FE Track Selection)
```cpp
// In Gui_Games.cpp
void GuiGames::ShowTrackSelection()
{
    const auto& tracks = TrackDatabase::Get().GetAllTracks();
    
    for (const auto* track : tracks)
    {
        if (!track->isUnlocked)
            continue;  // Skip locked tracks
        
        // Display track with minimap
        ShowTrackCard(
            track->displayName,
            track->minimapUnlocked,
            track->difficulty,
            track->length
        );
    }
}
```

## ⚠️ Known Limitations

1. **No Runtime Carbon Bundle Loading**
   - TrackMaps.bin, minimap .bin files are referenced but not loaded
   - Need separate importer for actual asset loading

2. **Zone Triggers Not Implemented**
   - Zone data structures exist but runtime trigger logic is not implemented
   - Need to integrate with game's trigger/collision system

3. **Barrier Topology Not Integrated**
   - Barriers are data-only, not connected to physics/collision
   - Need to integrate with road collision system

4. **No Visible Section Support**
   - Carbon's visible section manager not implemented
   - Deferred to later phase (full streaming)

## 📝 Next Steps

1. **Zone Trigger System** - Implement runtime zone detection and callbacks
2. **Barrier Collision** - Connect barriers to physics/collision system
3. **Minimap Renderer** - Load and render minimap assets in FE
4. **Track Preview** - Load TrackMaps.bin for FE track preview
5. **Carbon Parser Enhancement** - Full binary parsing of Carbon zone/barrier data

## 📚 References

- `NFS Docs/NFSC-Track-Usage-Research.md` - Carbon track architecture
- `NFS Docs/Slice3-Track-Usage-Plan.md` - Implementation plan
- `hyperlinked/src/hyperlib/streamer/track_path.hpp` - Carbon zone types
- `src/game/data/TrackInfo.h` - API reference
