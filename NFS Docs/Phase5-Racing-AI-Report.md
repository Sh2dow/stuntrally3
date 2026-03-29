# SR3 Phase 5: Racing AI and Traffic Foundation - Implementation Report

## Overview

Phase 5 implements the core Racing AI and Traffic system for SR3, including racing line data structures, AI car controllers, look-ahead steering, target speed logic, drift event AI behavior, and traffic routes.

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `src/game/RaceAI.h` | 234 | AI system declarations |
| `src/game/RaceAI.cpp` | 717 | Full AI implementation |

---

## Racing Line System

### RacingLinePoint Structure

```cpp
struct RacingLinePoint
{
    Ogre::Vector3 position;       // World position
    Ogre::Vector3 normal;         // Track normal (for banking)
    Ogre::Vector3 tangent;        // Direction along racing line
    
    float idealSpeed;             // Optimal speed (m/s)
    float brakePoint;             // Brake pressure (0-1)
    float throttlePoint;          // Throttle pressure (0-1)
    float steerAngle;             // Ideal steering angle (radians)
    
    int checkpoint;               // Associated checkpoint index
    bool isCorner;                // True if corner apex
    bool isBrakeZone;             // True if braking zone
    
    // AI hints
    float suggestedGear;
    float cornerRadius;
    float cornerAngle;
};
```

### RacingLine Class

```cpp
class RacingLine
{
    // Load/save
    bool LoadFromXml(const std::string& file);
    bool SaveToXml(const std::string& file) const;
    
    // Generate from track spline
    bool GenerateFromSpline(const std::vector<Ogre::Vector3>& trackPoints,
                           const std::vector<float>& widths);
    
    // Query racing line
    const RacingLinePoint& GetPoint(int index) const;
    int GetPointCount() const;
    
    // Find nearest point
    int FindNearestPoint(const Ogre::Vector3& position) const;
    float GetDistanceToPoint(int pointIndex, 
                            const Ogre::Vector3& position) const;
    
    // Get interpolated point
    RacingLinePoint GetInterpolatedPoint(float distance) const;
    
    // Info
    float GetTotalLength() const;
    int GetCheckpointCount() const;
};
```

### Racing Line XML Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<racingLine>
    <point pos="0.000,0.000,0.000" speed="60.0" brake="0.0" 
           throttle="1.0" steer="0.0" checkpoint="0" 
           corner="false" brakeZone="false"/>
    
    <point pos="10.000,0.000,5.000" speed="40.0" brake="0.8" 
           throttle="0.0" steer="0.5" checkpoint="1" 
           corner="true" brakeZone="false"/>
    
    <point pos="20.000,5.000,5.000" speed="35.0" brake="0.0" 
           throttle="0.5" steer="0.8" checkpoint="1" 
           corner="true" brakeZone="true"/>
</racingLine>
```

### Auto-Generation from Track Spline

```cpp
bool RacingLine::GenerateFromSpline(
    const std::vector<Ogre::Vector3>& trackPoints,
    const std::vector<float>& widths)
{
    if (trackPoints.size() < 2) return false;
    
    points.clear();
    
    // Generate points from track spline
    for (size_t i = 0; i < trackPoints.size(); ++i)
    {
        RacingLinePoint p;
        p.position = trackPoints[i];
        
        // Offset to optimal racing line
        // (outside-inside-outside for corners)
        if (i < widths.size())
        {
            // Calculate optimal line offset
            p.position += CalculateRacingOffset(i, trackPoints, widths);
        }
        
        points.push_back(p);
    }
    
    // Calculate derived data
    CalculateTangents();
    CalculateIdealSpeeds();
    DetectCorners();
    
    return true;
}
```

### Corner Detection

```cpp
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
```

### Ideal Speed Calculation

```cpp
void RacingLine::CalculateIdealSpeeds()
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
        
        float baseSpeed = 60.0f;  // Base speed (m/s)
        
        if (angle > 30.0f)
            points[i].idealSpeed = baseSpeed * 0.5f;  // Sharp corner
        else if (angle > 15.0f)
            points[i].idealSpeed = baseSpeed * 0.75f;  // Medium corner
        else
            points[i].idealSpeed = baseSpeed;  // Straight
    }
}
```

---

## AI Car Controller

### AICarState Structure

```cpp
struct AICarState
{
    int currentPoint;           // Current racing line point
    int targetPoint;            // Target point
    float lookAhead;            // Look-ahead distance (meters)
    
    Ogre::Vector3 position;     // World position
    Ogre::Vector3 velocity;     // Current velocity
    
