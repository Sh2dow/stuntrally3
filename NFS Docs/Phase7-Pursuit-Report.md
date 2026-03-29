# SR3 Phase 7: Pursuit and Failure States - Implementation Report

## Overview

Phase 7 implements a Carbon-style police pursuit system with heat levels, busted/escaped states, roadblocks, and pursuit events. This adds the failure state counterpart to the racing success conditions.

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `src/game/Pursuit.h` | 202 | Pursuit system declarations |
| `src/game/Pursuit.cpp` | 629 | Full pursuit implementation |

---

## Heat Level System

### HeatLevel Structure

```cpp
struct HeatLevel
{
    int level;                // 0-5 heat level
    float score;              // Current heat score
    float decayRate;          // Heat decay per second
    
    // Thresholds: 0, 500, 1500, 3000, 6000, 10000
    static const int thresholds[6];
    
    void AddScore(float points);
    void Decay(float dt);
    void Reset();
    
    int GetLevel() const { return level; }
    float GetProgressToNext() const;  // 0.0 to 1.0
};
```

### Heat Level Thresholds

| Level | Threshold | Rep Mult | Cash Mult | Cops | Roadblocks |
|-------|-----------|----------|-----------|------|------------|
| 1 | 500 | 1.0x | 1.0x | 2 | No |
| 2 | 1500 | 1.2x | 1.2x | 4 | No |
| 3 | 3000 | 1.5x | 1.5x | 6 | Yes (light) |
| 4 | 6000 | 2.0x | 2.0x | 8 | Yes (medium) |
| 5 | 10000 | 3.0x | 3.0x | 10 | Yes (heavy) |

### Heat Management

```cpp
// Add heat (for speeding, ramming cops, etc.)
PursuitManager::Get().AddHeat(100.0f);

// Heat decays over time when not in pursuit
PursuitManager::Get().GetHeat().Decay(dt);

// Check current level
int heat = PursuitManager::Get().GetHeat().GetLevel();
float progress = PursuitManager::Get().GetHeat().GetProgressToNext();
```

---

## Pursuit System

### PursuitState Enum

```cpp
enum PursuitState
{
    PURSUIT_NONE = 0,           // No pursuit active
    PURSUIT_EVADE,              // Player evading police
    PURSUIT_SPIKED,             // Player spiked (tire damage)
    PURSUIT_BUSTED,             // Player caught
    PURSUIT_ESCAPED,            // Player escaped
    PURSUIT_COOLDOWN            // Cooldown after escape
};
```

### PursuitManager Singleton

```cpp
class PursuitManager
{
public:
    static PursuitManager& Get();
    
    // Initialization
    bool Initialize(GAME* game);
    void Shutdown();
    
    // Pursuit lifecycle
    bool StartPursuit(int heatLevel);
    void Update(float dt);
    void EndPursuit(bool escaped);
    
    // State access
    PursuitState GetState() const;
    bool IsInPursuit() const;
    bool IsBusted() const;
    bool IsEscaped() const;
    
    // Police cars
    PoliceCar* SpawnPoliceCar(const Ogre::Vector3& position);
    void DespawnPoliceCar(int id);
    std::vector<PoliceCar*> GetActivePoliceCars();
    int GetActiveCopCount() const;
    int GetMaxCopCount() const;
    
    // Roadblocks
    Roadblock* DeployRoadblock(const Ogre::Vector3& position,
                               const Ogre::Vector3& direction,
                               int strength);
    std::vector<Roadblock*> GetActiveRoadblocks();
    
    // Player actions
    void OnPlayerRamPolice(int policeId, float damage);
    void OnPlayerEvade(float dt);
    void OnPlayerSpiked();
    void OnPoliceDisabled(int policeId);
    
    // Stats
    float GetPursuitTime() const;
    int GetCopsDisabled() const;
    float GetDistanceTraveled() const;
    float GetCooldownTime() const;
};
```

### Pursuit Lifecycle

```cpp
// Start pursuit (e.g., when player enters restricted area)
if (playerCommitsCrime)
{
    PursuitManager::Get().StartPursuit(heatLevel);
}

// Update in game loop
void Update(float dt)
{
    if (PursuitManager::Get().IsInPursuit())
    {
        PursuitManager::Get().Update(dt);
        
        // Update UI
        UpdatePursuitUI();
    }
}

// End pursuit
if (PursuitManager::Get().IsEscaped())
{
    // Player escaped - award bonus
    AwardEscapeBonus();
}
else if (PursuitManager::Get().IsBusted())
{
    // Player busted - apply penalty
    ApplyBustedPenalty();
}
```

---

## Police Car System

### PoliceCar Structure

