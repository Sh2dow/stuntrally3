#include "pch.h"
#include "TrackZones.h"
#include "game.h"
#include <OgreMath.h>

using namespace Ogre;


//--------------------------------------------------------------------------------------------------------------------------
// TrackZoneTrigger
//--------------------------------------------------------------------------------------------------------------------------

bool TrackZoneTrigger::IsInside(const Vector3& point) const
{
	if (!enabled) return false;
	
	Vector3 delta = point - position;
	float distSq = delta.squaredLength();
	return distSq <= (radius * radius);
}

float TrackZoneTrigger::DistanceTo(const Vector3& point) const
{
	Vector3 delta = point - position;
	return delta.length();
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackBarrierRuntime
//--------------------------------------------------------------------------------------------------------------------------

bool TrackBarrierRuntime::Intersects(const Vector3& p1, const Vector3& p2) const
{
	if (!enabled) return false;
	
	// Simple line-box intersection test
	Vector3 barrierDir = end - start;
	float barrierLen = barrierDir.length();
	
	if (barrierLen < 0.01f) return false;
	
	Vector3 barrierNormal = Vector3(barrierDir.z, 0, -barrierDir.x).normalisedCopy();
	
	// Check if line segment crosses barrier plane
	float d1 = (p1 - start).dotProduct(barrierNormal);
	float d2 = (p2 - start).dotProduct(barrierNormal);
	
	// Different signs = crossed plane
	if (d1 * d2 > 0) return false;
	
	// Check height
	float minY = std::min(p1.y, p2.y);
	float maxY = std::max(p1.y, p2.y);
	
	if (maxY < start.y || minY > start.y + height) return false;
	
	// Check along barrier length
	float t = -d1 / (d2 - d1);
	Vector3 intersect = p1 + (p2 - p1) * t;
	
	Vector3 alongBarrier = intersect - start;
	float distAlong = alongBarrier.dotProduct(barrierDir.normalisedCopy());
	
	return std::abs(distAlong) <= barrierLen * 0.5f;
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackZoneManager
//--------------------------------------------------------------------------------------------------------------------------

TrackZoneManager::TrackZoneManager()
{
}

void TrackZoneManager::Init(const TrackMetadata* track)
{
	Clear();
	
	if (!track) return;
	
	// Convert metadata zones to runtime triggers
	for (const auto& zone : track->zones)
	{
		TrackZoneTrigger trigger;
		trigger.type = zone.type;
		trigger.position = Vector3(zone.position[0], zone.position[1], zone.position[2]);
		trigger.radius = zone.radius;
		trigger.length = zone.length;
		trigger.groupKey = zone.groupKey;
		trigger.enabled = zone.enabled;
		trigger.params = zone.params;
		
		zones.push_back(trigger);
	}
	
	// Convert metadata barriers to runtime barriers
	for (const auto& barrier : track->barriers)
	{
		TrackBarrierRuntime rt;
		rt.start = Vector3(barrier.start[0], barrier.start[1], barrier.start[2]);
		rt.end = Vector3(barrier.end[0], barrier.end[1], barrier.end[2]);
		rt.height = barrier.height;
		rt.groupKey = barrier.groupKey;
		rt.playerOnly = barrier.playerOnly;
		rt.handedness = barrier.handedness;
		rt.enabled = barrier.enabled;
		
		barriers.push_back(rt);
		
		// Initialize barrier group state
		if (barrierGroups.find(barrier.groupKey) == barrierGroups.end())
			barrierGroups[barrier.groupKey] = true;
	}
}

void TrackZoneManager::Clear()
{
	zones.clear();
	barriers.clear();
	barrierGroups.clear();
}

void TrackZoneManager::Update(float dt, const Vector3& carPos, const Vector3& carVel)
{
	// Update all zones
	for (auto& zone : zones)
	{
		if (!zone.enabled) continue;
		
		bool wasInside = zone.playerInside;
		bool isInside = zone.IsInside(carPos);
		
		if (isInside && !wasInside)
		{
			// Entered zone
			zone.playerInside = true;
			zone.timeInside = 0.0f;
			OnZoneEnter(&zone);
		}
		else if (!isInside && wasInside)
		{
			// Exited zone
			zone.playerInside = false;
			OnZoneExit(&zone);
		}
		else if (isInside)
		{
			// Staying in zone
			zone.timeInside += dt;
			OnZoneStay(&zone, zone.timeInside);
		}
	}
}

bool TrackZoneManager::IsInZone(TrackZoneType type) const
{
	for (const auto& zone : zones)
	{
		if (zone.type == type && zone.playerInside)
			return true;
	}
	return false;
}

TrackZoneTrigger* TrackZoneManager::GetActiveZone(TrackZoneType type)
{
	for (auto& zone : zones)
	{
		if (zone.type == type && zone.playerInside)
			return &zone;
	}
	return nullptr;
}

std::vector<TrackZoneTrigger*> TrackZoneManager::GetActiveZones() const
{
	std::vector<TrackZoneTrigger*> active;
	for (const auto& zone : zones)
	{
		if (zone.playerInside)
			active.push_back(const_cast<TrackZoneTrigger*>(&zone));
	}
	return active;
}

void TrackZoneManager::EnableBarrierGroup(int groupKey)
{
	barrierGroups[groupKey] = true;
	
	for (auto& barrier : barriers)
	{
		if (barrier.groupKey == groupKey)
			barrier.enabled = true;
	}
}

void TrackZoneManager::DisableBarrierGroup(int groupKey)
{
	barrierGroups[groupKey] = false;
	
	for (auto& barrier : barriers)
	{
		if (barrier.groupKey == groupKey)
			barrier.enabled = false;
	}
}

bool TrackZoneManager::IsBarrierEnabled(int groupKey) const
{
	auto it = barrierGroups.find(groupKey);
	if (it != barrierGroups.end())
		return it->second;
	return true;
}

void TrackZoneManager::OnZoneEnter(TrackZoneTrigger* zone)
{
	// Base implementation - override in derived classes
}

void TrackZoneManager::OnZoneExit(TrackZoneTrigger* zone)
{
	// Base implementation - override in derived classes
}

void TrackZoneManager::OnZoneStay(TrackZoneTrigger* zone, float time)
{
	// Base implementation - override in derived classes
}


//--------------------------------------------------------------------------------------------------------------------------
// GameZoneHandler
//--------------------------------------------------------------------------------------------------------------------------

GameZoneHandler::GameZoneHandler(GAME* game) : pGame(game)
{
}

void GameZoneHandler::OnZoneEnter(TrackZoneTrigger* zone)
{
	switch (zone->type)
	{
		case ZONE_JUMP_CAMERA:
			TriggerJumpCamera(zone);
			break;
			
		case ZONE_VERTIGO_CAMERA:
			TriggerVertigoCamera(zone);
			break;
			
		case ZONE_CANYON_DROP:
			CheckCanyonDrop(zone);
			break;
			
		default:
			break;
	}
}

void GameZoneHandler::OnZoneExit(TrackZoneTrigger* zone)
{
	// Reset effects when exiting zones
	if (zone->type == ZONE_JUMP_CAMERA || zone->type == ZONE_VERTIGO_CAMERA)
	{
		// Reset camera to normal
		// TODO: Implement camera reset
	}
}

void GameZoneHandler::OnZoneStay(TrackZoneTrigger* zone, float time)
{
	if (zone->type == ZONE_CANYON_DROP)
	{
		CheckCanyonDrop(zone);
	}
}

void GameZoneHandler::TriggerJumpCamera(TrackZoneTrigger* zone)
{
	if (!pGame) return;
	
	// Get camera parameters from zone
	float cameraAngle = 45.0f;
	float duration = 3.0f;
	float fov = 90.0f;
	
	auto it = zone->params.find("cameraAngle");
	if (it != zone->params.end())
		cameraAngle = std::stof(it->second);
	
	it = zone->params.find("duration");
	if (it != zone->params.end())
		duration = std::stof(it->second);
	
	it = zone->params.find("fov");
	if (it != zone->params.end())
		fov = std::stof(it->second);
	
	// TODO: Trigger jump camera with these parameters
	// pGame->cam->SetJumpCamera(zone->position, cameraAngle, duration, fov);
}

void GameZoneHandler::TriggerVertigoCamera(TrackZoneTrigger* zone)
{
	if (!pGame) return;
	
	// Get camera parameters from zone
	float cameraTilt = 60.0f;
	float duration = 5.0f;
	
	auto it = zone->params.find("cameraTilt");
	if (it != zone->params.end())
		cameraTilt = std::stof(it->second);
	
	it = zone->params.find("duration");
	if (it != zone->params.end())
		duration = std::stof(it->second);
	
	// TODO: Trigger vertigo camera
	// pGame->cam->SetVertigoCamera(zone->position, cameraTilt, duration);
}

void GameZoneHandler::CheckCanyonDrop(TrackZoneTrigger* zone)
{
	if (!pGame) return;
	
	// Get fail height from zone
	float failHeight = 50.0f;
	
	auto it = zone->params.find("failHeight");
	if (it != zone->params.end())
		failHeight = std::stof(it->second);
	
	// Check if car is below fail height
	// TODO: Implement canyon drop fail logic
	// if (pGame->cars[0]->GetPosition().y < zone->position.y - failHeight)
	// {
	//     // Trigger reset
	//     pGame->ResetCar(0);
	// }
}