    float currentSpeed;         // Speed (m/s)
    float targetSpeed;          // Target speed (m/s)
    
    float throttle;             // 0-1
    float brake;                // 0-1
    float steer;                // -1 to 1
    int gear;                   // Current gear
    
    float distanceOnTrack;      // Distance along track (meters)
    float progress;             // 0.0 to 1.0
    
    int lapsCompleted;
    int currentPosition;
    int totalRacers;
    
    bool isCrashed;
    float crashTime;
};
```

### AIDrivingStyle

```cpp
struct AIDrivingStyle
{
    float aggression;       // 0 = cautious, 1 = aggressive
    float skill;            // 0 = novice, 1 = expert
    float consistency;      // 0 = variable, 1 = consistent
    
    // Derived parameters
    float maxSpeed;         // Maximum speed (m/s)
    float brakeForce;       // Braking force (0-1)
    float corneringSpeed;   // Cornering speed multiplier
    float reactionTime;     // Reaction time (seconds)
    float mistakeChance;    // Chance of mistake (0-1)
    
    // Initialize from aggression/skill
    void Initialize(float agg, float skl, float cons)
    {
        aggression = agg;
        skill = skl;
        consistency = cons;
        
        maxSpeed = 50.0f + (agg * 30.0f);      // 50-80 m/s
        brakeForce = 0.5f + (skl * 0.5f);      // 0.5-1.0
        corneringSpeed = 0.6f + (skl * 0.4f);  // 0.6-1.0
        reactionTime = 0.5f - (skl * 0.3f);    // 0.2-0.5 seconds
        mistakeChance = 0.1f - (cons * 0.08f); // 0.02-0.1
    }
};
```

### AI Driving Personalities

| Name | Aggression | Skill | Consistency | Description |
|------|------------|-------|-------------|-------------|
| Cautious Novice | 0.2 | 0.3 | 0.8 | Slow but consistent |
| Aggressive Rookie | 0.8 | 0.4 | 0.3 | Fast but makes mistakes |
| Balanced Racer | 0.5 | 0.5 | 0.5 | Average across all |
| Skilled Veteran | 0.6 | 0.8 | 0.7 | Fast and consistent |
| Carbon Boss | 0.9 | 0.9 | 0.9 | Maximum difficulty |

---

## Look-ahead Steering

### Basic AI Update Loop

```cpp
void UpdateAI(float dt, AICarState& state, const RacingLine& line)
{
    // Find current position on racing line
    state.currentPoint = line.FindNearestPoint(state.position);
    
    // Calculate target point (look-ahead)
    float lookAheadDist = state.lookAhead;
    state.targetPoint = state.currentPoint;
    
    for (int i = 0; i < line.GetPointCount(); ++i)
    {
        int checkPoint = (state.currentPoint + i) % line.GetPointCount();
        const RacingLinePoint& p = line.GetPoint(checkPoint);
        
        float dist = (p.position - state.position).length();
        if (dist >= lookAheadDist)
        {
            state.targetPoint = checkPoint;
            break;
        }
    }
    
    // Get target point data
    const RacingLinePoint& target = line.GetPoint(state.targetPoint);
    
    // Calculate steering
    Ogre::Vector3 toTarget = target.position - state.position;
    toTarget.y = 0;  // Ignore height
    toTarget.normalise();
    
    Ogre::Vector3 forward = state.velocity;
    forward.y = 0;
    forward.normalise();
    
    float dot = forward.dotProduct(toTarget);
    float angle = Ogre::Math::ACos(dot).valueDegrees();
    
    // Determine steer direction
    Ogre::Vector3 cross = forward.crossProduct(toTarget);
    state.steer = (cross.y > 0) ? angle : -angle;
    state.steer /= 45.0f;  // Normalize to -1 to 1
    state.steer = std::max(-1.0f, std::min(1.0f, state.steer));
    
    // Calculate target speed
    state.targetSpeed = target.idealSpeed;
    
    // Apply throttle/brake
    if (state.currentSpeed < state.targetSpeed)
    {
        state.throttle = 1.0f;
        state.brake = 0.0f;
    }
    else
    {
        state.throttle = 0.0f;
        state.brake = target.brakePoint;
    }
}
```

---

## Target Speed Logic

### Speed Profile

```cpp
void CalculateTargetSpeed(AICarState& state, 
                         const RacingLinePoint& target)
{
    // Base target speed from racing line
    float baseSpeed = target.idealSpeed;
    
    // Adjust for driving style
    baseSpeed *= (0.8f + (drivingStyle.aggression * 0.4f));
    
    // Adjust for cornering
    if (target.isCorner)
    {
        baseSpeed *= drivingStyle.corneringSpeed;
    }
    
    // Adjust for weather/conditions (future)
    // baseSpeed *= GetTractionMultiplier();
    
    state.targetSpeed = baseSpeed;
}
```

### Braking Logic

```cpp
void ApplyBraking(AICarState& state, 
                 const RacingLinePoint& current,
                 const RacingLinePoint& target)
{
    // Check if we need to brake for upcoming corner
    if (target.isBrakeZone || target.isCorner)
    {
        float distToCorner = (target.position - state.position).length();
        float brakingDistance = CalculateBrakingDistance(
            state.currentSpeed, target.idealSpeed);
        
        if (distToCorner <= brakingDistance)
        {
            state.brake = drivingStyle.brakeForce;
            state.throttle = 0.0f;
        }
    }
    
    // Emergency braking if off-track
    if (IsOffTrack(state.position))
    {
        state.brake = 1.0f;
        state.throttle = 0.0f;
    }
}
```

### Braking Distance Calculation

```cpp
float CalculateBrakingDistance(float currentSpeed, float targetSpeed)
{
    // Simple physics: v² = u² + 2as
    // s = (v² - u²) / 2a
    
    float deceleration = 8.0f;  // m/s² (typical car braking)
    
    float u2 = currentSpeed * currentSpeed;
    float v2 = targetSpeed * targetSpeed;
    
    float distance = (u2 - v2) / (2.0f * deceleration);
    
    return std::max(0.0f, distance);
}
```

---

## Drift AI Controller

### DriftAIController Class

```cpp
class DriftAIController
{
    // Initialize for drift event
    void Initialize(const RacingLine* racingLine, AIDrivingStyle style);
    
