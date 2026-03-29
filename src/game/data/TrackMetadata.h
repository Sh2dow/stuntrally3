#pragma once
#include <string>
#include <vector>
#include <map>
#include "mathvector.h"

/// 🗺️ Carbon-style Track Zone Types
/// These match Carbon's track_path::zone::type vocabulary
enum TrackZoneType
{
	ZONE_RESET = 0,
	ZONE_GUIDED_RESET,
	ZONE_TUNNEL,
	ZONE_OVERPASS,
	ZONE_STREAMER_PREDICTION,
	ZONE_GARAGE,
	ZONE_TRAFFIC_PATTERN,
	ZONE_DYNAMIC,
	ZONE_NEIGHBORHOOD,
	ZONE_JUMP_CAMERA,       // 🎥 Trigger jump camera
	ZONE_NO_COP_SPAWN,
	ZONE_PURSUIT_START,
	ZONE_HIGHWAY,
	ZONE_CANYON_DROP,       // ⚠️ Fail zone for canyon drops
	ZONE_VERTIGO_CAMERA,    // 🎥 Trigger vertigo camera
	ZONE_COUNT
};

// Zone type names for debugging/config
const static std::string TrackZoneTypeNames[ZONE_COUNT] = {
	"reset",
	"guided_reset",
	"tunnel",
	"overpass",
	"streamer_prediction",
	"garage",
	"traffic_pattern",
	"dynamic",
	"neighborhood",
	"jump_camera",
	"no_cop_spawn",
	"pursuit_start",
	"highway",
	"canyon_drop",
	"vertigo_camera"
};

/// 🚧 Track Barrier
/// Matches Carbon's track_path::barrier structure
struct TrackBarrier
{
	MATHVECTOR<float,3> start;      // Barrier start position
	MATHVECTOR<float,3> end;        // Barrier end position
	float height = 3.0f;            // Barrier height
	
	int groupKey = 0;               // Barrier group for enable/disable
	bool playerOnly = false;        // Player-only barrier
	int handedness = 0;             // Which side is blocked (-1 left, 0 both, 1 right)
	bool enabled = true;            // Current state
	
	//  Serialize from/to XML
	bool LoadXml(class tinyxml2::XMLElement* elem);
	void SaveXml(class tinyxml2::XMLElement* elem, class tinyxml2::XMLDocument& doc) const;
};

/// 🎯 Track Gameplay Zone
/// A zone on the track that triggers gameplay behavior
struct TrackZone
{
	TrackZoneType type = ZONE_RESET;
	
	MATHVECTOR<float,3> position;   // Zone center
	float radius = 10.0f;           // Zone radius
	float length = 0.0f;            // Zone length along track (0 = point)
	
	int groupKey = 0;               // Zone group for enable/disable
	bool enabled = true;            // Current state
	
	//  Optional parameters
	std::map<std::string, std::string> params;
	
	//  Serialize from/to XML
	bool LoadXml(class tinyxml2::XMLElement* elem);
	void SaveXml(class tinyxml2::XMLElement* elem, class tinyxml2::XMLDocument& doc) const;
};

/// 🗺️ Track Metadata (Carbon-style)
/// Stores FE track metadata, minimap paths, and zone/barrier data
class TrackMetadata
{
public:
	TrackMetadata();
	
	//  Core identity
	std::string id;                 // Internal ID (e.g., "L5RA_01", "Test1-Flat")
	std::string displayName;        // Display name (e.g., "Casino Tower")
	std::string artName;            // Art asset name for preview
	std::string region;             // Region ID (e.g., "CASINOTOWN", "SR3_DEFAULT")
	
	//  Engage position (FE camera start)
	MATHVECTOR<float,3> engagePos;  // FE camera position
	float engageYaw = 0.0f;         // FE camera yaw
	
	//  Minimap assets
	std::string minimapLocked;      // Path to locked minimap
	std::string minimapUnlocked;    // Path to unlocked minimap
	std::string trackMap;           // Path to TrackMaps.bin (FE preview)
	
	//  Track stats
	float length = 0.0f;            // Track length (meters)
	int difficulty = 1;             // 1-5 difficulty rating
	int eventTypes = 0;             // Bitmask of allowed event types
	
	//  Progression state
	bool isUnlocked = false;        // Player progression state
	int districtId = 0;             // District/territory ID
	
	//  Gameplay zones and barriers
	std::vector<TrackZone> zones;       // Track zones
	std::vector<TrackBarrier> barriers; // Track barriers
	
	//  Load/Save
	bool LoadXml(const std::string& file);
	bool SaveXml(const std::string& file) const;
	
	//  Helper functions
	std::string GetZoneTypeName(TrackZoneType type) const;
	TrackZoneType GetZoneType(const std::string& name) const;
	
	//  Find zones by type
	std::vector<TrackZone*> GetZonesByType(TrackZoneType type);
	std::vector<const TrackZone*> GetZonesByType(TrackZoneType type) const;
	
	//  Find barriers by group
	std::vector<TrackBarrier*> GetBarriersByGroup(int groupKey);
	std::vector<const TrackBarrier*> GetBarriersByGroup(int groupKey) const;
};

/// 🗄️ Track Database
/// Manages all track metadata
class TrackDatabase
{
public:
	static TrackDatabase& Get();
	
	//  Load/save all track metadata
	bool Load(const std::string& basePath = "data/tracks/");
	bool Save(const std::string& basePath = "data/tracks/") const;
	
	//  Get track by ID
	const TrackMetadata* GetTrack(const std::string& trackId) const;
	TrackMetadata* GetTrack(const std::string& trackId);
	
	//  Get all tracks
	const std::vector<TrackMetadata>& GetAllTracks() const { return tracks; }
	std::vector<TrackMetadata>& GetAllTracks() { return tracks; }
	
	//  Get tracks by region
	std::vector<const TrackMetadata*> GetTracksByRegion(const std::string& region) const;
	
	//  Get tracks by district
	std::vector<const TrackMetadata*> GetTracksByDistrict(int districtId) const;
	
	//  Unlock/lock track
	void UnlockTrack(const std::string& trackId);
	void LockTrack(const std::string& trackId);
	bool IsTrackUnlocked(const std::string& trackId) const;
	
	//  Enable/disable barrier groups
	void EnableBarrierGroup(const std::string& trackId, int groupKey);
	void DisableBarrierGroup(const std::string& trackId, int groupKey);
	
private:
	TrackDatabase() = default;
	
	std::vector<TrackMetadata> tracks;
	std::map<std::string, size_t> trackIndex;  // ID -> index mapping
};
