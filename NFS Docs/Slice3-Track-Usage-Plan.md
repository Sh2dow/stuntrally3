# SR3 Slice 3: Track Usage Foundation - Implementation Plan

## Goal

Implement Carbon-style track metadata and zone vocabulary to enable Carbon-feeling track usage without full runtime streaming.

## Scope

This slice focuses on **Track Usage Layer 1 & 2**:
1. ✅ FE track metadata and previews (Layer 1)
2. ✅ Track-path gameplay zones and barriers (Layer 2)
3. ❌ Runtime streamed sections (Layer 3 - deferred to later)

## Deliverables

### D1: Track Metadata Structure
Add Carbon-style track metadata to SR3's career system:
- Track display name
- Track art name (for preview)
- Event/track ID
- Region assignment
- Minimap paths (locked/unlocked)
- Engage position (FE camera start)
- Track length/difficulty stats

### D2: Carbon Zone Vocabulary
Add zone types to SR3's road/track system:
- `jump_camera` - Jump camera trigger zones
- `canyon_drop` - Fail zone for canyon drops
- `vertigo_camera` - Vertigo camera trigger
- `traffic_pattern` - Traffic behavior zone
- `garage` - Garage entry/exit zone
- `barrier_group` - Barrier enable/disable groups

### D3: Track Preview Integration
- Load TrackMaps.bin-style data
- Display track preview in FE
- Show locked/unlocked minimaps

### D4: Parser/Importer Tool
- Standalone tool to read Carbon track metadata
- Export to SR3-compatible format
- No runtime Carbon bundle loading yet

## Implementation Order

### Phase 3.1: Track Metadata (Week 1-2)
1. Add `TrackInfo` class to store metadata
2. Extend `CareerRace` with track metadata
3. Add region/district system
4. Create track database loader

### Phase 3.2: Zone Vocabulary (Week 2-3)
1. Add zone type enum to road system
2. Extend road marker system with zones
3. Add barrier group support
4. Create zone trigger system

### Phase 3.3: FE Integration (Week 3-4)
1. Add track preview display to FE
2. Implement minimap locked/unlocked state
3. Add engage position handling
4. Connect to world map shell

### Phase 3.4: Parser Tool (Week 4)
1. Build standalone Carbon track parser
2. Export to SR3 format
3. Test with sample Carbon tracks

## Technical Approach

### Track Metadata Storage
```cpp
class TrackInfo {
    std::string id;              // Internal ID (e.g., "L5RA_01")
    std::string displayName;     // Display name (e.g., "Casino Tower")
    std::string artName;         // Art asset name
    std::string region;          // Region ID (e.g., "CASINOTOWN")
    
    MATHVECTOR<float,3> engagePos;  // FE camera position
    float engageYaw;                // FE camera yaw
    
    std::string minimapLocked;      // Path to locked minimap
    std::string minimapUnlocked;    // Path to unlocked minimap
    std::string trackMap;           // Path to TrackMaps.bin
    
    float length;                   // Track length (meters)
    int difficulty;                 // 1-5 difficulty rating
    int eventTypes;                 // Bitmask of allowed event types
    
    bool isUnlocked;                // Player progression state
};
```

### Zone Type Enum
```cpp
enum TrackZoneType {
    ZONE_RESET = 0,
    ZONE_GUIDED_RESET,
    ZONE_TUNNEL,
    ZONE_OVERPASS,
    ZONE_STREAMER_PREDICTION,
    ZONE_GARAGE,
    ZONE_TRAFFIC_PATTERN,
    ZONE_DYNAMIC,
    ZONE_NEIGHBORHOOD,
    ZONE_JUMP_CAMERA,
    ZONE_NO_COP_SPAWN,
    ZONE_PURSUIT_START,
    ZONE_HIGHWAY,
    ZONE_CANYON_DROP,
    ZONE_VERTIGO_CAMERA,
    ZONE_COUNT
};
```

### Barrier Structure
```cpp
struct TrackBarrier {
    MATHVECTOR<float,3> start;
    MATHVECTOR<float,3> end;
    float height;
    int groupKey;           // Barrier group for enable/disable
    bool playerOnly;        // Player-only barrier
    int handedness;         // Which side is blocked
    bool enabled;           // Current state
};
```