```cpp
struct PoliceCar
{
    int id;
    std::string carType;
    MATHVECTOR<float,3> position;
    MATHVECTOR<float,3> velocity;
    
    int heatLevel;              // Heat level this cop appears
    int difficulty;             // AI difficulty (1-5)
    
    bool isActive;
    bool isDisabled;
    
    float health;               // Cop car health (0-100)
    float aggression;           // How aggressive (0-1)
    
    int ramAttempts;            // Number of ram attempts
    float lastRamTime;          // Time of last ram
};
```

### Police AI Behavior

```cpp
void PursuitManager::UpdatePoliceAI(float dt)
{
    if (!pGame || pGame->cars.empty())
        return;
    
    CAR* playerCar = pGame->cars[0];
    MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
    
    for (auto& cop : policeCars)
    {
        if (!cop.isActive || cop.isDisabled)
            continue;
        
        // Move cop towards player (simplified AI)
        float dx = playerPos[0] - cop.position[0];
        float dy = playerPos[1] - cop.position[1];
        float dz = playerPos[2] - cop.position[2];
        
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist > 0.1f)
        {
            float speed = 20.0f + (cop.difficulty * 5.0f);
            cop.position[0] += (dx / dist) * speed * dt;
            cop.position[1] += (dy / dist) * speed * dt;
            cop.position[2] += (dz / dist) * speed * dt;
        }
        
        // Update pursuit distance
        pursuitDistance += dist * dt;
    }
}
```

### Spawning Police

```cpp
void PursuitManager::SpawnPoliceForHeat()
{
    int copsNeeded = GetMaxCopCount() - GetActiveCopCount();
    
    for (int i = 0; i < copsNeeded && i < 2; ++i)
    {
        CAR* playerCar = pGame->cars[0];
        MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
        
        // Spawn cop behind player
        MATHVECTOR<float,3> spawnPos = playerPos;
        spawnPos[0] -= 50.0f;  // Behind player
        spawnPos[1] += (rand() % 20) - 10;  // Random offset
        
        SpawnPoliceCar(spawnPos);
    }
}
```

---

## Roadblock System

### Roadblock Structure

```cpp
struct Roadblock
{
    int id;
    Ogre::Vector3 position;
    Ogre::Vector3 direction;    // Direction roadblock faces
    
    int heatLevel;              // Minimum heat for this roadblock
    int strength;               // 1=light, 2=medium, 3=heavy
    
    bool isDeployed;
    bool isDestroyed;
    
    std::vector<int> policeCarIds;  // Cars manning roadblock
    
    float deployTime;           // Time when deployed
};
```

### Roadblock Deployment

```cpp
void PursuitManager::SpawnRoadblockForHeat()
{
    if (heatLevel.level < 3) return;  // Only at heat 3+
    
    CAR* playerCar = pGame->cars[0];
    MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
    MATHVECTOR<float,3> playerVel = playerCar->GetVelocity();
    
    // Deploy roadblock ahead of player
    MATHVECTOR<float,3> roadblockPos = playerPos;
    roadblockPos[0] += 100.0f;  // 100m ahead
    
    Ogre::Vector3 direction(playerVel[0], playerVel[1], playerVel[2]);
    direction.normalise();
    
    int strength = heatLevel.level >= 5 ? 3 : 
                   (heatLevel.level >= 3 ? 2 : 1);
    
    DeployRoadblock(roadblockPos, direction, strength);
}
```

---

## Busted/Escaped Detection

### Busted Conditions

```cpp
void PursuitManager::CheckBusted()
{
    CAR* playerCar = pGame->cars[0];
    
    // Busted if:
    // 1. Car is too damaged
    if (playerCar->GetDamage() >= 100.f)
    {
        TriggerBusted();
        return;
    }
    
    // 2. Surrounded by cops for 5 seconds
    int nearbyCops = 0;
    MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
    
    for (const auto& cop : policeCars)
    {
        if (!cop.isActive || cop.isDisabled)
            continue;
        
        float dist = Distance(playerPos, cop.position);
        if (dist < 10.0f)  // Within 10 meters
        {
            nearbyCops++;
        }
    }
    
    static float surroundedTime = 0.0f;
    if (nearbyCops >= 3)
    {
        surroundedTime += dt;
        if (surroundedTime >= 5.0f)
        {
            TriggerBusted();
            surroundedTime = 0.0f;
        }
    }
    else
    {
        surroundedTime = 0.0f;
    }
}
```

### Escaped Conditions

```cpp
void PursuitManager::CheckEscaped()
{
    // Escaped if no cops nearby for 10 seconds
    if (GetActiveCopCount() == 0)
    {
        evadeTime += dt;
        
        if (evadeTime >= 10.0f)
        {
            TriggerEscaped();
            evadeTime = 0.0f;
        }
    }
    else
    {
        evadeTime = 0.0f;
    }
}
```

---

## Pursuit Event Type

### PursuitEvent Class

```cpp
class PursuitEvent : public BaseEvent
{
protected:
    int targetHeat;             // Heat level to reach
    float timeLimit;            // Time limit
    int copsToDisable;          // Cops to disable to win
    
    bool isEscaped;
    bool isBusted;
    
    void CheckWinCondition();
    void CheckLossCondition();
};
```