    // Update AI drift behavior
    void Update(float dt, AICarState& state);
    
    // Get drift target
    Ogre::Vector3 GetDriftTarget() const;
    float GetTargetDriftAngle() const;
    
    // Drift state
    bool IsDrifting() const;
    float GetCurrentScore() const;
    
private:
    void FindDriftZones();
    void EvaluateDriftOpportunity(const AICarState& state);
    void ExecuteDrift(AICarState& state, float dt);
    void EndDrift();
    
    const RacingLine* pRacingLine;
    AIDrivingStyle drivingStyle;
    
    Ogre::Vector3 driftTarget;
    float targetDriftAngle;
    
    bool isDrifting;
    float currentScore;
    float combo;
    
    std::vector<int> driftZoneIndices;
    int currentDriftZone;
};
```

### Drift Zone Detection

```cpp
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
```

### Drift Evaluation

```cpp
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
        if (dist < nearestDist && dist < 50.0f)
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
        targetDriftAngle = zone.steerAngle * 1.5f;  // More aggressive
        
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
```

### Drift Execution

```cpp
void DriftAIController::ExecuteDrift(AICarState& state, float dt)
{
    // Maintain drift by applying opposite steer and throttle
    state.steer = -targetDriftAngle;  // Counter-steer
    state.throttle = 0.8f + (drivingStyle.aggression * 0.2f);
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
}
```

### Drift Scoring Formula

```
score = drift_angle × speed × combo × time

Where:
- drift_angle: Absolute steer value (0-1 radians)
- speed: Current speed (m/s)
- combo: Multiplier (1.0-5.0x) based on skill and duration
- time: Time delta (seconds)

Combo builds at 0.1 per second, capped by:
maxCombo = 1.0 + (skill × 4.0)

Example:
- Drift angle: 0.8 rad
- Speed: 20 m/s
- Combo: 3.0x
- Duration: 2.0 seconds

score = 0.8 × 20 × 3.0 × 2.0 = 96.0 points
```

---

## Traffic System

### TrafficRoute Structure

```cpp
struct TrafficRoutePoint
{
    Ogre::Vector3 position;
    Ogre::Vector3 direction;
    float speed;          // Traffic speed (m/s)
    float waitTime;       // Wait time at this point (seconds)
    
    int trafficType;      // 0=car, 1=truck, 2=van
    bool isIntersection;  // True if intersection
};

class TrafficRoute
{
public:
    std::vector<TrafficRoutePoint> points;
    std::string id;
    bool isLoop;
    
