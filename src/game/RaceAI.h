#pragma once
#include <vector>
#include <OgreVector3.h>
#include "mathvector.h"

/// 🏎️ Racing Line Waypoint
/// Defines a single point on the optimal racing line
struct RacingLinePoint
{
	Ogre::Vector3 position;       // World position
	Ogre::Vector3 normal;         // Track normal (for banking)
	Ogre::Vector3 tangent;        // Direction along racing line
	
	float idealSpeed = 0.0f;      // Optimal speed at this point (m/s)
	float brakePoint = 0.0f;      // Brake pressure (0-1)
	float throttlePoint = 0.0f;   // Throttle pressure (0-1)
	float steerAngle = 0.0f;      // Ideal steering angle (radians)
	
	int checkpoint = 0;           // Associated checkpoint index
	bool isCorner = false;        // True if this is a corner apex
	bool isBrakeZone = false;     // True if this is a braking zone
	
	// AI hints
	float suggestedGear = 0.0f;   // Suggested gear
	float cornerRadius = 0.0f;    // Corner radius (0 = straight)
	float cornerAngle = 0.0f;     // Corner angle in degrees
};

/// 🏁 Racing Line
/// Complete racing line for a track
class RacingLine
{
public:
	RacingLine();
	
	// Load/save racing line
	bool LoadFromXml(const std::string& file);
	bool SaveToXml(const std::string& file) const;
	
	// Generate from track spline
	bool GenerateFromSpline(const std::vector<Ogre::Vector3>& trackPoints,
		const std::vector<float>& widths);
	
	// Query racing line
	const RacingLinePoint& GetPoint(int index) const;
	RacingLinePoint& GetPoint(int index);
	int GetPointCount() const { return (int)points.size(); }
	
	// Find nearest point on racing line
	int FindNearestPoint(const Ogre::Vector3& position) const;
	float GetDistanceToPoint(int pointIndex, const Ogre::Vector3& position) const;
	
	// Get interpolated point
	RacingLinePoint GetInterpolatedPoint(float distance) const;
	
	// Racing line info
	float GetTotalLength() const { return totalLength; }
	int GetCheckpointCount() const;
	
private:
	std::vector<RacingLinePoint> points;
	float totalLength = 0.0f;
	
	// Helper functions
	void CalculateTangents();
	void CalculateIdealSpeeds();
	void DetectCorners();
};

/// 🤖 AI Car State
/// Current state of an AI-controlled car
struct AICarState
{
	int currentPoint = 0;         // Current racing line point index
	int targetPoint = 0;          // Target point to aim for
	float lookAhead = 20.0f;      // Look-ahead distance (meters)
	
	Ogre::Vector3 position;       // Current world position
	Ogre::Vector3 velocity;       // Current velocity
	
	float currentSpeed = 0.0f;    // Current speed (m/s)
	float targetSpeed = 0.0f;     // Target speed (m/s)
	
	float throttle = 0.0f;        // Current throttle (0-1)
	float brake = 0.0f;           // Current brake (0-1)
	float steer = 0.0f;           // Current steer (-1 to 1)
	int gear = 1;                 // Current gear
	
	float distanceOnTrack = 0.0f; // Distance along track (meters)
	float progress = 0.0f;        // Progress (0.0 to 1.0)
	
	int lapsCompleted = 0;        // Laps completed
	int currentPosition = 0;      // Current race position
	int totalRacers = 0;          // Total racers in event
	
	bool isCrashed = false;       // True if AI car is crashed
	float crashTime = 0.0f;       // Time since crash
};

/// 🎯 AI Driving Style
/// Parameters that define AI driving behavior
struct AIDrivingStyle
{
	float aggression = 0.5f;      // 0 = cautious, 1 = aggressive
	float skill = 0.5f;           // 0 = novice, 1 = expert
	float consistency = 0.5f;     // 0 = variable, 1 = consistent
	
	// Derived parameters
	float maxSpeed = 0.0f;        // Maximum speed (m/s)
	float brakeForce = 0.0f;      // Braking force (0-1)
	float corneringSpeed = 0.0f;  // Cornering speed multiplier
	float reactionTime = 0.0f;    // Reaction time (seconds)
	float mistakeChance = 0.0f;   // Chance of mistake (0-1)
	
	// Initialize from aggression/skill
	void Initialize(float agg, float skl, float cons);
};

/// 🚦 Traffic Route Point
/// Defines a point on a traffic route
struct TrafficRoutePoint
{
	Ogre::Vector3 position;
	Ogre::Vector3 direction;
	float speed = 10.0f;          // Traffic speed (m/s)
	float waitTime = 0.0f;        // Wait time at this point (seconds)
	
	int trafficType = 0;          // Type of traffic (car, truck, etc.)
	bool isIntersection = false;  // True if this is an intersection
};

/// 🚗 Traffic Route
/// Defines a complete traffic route
class TrafficRoute
{
public:
	std::vector<TrafficRoutePoint> points;
	std::string id;
	bool isLoop = true;
	
	float GetTotalLength() const;
	TrafficRoutePoint GetInterpolatedPoint(float distance) const;
};

/// 🚦 Traffic System
/// Manages all traffic on the track
class TrafficSystem
{
public:
	static TrafficSystem& Get();
	
	// Initialize
	bool Initialize();
	void Shutdown();
	
	// Load traffic routes
	bool LoadRoutes(const std::string& file);
	const TrafficRoute* GetRoute(const std::string& routeId) const;
	
	// Traffic spawning
	void SpawnTraffic(const std::string& routeId, int count);
	void DespawnTraffic(int trafficId);
	
	// Update traffic
	void Update(float dt);
	
	// Traffic queries
	bool IsPointBlocked(const Ogre::Vector3& position, float radius) const;
	float GetTrafficDensity(const Ogre::Vector3& position) const;
	
private:
	TrafficSystem() = default;
	
	std::vector<TrafficRoute> routes;
	std::map<std::string, int> routeMap;  // routeId -> index
	
	struct ActiveTraffic
	{
		int id = 0;
		std::string routeId;
		float positionOnRoute = 0.0f;
		float speed = 0.0f;
		Ogre::Vector3 position;
		Ogre::Vector3 direction;
		bool active = false;
	};
	
	std::vector<ActiveTraffic> activeTraffic;
};

/// 💨 Drift AI Controller
/// AI behavior specifically for drift events
class DriftAIController
{
public:
	DriftAIController();
	
	// Initialize for drift event
	void Initialize(const RacingLine* racingLine, AIDrivingStyle style);
	
	// Update AI drift behavior
	void Update(float dt, AICarState& state);
	
	// Get drift target (where AI should drift)
	Ogre::Vector3 GetDriftTarget() const { return driftTarget; }
	float GetTargetDriftAngle() const { return targetDriftAngle; }
	
	// Drift state
	bool IsDrifting() const { return isDrifting; }
	float GetCurrentScore() const { return currentScore; }
	
private:
	const RacingLine* pRacingLine = nullptr;
	AIDrivingStyle drivingStyle;
	
	Ogre::Vector3 driftTarget;
	float targetDriftAngle = 0.0f;
	
	bool isDrifting = false;
	float driftStartTime = 0.0f;
	float currentScore = 0.0f;
	float combo = 1.0f;
	
	// Drift zones
	std::vector<int> driftZoneIndices;
	int currentDriftZone = -1;
	
	// AI drift behavior
	void FindDriftZones();
	void EvaluateDriftOpportunity(const AICarState& state);
	void ExecuteDrift(AICarState& state, float dt);
	void EndDrift();
};