### Win/Loss Conditions

```cpp
void PursuitEvent::CheckWinCondition()
{
    PursuitManager& pursuit = PursuitManager::Get();
    
    // Win if:
    // 1. Reached target heat level AND escaped
    if (pursuit.IsEscaped())
    {
        isEscaped = true;
        if (pursuit.GetHeat().GetLevel() >= targetHeat)
        {
            OnPlayerFinish(1);
        }
    }
    
    // 2. Disabled required number of cops
    if (pursuit.GetCopsDisabled() >= copsToDisable)
    {
        OnPlayerFinish(1);
    }
}

void PursuitEvent::CheckLossCondition()
{
    PursuitManager& pursuit = PursuitManager::Get();
    
    // Lose if:
    // 1. Busted by police
    if (pursuit.IsBusted())
    {
        isBusted = true;
        OnPlayerFail();
    }
    
    // 2. Time limit exceeded
    if (config.timeLimit > 0.0f && raceTime > config.timeLimit)
    {
        OnPlayerFail();
    }
}
```

### Pursuit Results & Rewards

```cpp
struct PursuitResults
{
    int heatLevel;
    float pursuitTime;
    int copsDisabled;
    float distanceTraveled;
    bool escaped;
    bool busted;
    
    int repEarned;
    int cashEarned;
    
    void CalculateRewards()
    {
        if (!escaped)
        {
            repEarned = 0;
            cashEarned = 0;
            return;
        }
        
        // Base rewards
        int baseRep = 100 * heatLevel;
        int baseCash = 50 * heatLevel;
        
        // Multipliers
        float timeMultiplier = std::min(2.0f, pursuitTime / 60.0f);
        float copsMultiplier = 1.0f + (copsDisabled * 0.2f);
        float distanceMultiplier = std::min(2.0f, distanceTraveled / 5000.0f);
        
        repEarned = (int)(baseRep * timeMultiplier * copsMultiplier * distanceMultiplier);
        cashEarned = (int)(baseCash * timeMultiplier * copsMultiplier * distanceMultiplier);
    }
};
```

---

## Integration Guide

### 1. Initialize Pursuit System

```cpp
// In game initialization
PursuitManager::Get().Initialize(pGame);
```

### 2. Start Pursuit

```cpp
// When player commits crime or enters restricted area
void OnPlayerCrime()
{
    int heat = CareerManager::Get().GetProgress().reputation / 1000;
    heat = std::min(5, std::max(1, heat));
    
    PursuitManager::Get().StartPursuit(heat);
}
```

### 3. Update Pursuit

```cpp
void Update(float dt)
{
    if (PursuitManager::Get().IsInPursuit())
    {
        PursuitManager::Get().Update(dt);
        
        // Update UI
        int heat = PursuitManager::Get().GetHeat().GetLevel();
        int cops = PursuitManager::Get().GetActiveCopCount();
        float cooldown = PursuitManager::Get().GetCooldownTime();
        
        UpdatePursuitUI(heat, cops, cooldown);
    }
}
```

### 4. Handle Collisions

```cpp
void OnCarHit(CAR* otherCar)
{
    if (otherCar->isPolice)
    {
        // Ramming police adds heat
        PursuitManager::Get().OnPlayerRamPolice(
            otherCar->id, damage);
    }
}
```

### 5. Pursuit Event

```cpp
// Create pursuit event
EventConfig cfg;
cfg.type = EVENT_PURSUIT;
cfg.pursuitHeat = 3;  // Target heat level
cfg.timeLimit = 120;  // 2 minute limit

PursuitEvent* pursuit = new PursuitEvent(pGame);
pursuit->Init(cfg);
pursuit->Start();
```

---

## Known Limitations

1. **Police AI is Basic**
   - Simple follow behavior
   - No ramming tactics
   - No spike strip deployment
   - No coordinated pursuit

2. **Roadblocks are Static**
   - No visual representation
   - No collision detection
   - Temporary only

3. **No Visual Indicators**
   - No police sirens/lights
   - No minimap police markers
   - No heat level UI

4. **Busted/Escaped Logic is Simplified**
   - No visual detection cones
   - No hiding mechanics
   - No cooldown zones

---

## Next Steps

### Immediate
- [ ] Police car visual models
- [ ] Siren/light effects
- [ ] Roadblock visual/collision
- [ ] Minimap police markers

### Future
- [ ] Advanced police AI (ramming, PIT maneuvers)
- [ ] Spike strips
- [ ] Helicopter pursuit
- [ ] Safe houses/cooldown zones
- [ ] Pursuit breakers (environmental escapes)

---

## References

- `NFS Docs/Implementation Plan.md` - Original Phase 7 spec
- `src/game/Pursuit.h/cpp` - Full implementation
