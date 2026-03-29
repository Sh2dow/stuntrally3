#include "pch.h"
#include "RaceAI.h"
#include "tinyxml2.h"
#include "Def_Str.h"
#include <OgreMath.h>
#include <algorithm>
#include <cmath>

using namespace tinyxml2;


//--------------------------------------------------------------------------------------------------------------------------
// RacingLine
//--------------------------------------------------------------------------------------------------------------------------

RacingLine::RacingLine()
{
}

bool RacingLine::LoadFromXml(const std::string& file)
{
	XMLDocument doc;
	XMLError er = doc.LoadFile(file.c_str());
	if (er != XML_SUCCESS) return false;
	
	XMLElement* root = doc.RootElement();
	if (!root) return false;
	
	points.clear();
	totalLength = 0.0f;
	
	XMLElement* pointElem = root->FirstChildElement("point");
	while (pointElem)
	{
		RacingLinePoint p;
		
		const char* a = pointElem->Attribute("pos");
		if (a)
		{
			Ogre::Vector3 v;
			sscanf(a, "%f,%f,%f", &v.x, &v.y, &v.z);
			p.position = v;
		}
		
		a = pointElem->Attribute("speed");
		if (a) p.idealSpeed = s2r(a);
		
		a = pointElem->Attribute("brake");
		if (a) p.brakePoint = s2r(a);
		
		a = pointElem->Attribute("throttle");
		if (a) p.throttlePoint = s2r(a);
		
		a = pointElem->Attribute("steer");
		if (a) p.steerAngle = s2r(a);
		
		a = pointElem->Attribute("checkpoint");
		if (a) p.checkpoint = s2i(a);
		
		a = pointElem->Attribute("corner");
		if (a) p.isCorner = Ogre::StringConverter::parseBool(a);
		
		a = pointElem->Attribute("brakeZone");
		if (a) p.isBrakeZone = Ogre::StringConverter::parseBool(a);
		
		points.push_back(p);
		pointElem = pointElem->NextSiblingElement("point");
	}
	
	// Calculate derived data
	CalculateTangents();
	CalculateIdealSpeeds();
	DetectCorners();
	
	return !points.empty();
}

bool RacingLine::SaveToXml(const std::string& file) const
{
	XMLDocument xml;
	XMLElement* root = xml.NewElement("racingLine");
	
	for (const auto& p : points)
	{
		XMLElement* pointElem = xml.NewElement("point");
		
		char posBuf[64];
		snprintf(posBuf, sizeof(posBuf), "%.3f,%.3f,%.3f",
			p.position.x, p.position.y, p.position.z);
		pointElem->SetAttribute("pos", posBuf);
		
		pointElem->SetAttribute("speed", fToStr(p.idealSpeed).c_str());
		pointElem->SetAttribute("brake", fToStr(p.brakePoint).c_str());
		pointElem->SetAttribute("throttle", fToStr(p.throttlePoint).c_str());
		pointElem->SetAttribute("steer", fToStr(p.steerAngle).c_str());
		pointElem->SetAttribute("checkpoint", p.checkpoint);
		pointElem->SetAttribute("corner", p.isCorner);
		pointElem->SetAttribute("brakeZone", p.isBrakeZone);
		
		root->InsertEndChild(pointElem);
	}
	
	xml.InsertEndChild(root);
	return xml.SaveFile(file.c_str());
}

bool RacingLine::GenerateFromSpline(const std::vector<Ogre::Vector3>& trackPoints,
	const std::vector<float>& widths)
{
	if (trackPoints.size() < 2) return false;
	
	points.clear();
	
	// Generate racing line points from track spline
	for (size_t i = 0; i < trackPoints.size(); ++i)
	{
		RacingLinePoint p;
		p.position = trackPoints[i];
		
		// Offset to optimal racing line (typically outside-inside-outside)
		// For now, use center of track
		if (i < widths.size())
		{
			// Could offset based on corner direction
			p.position += Ogre::Vector3::ZERO;  // Placeholder
		}
		
		points.push_back(p);
	}
	
	CalculateTangents();
	CalculateIdealSpeeds();
	DetectCorners();
	
	return true;
}