    float GetTotalLength() const;
    TrafficRoutePoint GetInterpolatedPoint(float distance) const;
};
```

### TrafficSystem Singleton

```cpp
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
};
```

### Traffic Spawning

```cpp
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
        TrafficRoutePoint p = route->GetInterpolatedPoint(
            traffic.positionOnRoute);
        traffic.position = p.position;
        traffic.direction = p.direction;
        
        activeTraffic.push_back(traffic);
    }
}
```

### Traffic Update

```cpp
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
        TrafficRoutePoint p = route->GetInterpolatedPoint(
            traffic.positionOnRoute);
        traffic.position = p.position;
        traffic.direction = p.direction;
        traffic.speed = p.speed;
    }
}
```

### Traffic Avoidance

```cpp
bool TrafficSystem::IsPointBlocked(const Ogre::Vector3& position, 
                                   float radius) const
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
```

---

## Integration Guide

### 1. Initialize Racing AI

```cpp
// Load racing line
RacingLine* racingLine = new RacingLine();
racingLine->LoadFromXml("data/tracks/Test1-Flat_line.xml");

// Initialize AI car
AICarState aiState;
aiState.lookAhead = 20.0f;

// Set driving style
AIDrivingStyle style;
style.Initialize(0.6f, 0.7f, 0.8f);  // Skilled veteran
```

### 2. Update AI in Game Loop

```cpp
void UpdateOpponentAI(float dt, AICarState& ai, CAR* aiCar)
{
    // Get current racing line point
    int currentPoint = racingLine->FindNearestPoint(aiCar->GetPosition());
    ai.currentPoint = currentPoint;
    
    // Get target point (look-ahead)
    int targetPoint = (currentPoint + 5) % racingLine->GetPointCount();
    const RacingLinePoint& target = racingLine->GetPoint(targetPoint);
    
    // Calculate steering
    Ogre::Vector3 toTarget = target.position - aiCar->GetPosition();
    Ogre::Vector3 forward = aiCar->GetVelocity();
    
    toTarget.y = 0;
    forward.y = 0;
    toTarget.normalise();
    forward.normalise();
    
    float cross = forward.crossProduct(toTarget).y;
    ai.steer = (cross > 0) ? 1.0f : -1.0f;
    
    // Apply throttle/brake
    if (aiCar->GetSpeed() < target.idealSpeed)
        ai.throttle = 1.0f;
    else
        ai.brake = target.brakePoint;
    
    // Apply to car
    aiCar->SetSteering(ai.steer, 1.0f);
    aiCar->SetThrottle(ai.throttle);
    aiCar->SetBrake(ai.brake);
}
```

### 3. Initialize Drift AI

```cpp
DriftAIController driftAI;
driftAI.Initialize(racingLine, drivingStyle);

// In update loop
driftAI.Update(dt, aiState);

// Access drift state
if (driftAI.IsDrifting())
{
    float score = driftAI.GetCurrentScore();
    Ogre::Vector3 target = driftAI.GetDriftTarget();
}
```

### 4. Initialize Traffic

```cpp
TrafficSystem::Get().Initialize();
TrafficSystem::Get().LoadRoutes("data/traffic/routes.xml");

// Spawn traffic
TrafficSystem::Get().SpawnTraffic("route_01", 5);

// In game update
TrafficSystem::Get().Update(dt);

// Check for traffic avoidance
if (TrafficSystem::Get().IsPointBlocked(carPosition, 5.0f))
{
    // Avoid traffic
    aiState.steer += 0.5f;
}
```

---

## Known Limitations

1. **No Full AI Car Integration**
   - AI controller exists but not integrated with CAR class
   - Need to apply AI inputs to actual car physics

2. **Racing Line Generation is Basic**
   - Auto-generation uses simple offset
   - No optimal racing line calculation (outside-inside-outside)

3. **Traffic Routes Not Populated**
   - Traffic system exists but no default routes
   - Need to create traffic route XML files

4. **No Overtaking Logic**
   - AI doesn't detect or avoid other cars
   - No lane-changing behavior

5. **Drift AI is Simplified**
   - Basic zone detection
   - No complex drift line optimization

---

## Next Steps

### Immediate
- [ ] Full AI car integration with CAR class
- [ ] Racing line optimization algorithm
- [ ] Traffic route creation for default tracks
- [ ] Overtaking and collision avoidance

### Future
- **Phase 6**: Career progression integration
- **Phase 7**: Pursuit AI (police behavior)
- **Phase 8**: Advanced AI personalities

---

## References

- `NFS Docs/Implementation Plan.md` - Original Phase 5 spec
- `src/game/RaceAI.h/cpp` - Full implementation
- `hyperlinked/src/hyperlib/streamer/track_path.hpp` - Carbon track path