## SR3 File Modifications

### Core Data
- `src/game/data/CareerXml.h` - Add TrackInfo class
- `src/game/data/CareerXml.cpp` - Track metadata loading/saving
- `src/game/data/TrackDatabase.h` - New: track database manager
- `src/game/data/TrackDatabase.cpp` - New: track database implementation

### Road System
- `src/road/SplineBase.h` - Add zone type enum
- `src/road/Road_File.cpp` - Load zone/barrier data from track files
- `src/road/Road_Prepass.cpp` - Zone preprocessing
- `src/road/Road_Markers.cpp` - Add zone markers

### GUI/FE
- `src/game/Gui_Games.cpp` - Track preview display
- `src/game/Gui_InitGames.cpp` - Track selection with metadata
- `src/game/Gui_ProgressLoad.cpp` - Track loading with metadata

### New Files
- `src/game/data/TrackInfo.h/cpp` - Track metadata structure
- `src/game/data/TrackZone.h` - Zone type definitions
- `src/game/data/TrackBarrier.h` - Barrier definitions
- `tools/CarbonTrackParser.py` - Standalone parser tool

## Carbon Asset Integration

### What We Use (Phase 3)
✅ `TRACKS\<REGION>\TrackMaps.bin` - FE preview maps
✅ `TRACKS\<REGION>\MINI_MAP_*_LOCKED.bin` - Locked minimaps
✅ `TRACKS\<REGION>\MINI_MAP_*_UNLOCKED.bin` - Unlocked minimaps
✅ Track metadata (names, IDs, regions, engage positions)
✅ Track-path zone definitions
✅ Track-path barrier definitions

### What We Defer
❌ `TRACKS\STREAM*.BUN` - Runtime streaming bundles
❌ `TRACKS\<REGION>.BUN` - Full region bundles
❌ Visible section manager
❌ Topology/scenery group system
❌ Full WRoadNetwork integration

## Success Criteria

### Functional
- [ ] Track metadata loads from career/track files
- [ ] Track preview displays in FE
- [ ] Minimap shows locked/unlocked state
- [ ] Zone triggers work in-game (jump camera, canyon drop, etc.)
- [ ] Barriers can be enabled/disabled by group

### Technical
- [ ] No runtime Carbon bundle loading required
- [ ] Track metadata is data-driven (XML or similar)
- [ ] Zone system is extensible for new zone types
- [ ] Parser tool can extract Carbon track data

### Gameplay
- [ ] One Carbon-style track with all zone types working
- [ ] FE shows Carbon-style track selection
- [ ] Minimap matches Carbon visual style

## Risks & Mitigations

### Risk: Carbon format complexity
**Mitigation**: Start with SR3-native format, build parser as separate tool

### Risk: Road system changes break existing tracks
**Mitigation**: Make zone/barrier data optional, existing tracks continue to work

### Risk: Scope creep into full streaming
**Mitigation**: Explicitly defer STREAM*.BUN support to later phase

## Timeline

| Week | Focus | Deliverables |
|------|-------|--------------|
| 1-2 | Track Metadata | TrackInfo class, database, XML loading |
| 2-3 | Zone Vocabulary | Zone types, barriers, triggers |
| 3-4 | FE Integration | Track preview, minimaps, engage positions |
| 4 | Parser Tool | Carbon track parser, export tool |

## Next Steps After Slice 3

1. **Slice 4: Event Mode Layer** - Sprint/circuit wrappers, drift scoring, canyon duel
2. **Slice 5: Racing AI** - Racing lines, AI behavior, traffic
3. **Slice 6: Career/World Map** - District ownership, boss progression, territory map

## References

- `NFS Docs/NFSC-Track-Usage-Research.md` - Detailed Carbon track architecture
- `NFS Docs/Implementation Plan.md` - Overall SR3 to Carbon plan
- `hyperlinked/src/hyperlib/streamer/track_path.hpp` - Carbon track path structures
- `hyperlinked/src/hyperlib/chunk.hpp` - Carbon chunk IDs