const RacingLinePoint& RacingLine::GetPoint(int index) const
{
	static RacingLinePoint dummy;
	if (index < 0 || index >= (int)points.size()) return dummy;
	return points[index];
}

RacingLinePoint& RacingLine::GetPoint(int index)
{
	return points[index];
}

int RacingLine::FindNearestPoint(const Ogre::Vector3& position) const
{
	if (points.empty()) return -1;
	
	int nearest = 0;
	float nearestDistSq = (position - points[0].position).squaredLength();
	
	for (size_t i = 1; i < points.size(); ++i)
	{
		float distSq = (position - points[i].position).squaredLength();
		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearest = (int)i;
		}
	}
	
	return nearest;
}

float RacingLine::GetDistanceToPoint(int pointIndex, const Ogre::Vector3& position) const
{
	if (pointIndex < 0 || pointIndex >= (int)points.size())
		return 0.0f;
	
	return (position - points[pointIndex].position).length();
}

RacingLinePoint RacingLine::GetInterpolatedPoint(float distance) const
{
	if (points.empty() || totalLength <= 0.0f)
	{
		RacingLinePoint dummy;
		return dummy;
	}
	
	// Wrap distance
	while (distance >= totalLength) distance -= totalLength;
	while (distance < 0) distance += totalLength;
	
	// Find segment
	float accumDist = 0.0f;
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		float segDist = (points[i+1].position - points[i].position).length();
		
		if (distance <= accumDist + segDist)
		{
			float t = (distance - accumDist) / segDist;
			
			RacingLinePoint result;
			result.position = points[i].position + (points[i+1].position - points[i].position) * t;
			result.normal = points[i].normal + (points[i+1].normal - points[i].normal) * t;
			result.tangent = points[i].tangent + (points[i+1].tangent - points[i].tangent) * t;
			result.tangent.normalise();
			
			result.idealSpeed = points[i].idealSpeed + (points[i+1].idealSpeed - points[i].idealSpeed) * t;
			result.brakePoint = points[i].brakePoint + (points[i+1].brakePoint - points[i].brakePoint) * t;
			result.throttlePoint = points[i].throttlePoint + (points[i+1].throttlePoint - points[i].throttlePoint) * t;
			
			return result;
		}
		
		accumDist += segDist;
	}
	
	return points.back();
}

int RacingLine::GetCheckpointCount() const
{
	int maxCkpt = 0;
	for (const auto& p : points)
	{
		if (p.checkpoint > maxCkpt)
			maxCkpt = p.checkpoint;
	}
	return maxCkpt + 1;
}

void RacingLine::CalculateTangents()
{
	if (points.size() < 2) return;
	
	for (size_t i = 0; i < points.size(); ++i)
	{
		size_t prev = (i > 0) ? (i - 1) : (points.size() - 1);
		size_t next = (i < points.size() - 1) ? (i + 1) : 0;
		
		// Tangent is direction from prev to next
		Ogre::Vector3 tangent = points[next].position - points[prev].position;
		tangent.normalise();
		points[i].tangent = tangent;
		
		// Normal is up (could be calculated from track banking)
		points[i].normal = Ogre::Vector3::UNIT_Y;
	}
	
	// Calculate total length
	totalLength = 0.0f;
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		totalLength += (points[i+1].position - points[i].position).length();
	}
}

void RacingLine::CalculateIdealSpeeds()
{
	// Simple speed calculation based on corner sharpness
	for (size_t i = 0; i < points.size(); ++i)
	{
		size_t prev = (i > 0) ? (i - 1) : (points.size() - 1);
		size_t next = (i < points.size() - 1) ? (i + 1) : 0;
		
		// Calculate corner angle
		Ogre::Vector3 v1 = points[i].position - points[prev].position;
		Ogre::Vector3 v2 = points[next].position - points[i].position;
		
		v1.normalise();
		v2.normalise();
		
		float dot = v1.dotProduct(v2);
		float angle = Ogre::Math::ACos(dot).valueDegrees();
		
		// Sharper corners = lower speed
		float baseSpeed = 60.0f;  // Base speed (m/s)
		
		if (angle > 30.0f)
			points[i].idealSpeed = baseSpeed * 0.5f;  // Sharp corner
		else if (angle > 15.0f)
			points[i].idealSpeed = baseSpeed * 0.75f;  // Medium corner
		else
			points[i].idealSpeed = baseSpeed;  // Straight
	}
}

