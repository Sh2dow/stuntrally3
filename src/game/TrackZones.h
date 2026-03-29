#pragma once
#include "TrackMetadata.h"
#include <OgreVector3.h>
#include <vector>

class GAME;

/// 🎯 Track Zone Trigger
/// Runtime zone trigger for gameplay effects
struct TrackZoneTrigger
{
	TrackZoneType type = ZONE_RESET;
	
	Ogre::Vector3 position;
	float radius = 10.0f;
	float length = 0.0f;  // along track
	
	int groupKey = 0;
	bool enabled = true;
	
	// Runtime state
	bool playerInside = false;
	float timeInside = 0.0f;
	
	// Parameters
	std::map<std::string, std::string> params;
	
	// Check if point is inside zone
	bool IsInside(const Ogre::Vector3& point) const;
	
	// Get distance to zone center
	float DistanceTo(const Ogre::Vector3& point) const;
};

/// 🚧 Track Barrier Runtime
struct TrackBarrierRuntime
{
	Ogre::Vector3 start, end;
	float height = 3.0f;
	
	int groupKey = 0;
	bool playerOnly = false;
	int handedness = 0;
	bool enabled = true;
	
	// Check if line segment intersects barrier
	bool Intersects(const Ogre::Vector3& p1, const Ogre::Vector3& p2) const;
};

/// 🗺️ Track Zone Manager
/// Manages runtime zone triggers and barriers for a track
class TrackZoneManager
{
public:
	TrackZoneManager();
	
	// Initialize from track metadata
	void Init(const TrackMetadata* track);
	void Clear();
	
	// Update zones (call each frame)
	void Update(float dt, const Ogre::Vector3& carPos, const Ogre::Vector3& carVel);
	
	// Zone queries
	bool IsInZone(TrackZoneType type) const;
	TrackZoneTrigger* GetActiveZone(TrackZoneType type);
	std::vector<TrackZoneTrigger*> GetActiveZones() const;
	
	// Barrier control
	void EnableBarrierGroup(int groupKey);
	void DisableBarrierGroup(int groupKey);
	bool IsBarrierEnabled(int groupKey) const;
	
	// Zone callbacks (override these for effects)
	virtual void OnZoneEnter(TrackZoneTrigger* zone);
	virtual void OnZoneExit(TrackZoneTrigger* zone);
	virtual void OnZoneStay(TrackZoneTrigger* zone, float time);
	
	// Get all zones
	const std::vector<TrackZoneTrigger>& GetAllZones() const { return zones; }
	const std::vector<TrackBarrierRuntime>& GetAllBarriers() const { return barriers; }
	
protected:
	std::vector<TrackZoneTrigger> zones;
	std::vector<TrackBarrierRuntime> barriers;
	
	std::map<int, bool> barrierGroups;  // groupKey -> enabled state
};

/// 🎮 Game Zone Handler
/// Handles zone effects in the game
class GameZoneHandler : public TrackZoneManager
{
public:
	GameZoneHandler(GAME* game);
	
	// Zone effects
	void OnZoneEnter(TrackZoneTrigger* zone) override;
	void OnZoneExit(TrackZoneTrigger* zone) override;
	void OnZoneStay(TrackZoneTrigger* zone, float time) override;
	
private:
	GAME* pGame = nullptr;
	
	// Effect helpers
	void TriggerJumpCamera(TrackZoneTrigger* zone);
	void TriggerVertigoCamera(TrackZoneTrigger* zone);
	void CheckCanyonDrop(TrackZoneTrigger* zone);
};