void RacingLine::DetectCorners()
{
	for (size_t i = 0; i < points.size(); ++i)
	{
		size_t prev = (i > 0) ? (i - 1) : (points.size() - 1);
		size_t next = (i < points.size() - 1) ? (i + 1) : 0;
		
		Ogre::Vector3 v1 = points[i].position - points[prev].position;
		Ogre::Vector3 v2 = points[next].position - points[i].position;
		
		v1.normalise();
		v2.normalise();
		
		float dot = v1.dotProduct(v2);
		float angle = Ogre::Math::ACos(dot).valueDegrees();
		
		if (angle > 20.0f)
		{
			points[i].isCorner = true;
			points[i].cornerAngle = angle;
			
			// Mark brake zone before corner
			if (i > 0)
				points[i-1].isBrakeZone = true;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------------------
// AIDrivingStyle
//--------------------------------------------------------------------------------------------------------------------------

void AIDrivingStyle::Initialize(float agg, float skl, float cons)
{
	aggression = agg;
	skill = skl;
	consistency = cons;
	
	// Calculate derived parameters
	maxSpeed = 50.0f + (agg * 30.0f);  // 50-80 m/s
	brakeForce = 0.5f + (skl * 0.5f);  // 0.5-1.0
	corneringSpeed = 0.6f + (skl * 0.4f);  // 0.6-1.0
	reactionTime = 0.5f - (skl * 0.3f);  // 0.2-0.5 seconds
	mistakeChance = 0.1f - (cons * 0.08f);  // 0.02-0.1
}


//--------------------------------------------------------------------------------------------------------------------------
// TrafficRoute
//--------------------------------------------------------------------------------------------------------------------------

float TrafficRoute::GetTotalLength() const
{
	if (points.size() < 2) return 0.0f;
	
	float length = 0.0f;
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		length += (points[i+1].position - points[i].position).length();
	}
	
	return length;
}

TrafficRoutePoint TrafficRoute::GetInterpolatedPoint(float distance) const
{
	if (points.empty())
	{
		TrafficRoutePoint dummy;
		return dummy;
	}
	
	float totalLen = GetTotalLength();
	while (distance >= totalLen) distance -= totalLen;
	while (distance < 0) distance += totalLen;
	
	float accumDist = 0.0f;
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		float segDist = (points[i+1].position - points[i].position).length();
		
		if (distance <= accumDist + segDist)
		{
			float t = (distance - accumDist) / segDist;
			
			TrafficRoutePoint result;
			result.position = points[i].position + (points[i+1].position - points[i].position) * t;
			result.direction = points[i].direction + (points[i+1].direction - points[i].direction) * t;
			result.direction.normalise();
			result.speed = points[i].speed + (points[i+1].speed - points[i].speed) * t;
			
			return result;
		}
		
		accumDist += segDist;
	}
	
	return points.back();
}


//--------------------------------------------------------------------------------------------------------------------------
// TrafficSystem
//--------------------------------------------------------------------------------------------------------------------------

TrafficSystem& TrafficSystem::Get()
{
	static TrafficSystem instance;
	return instance;
}

bool TrafficSystem::Initialize()
{
	// Load default traffic routes
	LoadRoutes("data/traffic/routes.xml");
	
	return true;
}

void TrafficSystem::Shutdown()
{
	routes.clear();
	routeMap.clear();
	activeTraffic.clear();
}

bool TrafficSystem::LoadRoutes(const std::string& file)
{
	XMLDocument doc;
	XMLError er = doc.LoadFile(file.c_str());
	if (er != XML_SUCCESS) return false;
	
	XMLElement* root = doc.RootElement();
	if (!root) return false;
	
	XMLElement* routeElem = root->FirstChildElement("route");
	while (routeElem)
	{
		TrafficRoute route;
		
		const char* a = routeElem->Attribute("id");
		if (a) route.id = std::string(a);
		
		a = routeElem->Attribute("loop");
		if (a) route.isLoop = Ogre::StringConverter::parseBool(a);
		
		XMLElement* pointElem = routeElem->FirstChildElement("point");
		while (pointElem)
		{
			TrafficRoutePoint p;
			
			a = pointElem->Attribute("pos");
			if (a)
			{
				Ogre::Vector3 v;
				sscanf(a, "%f,%f,%f", &v.x, &v.y, &v.z);
				p.position = v;
			}
			
			a = pointElem->Attribute("dir");
			if (a)
			{
				Ogre::Vector3 v;
				sscanf(a, "%f,%f,%f", &v.x, &v.y, &v.z);
				p.direction = v;
			}
			
			a = pointElem->Attribute("speed");
			if (a) p.speed = s2r(a);
			
			route.points.push_back(p);
			pointElem = pointElem->NextSiblingElement("point");
		}
		
		if (!route.id.empty() && !route.points.empty())
		{
			routes.push_back(route);
			routeMap[route.id] = (int)(routes.size() - 1);
		}
		
		routeElem = routeElem->NextSiblingElement("route");
	}
	
	return !routes.empty();
}

const TrafficRoute* TrafficSystem::GetRoute(const std::string& routeId) const
{
	auto it = routeMap.find(routeId);
	if (it != routeMap.end())
		return &routes[it->second];
	return nullptr;
}

void TrafficSystem::SpawnTraffic(const std::string& routeId, int count)
{
	const TrafficRoute* route = GetRoute(routeId);
	if (!route) return;
	
	for (int i = 0; i < count; ++i)
	{
		ActiveTraffic traffic;
		traffic.id = (int)activeTraffic.size();
		traffic.routeId = routeId;
		traffic.positionOnRoute = (float)i * (route->GetTotalLength() / count);
		traffic.speed = 10.0f;
		traffic.active = true;
		
		// Set initial position
		TrafficRoutePoint p = route->GetInterpolatedPoint(traffic.positionOnRoute);
		traffic.position = p.position;
		traffic.direction = p.direction;
		
		activeTraffic.push_back(traffic);
	}
}

void TrafficSystem::DespawnTraffic(int trafficId)
{
	if (trafficId < 0 || trafficId >= (int)activeTraffic.size())
		return;
	
	activeTraffic[trafficId].active = false;
}

void TrafficSystem::Update(float dt)
{
	for (auto& traffic : activeTraffic)
	{
		if (!traffic.active) continue;
		
		const TrafficRoute* route = GetRoute(traffic.routeId);
		if (!route) continue;
		
		// Move traffic along route
		traffic.positionOnRoute += traffic.speed * dt;
		
		// Wrap around if loop
		if (route->isLoop)
		{
			float totalLen = route->GetTotalLength();
			while (traffic.positionOnRoute >= totalLen)
				traffic.positionOnRoute -= totalLen;
		}
		else
		{
			if (traffic.positionOnRoute >= route->GetTotalLength())
			{
				traffic.active = false;
				continue;
			}
		}
		
		// Update position
		TrafficRoutePoint p = route->GetInterpolatedPoint(traffic.positionOnRoute);
		traffic.position = p.position;
		traffic.direction = p.direction;
		traffic.speed = p.speed;
	}
}

bool TrafficSystem::IsPointBlocked(const Ogre::Vector3& position, float radius) const
{
	for (const auto& traffic : activeTraffic)
	{
		if (!traffic.active) continue;
		
		float distSq = (position - traffic.position).squaredLength();
		if (distSq < (radius * radius))
			return true;
	}
	
	return false;
}

float TrafficSystem::GetTrafficDensity(const Ogre::Vector3& position) const
{
	float density = 0.0f;
	float searchRadius = 50.0f;
	float searchRadiusSq = searchRadius * searchRadius;
	
	for (const auto& traffic : activeTraffic)
	{
		if (!traffic.active) continue;
		
		float distSq = (position - traffic.position).squaredLength();
		if (distSq < searchRadiusSq)
			density += 1.0f;
	}
	
	return density;
}


//--------------------------------------------------------------------------------------------------------------------------
// DriftAIController
//--------------------------------------------------------------------------------------------------------------------------

DriftAIController::DriftAIController()
{
}

void DriftAIController::Initialize(const RacingLine* racingLine, AIDrivingStyle style)
{
	pRacingLine = racingLine;
	drivingStyle = style;
	
	// Find drift zones on racing line
	FindDriftZones();
}

void DriftAIController::Update(float dt, AICarState& state)
{
	if (!pRacingLine) return;
	
	// Evaluate if we should drift
	EvaluateDriftOpportunity(state);
	
	// Execute drift behavior
	if (isDrifting)
	{
		ExecuteDrift(state, dt);
	}
	else
	{
		// Normal racing behavior
		state.throttle = state.targetSpeed > state.currentSpeed ? 1.0f : 0.0f;
		state.brake = state.targetSpeed < state.currentSpeed ? 0.8f : 0.0f;
	}
}

void DriftAIController::FindDriftZones()
{
	if (!pRacingLine) return;
	
	driftZoneIndices.clear();
	
	// Find corners suitable for drifting
	for (int i = 0; i < pRacingLine->GetPointCount(); ++i)
	{
		const RacingLinePoint& p = pRacingLine->GetPoint(i);
		
		// Mark corners with angle > 30 degrees as drift zones
		if (p.isCorner && p.cornerAngle > 30.0f)
		{
			driftZoneIndices.push_back(i);
		}
	}
}

void DriftAIController::EvaluateDriftOpportunity(const AICarState& state)
{
	if (!pRacingLine || driftZoneIndices.empty()) return;
	
	// Find nearest drift zone
	int nearestZone = -1;
	float nearestDist = 1000.0f;
	
	for (size_t i = 0; i < driftZoneIndices.size(); ++i)
	{
		int zoneIdx = driftZoneIndices[i];
		const RacingLinePoint& zone = pRacingLine->GetPoint(zoneIdx);
		
		float dist = (zone.position - state.position).length();
		if (dist < nearestDist && dist < 50.0f)  // Within 50m
		{
			nearestDist = dist;
			nearestZone = (int)i;
		}
	}
	
	// Decide to drift if approaching a zone
	if (nearestZone >= 0 && nearestDist < 30.0f && !isDrifting)
	{
		currentDriftZone = nearestZone;
		int zoneIdx = driftZoneIndices[nearestZone];
		const RacingLinePoint& zone = pRacingLine->GetPoint(zoneIdx);
		
		driftTarget = zone.position;
		targetDriftAngle = zone.steerAngle * 1.5f;  // More aggressive for drift
		
		// Start drift
		isDrifting = true;
		driftStartTime = state.distanceOnTrack;
		combo = 1.0f;
	}
	
	// End drift if past zone
	if (isDrifting && nearestZone < 0)
	{
		EndDrift();
	}
}

void DriftAIController::ExecuteDrift(AICarState& state, float dt)
{
	// Maintain drift by applying opposite steer and throttle
	state.steer = -targetDriftAngle;  // Counter-steer
	state.throttle = 0.8f + (drivingStyle.aggression * 0.2f);  // High throttle
	state.brake = 0.0f;  // No braking during drift
	
	// Calculate drift score
	float driftAngle = std::fabs(state.steer);
	float speed = state.currentSpeed;
	
	// Score = angle × speed × combo × time
	float scoreGain = driftAngle * speed * combo * dt;
	currentScore += scoreGain;
	
	// Build combo
	combo += dt * 0.1f;
	float maxCombo = 1.0f + (drivingStyle.skill * 4.0f);  // 1-5x combo
	if (combo > maxCombo) combo = maxCombo;
	
	// Update target
	if (currentDriftZone >= 0 && currentDriftZone < (int)driftZoneIndices.size())
	{
		int zoneIdx = driftZoneIndices[currentDriftZone];
		const RacingLinePoint& zone = pRacingLine->GetPoint(zoneIdx);
		driftTarget = zone.position;
	}
}

void DriftAIController::EndDrift()
{
	isDrifting = false;
	currentDriftZone = -1;
	combo = 1.0f;
	targetDriftAngle = 0.0f;
}
