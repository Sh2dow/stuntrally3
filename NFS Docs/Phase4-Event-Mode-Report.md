# SR3 Phase 4: Event Mode Layer - Implementation Report

## Overview

Phase 4 implements a complete Event Mode system for SR3 with Carbon-style event types including Sprint, Circuit, Drift, Canyon Duel, Canyon Run, and Boss battles.

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `src/game/EventMode.h` | 365 | Event system declarations |
| `src/game/EventMode.cpp` | 1142 | Full event implementation |
| `data/events/events_example.xml` | 88 | Example event database |

---

## Event Types

### EventType Enum

```cpp
enum EventType
{
    EVENT_NONE = 0,
    EVENT_SPRINT,          // Point-to-point race
    EVENT_CIRCUIT,         // Multi-lap circuit
    EVENT_DRIFT,           // Drift competition
    EVENT_CANYON_DUEL,     // Two-car canyon battle
    EVENT_CANYON_RUN,      // Single canyon run
    EVENT_TOLL_BOOTH,      // Time trial through tolls
    EVENT_PURSUIT,         // Police pursuit
    EVENT_BOSS,            // Boss battle
    EVENT_COUNT
};
```

### EventState Lifecycle

```cpp
enum EventState
{
    EVENT_STATE_IDLE = 0,
    EVENT_STATE_COUNTDOWN,    // 3..2..1..GO!
    EVENT_STATE_RACING,       // Active racing
    EVENT_STATE_FINISHED,     // Completed
    EVENT_STATE_FAILED,       // Failed event
    EVENT_STATE_CANCELLED     // Cancelled by player
};
```

---

## Event Classes

### BaseEvent

```cpp
class BaseEvent
{
    // Lifecycle
    virtual bool Init(const EventConfig& config);
    virtual void Start();
    virtual void Update(float dt);
    virtual void End();
    
    // State access
    EventState GetState() const;
    EventType GetType() const;
    
    // Race data
    virtual int GetPlayerPosition() const;
    virtual int GetTotalRacers() const;
    virtual float GetRaceTime() const;
    virtual float GetBestLap() const;
    
    // Callbacks
    virtual void OnPlayerFinish(int position);
    virtual void OnPlayerFail();
    virtual void OnLapComplete(int lap, float lapTime);
    virtual void OnCheckpoint(int index);
};
```

### SprintEvent

Point-to-point racing with progress tracking.

```cpp
class SprintEvent : public BaseEvent
{
protected:
    MATHVECTOR<float,3> startPos, endPos;
    float trackLength;
    float playerProgress;  // 0.0 to 1.0
    
    virtual float CalculateProgress(CAR* car) const;
};
```

**Usage:**
```cpp
EventConfig cfg;
cfg.type = EVENT_SPRINT;
cfg.trackId = "Test1-Flat";
cfg.laps = 1;

SprintEvent* sprint = new SprintEvent(pGame);
sprint->Init(cfg);
sprint->Start();
```

### CircuitEvent

Multi-lap circuit racing with lap counting.

```cpp
class CircuitEvent : public BaseEvent
{
protected:
    int totalLaps;
    std::vector<int> opponentLaps;
    
    virtual void CheckLapComplete();
};
```

**Features:**
- Lap counting
- Opponent lap tracking
- Best lap time tracking

### DriftEvent

Drift competition with scoring and combo system.

```cpp
class DriftEvent : public BaseEvent
{
    float GetDriftScore() const override;
    float GetDriftCombo() const override;
    
protected:
    float driftScore;
    float driftCombo;
    float currentDrift;
    bool isDrifting;
    
    virtual void UpdateDriftScore(float dt);
    virtual float CalculateDriftAmount(CAR* car) const;
};
```

**Scoring Formula:**
```
score = speed × drift_angle × combo × time

combo builds from 1.0 to 5.0 based on:
- Drift duration
- Drift angle maintained
- Successive drift zones
```

**Drift Detection:**
```cpp
float DriftEvent::CalculateDriftAmount(CAR* car) const
{
    MATHVECTOR<float,3> vel = car->GetVelocity();
    QUATERNION<float> orient = car->GetOrientation();
    
    // Get forward vector
    MATHVECTOR<float,3> forward(0, 0, -1);
    orient.RotateVector(forward);
    
    // Calculate angle between velocity and forward
    vel.Normalize();
    forward.Normalize();
    
    float dot = vel[0]*forward[0] + vel[1]*forward[1] + vel[2]*forward[2];
    float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
    
    return angle;  // Radians
}
```

### CanyonDuelEvent

Two-car canyon battle with KO mechanics.

```cpp
class CanyonDuelEvent : public BaseEvent
{
    float GetTimeDelta() const override;
    
protected:
    enum DuelPhase
    {
        PHASE_PLAYER_RUN,     // Player goes first
        PHASE_RIVAL_RUN,      // Rival goes second
        PHASE_RESULT          // Show winner
    };
    
    DuelPhase phase;
    float playerBestTime;
    float rivalBestTime;
    
    int playerKOs;
    int rivalKOs;
};
```

**Phases:**
1. **Player Run** - Player attempts canyon run
2. **Rival Run** - Rival attempts to beat player's time
3. **Result** - Winner determined by time or KOs

**Time Delta Display:**
```cpp
float CanyonDuelEvent::GetTimeDelta() const
{
    if (phase == PHASE_PLAYER_RUN)
    {
        // Show time to beat
        return rivalBestTime > 0 ? rivalBestTime - currentTime : 0.0f;
    }
    else
    {
        // Show rival's deficit
        return currentTime - playerBestTime;
    }
}
```

### CanyonRunEvent

Single-car time attack against target time.

```cpp
class CanyonRunEvent : public BaseEvent
{
protected:
    float targetTime;
    float currentTime;
    
    virtual void CheckTargetTime();
};
```

**Win Condition:**
- Finish in ≤ targetTime → Win
- Finish in > targetTime × 1.5 → Fail

### BossEvent

Boss battles with intro/outro phases.

```cpp
class BossEvent : public BaseEvent
{
protected:
    enum BossPhase
    {
        PHASE_INTRO,        // Boss intro dialogue
        PHASE_RACE,         // Actual race
        PHASE_OUTRO         // Boss reaction
    };
    
    BossPhase phase;
    std::string bossName;
    std::string bossCar;
    bool bossDefeated;
};
```

**Boss Defeat Conditions:**
1. Player finishes ahead of boss, OR
2. Boss crashes/stops (KO)

**Rewards:**
- Double points for boss defeat
- Special boss rewards (cash, reputation, unlocks)

---

## EventConfig Structure

```cpp
struct EventConfig
{
    EventType type;
    std::string id;
    std::string name;
    std::string description;
    
    std::string trackId;
    bool trackReversed;
    
    int laps;
    float timeLimit;
    
    int difficulty;
    int rewardCash;
    int rewardRep;
    
    std::vector<std::string> opponentCars;
    int opponentCount;
    
    // Event-specific
    float driftTarget;      // Drift target score
    float canyonTarget;     // Canyon target time
    int tollCount;          // Toll booth count
    int pursuitHeat;        // Pursuit heat level
    
    // Requirements
    int requiredRep;
    std::vector<std::string> requiredCars;
    
    bool LoadFromXml(const std::string& file);
    bool SaveToXml(const std::string& file) const;
};
```

---

## EventManager System

### Singleton Access

```cpp
class EventManager
{
public:
    static EventManager& Get();
    
    // Initialization
    bool Initialize(GAME* game);
    void Shutdown();
    
    // Event lifecycle
    BaseEvent* CreateEvent(EventType type);
    bool StartEvent(const EventConfig& config);
    void Update(float dt);
    void EndCurrentEvent();
    
    // Current event access
    BaseEvent* GetCurrentEvent();
    bool IsEventActive() const;
    
    // Event database
    bool LoadEventDatabase(const std::string& path);
    const EventConfig* GetEventConfig(const std::string& eventId) const;
    std::vector<const EventConfig*> GetAvailableEvents() const;
    std::vector<const EventConfig*> GetEventsByType(EventType type) const;
    
    // Progression
    bool IsEventUnlocked(const std::string& eventId) const;
    void UnlockEvent(const std::string& eventId);
    void AddReputation(int rep);
};
```

### Usage Example

```cpp
// Initialize
EventManager::Get().Initialize(pGame);
EventManager::Get().LoadEventDatabase("data/events/");

// Start event
EventConfig cfg;
cfg.LoadFromXml("data/events/sprint_01.xml");
EventManager::Get().StartEvent(cfg);

// Update in game loop
EventManager::Get().Update(dt);

// Check event state
if (EventManager::Get().IsEventActive())
{
    BaseEvent* evt = EventManager::Get().GetCurrentEvent();
    
    int pos = evt->GetPlayerPosition();
    float time = evt->GetRaceTime();
    
    // Event-specific data
    if (evt->GetType() == EVENT_DRIFT)
    {
        DriftEvent* drift = static_cast<DriftEvent*>(evt);
        float score = drift->GetDriftScore();
        float combo = drift->GetDriftCombo();
    }
}
```

---

## Event XML Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<events>

    <!-- Sprint Event -->
    <event type="Sprint" id="sprint_01" 
           name="Highway Sprint" 
           desc="Point-to-point race through the city">
        <track id="Test1-Flat" reversed="false"/>
        <params laps="1" timeLimit="0" difficulty="2"/>
        <rewards cash="500" rep="100"/>
        <opponents count="3">
            <car>FN</car>
            <car>LF</car>
            <car>MO</car>
        </opponents>
        <requirements rep="0"/>
    </event>
    
    <!-- Drift Event -->
    <event type="Drift" id="drift_01" 
           name="Drift Battle" 
           desc="Score points by drifting through zones">
        <track id="Test1-Flat" reversed="false"/>
        <params laps="0" timeLimit="120" difficulty="3"/>
        <rewards cash="750" rep="200"/>
        <drift target="5000"/>
        <opponents count="0"/>
        <requirements rep="150"/>
    </event>
    
    <!-- Canyon Duel -->
    <event type="CanyonDuel" id="canyon_01" 
           name="Canyon Duel" 
           desc="Two-car canyon battle">
        <track id="Test1-Flat" reversed="false"/>
        <params laps="0" timeLimit="0" difficulty="4"/>
        <rewards cash="1500" rep="400"/>
        <canyon target="90"/>
        <opponents count="1">
            <car>R1</car>
        </opponents>
        <requirements rep="500"/>
    </event>
    
    <!-- Boss Event -->
    <event type="Boss" id="boss_01" 
           name="Boss: Wolf" 
           desc="Defeat the canyon king">
        <track id="Test1-Flat" reversed="false"/>
        <params laps="1" timeLimit="0" difficulty="5"/>
        <rewards cash="5000" rep="1000"/>
        <boss name="Wolf" car="R3" difficulty="5"/>
        <opponents count="1">
            <car>R3</car>
        </opponents>
        <requirements rep="2000"/>
    </event>
    
</events>
```

---

## Event Results System

```cpp
struct EventResults
{
    int position;           // Final position (1 = first)
    int totalRacers;        // Total participants
    float time;             // Total time (seconds)
    float bestLap;          // Best lap time
    int points;             // Points earned
    
    // Drift-specific
    float driftScore;
    float driftCombo;
    int driftZones;
    
    // Canyon-specific
    float playerTime;
    float rivalTime;
    float timeDiff;
    bool wonByKO;
    
    // Pursuit-specific
    int heatLevel;
    float evasionTime;
    int busts;
    
    bool isWin() const;
    std::string GetResultString() const;
};
```

**Win Detection:**
```cpp
bool EventResults::isWin() const
{
    switch (position)
    {
        case 1: return true;  // First place is always a win
        case 2: case 3: return points > 0;  // Podium with points
        default: return false;
    }
}
```

---

## Integration Guide

### 1. Initialize Event System

```cpp
// In game initialization
EventManager::Get().Initialize(pGame);
EventManager::Get().LoadEventDatabase("data/events/");
```

### 2. Start Event from FE

```cpp
// Player selects event in FE
const EventConfig* cfg = EventManager::Get().GetEventConfig("sprint_01");

if (cfg && EventManager::Get().StartEvent(*cfg))
{
    // Event started successfully
    // Transition to race loading
}
```

### 3. Update Event in Game Loop

```cpp
// In game update
EventManager::Get().Update(dt);

// Check for event end
if (!EventManager::Get().IsEventActive())
{
    // Event finished, show results
    ShowResultsScreen();
}
```

### 4. Access Event Data

```cpp
BaseEvent* evt = EventManager::Get().GetCurrentEvent();

if (evt)
{
    // Common data
    int pos = evt->GetPlayerPosition();
    float time = evt->GetRaceTime();
    
    // Type-specific data
    switch (evt->GetType())
    {
        case EVENT_DRIFT:
        {
            DriftEvent* drift = static_cast<DriftEvent*>(evt);
            float score = drift->GetDriftScore();
            break;
        }
        
        case EVENT_CANYON_DUEL:
        {
            CanyonDuelEvent* canyon = static_cast<CanyonDuelEvent*>(evt);
            float delta = canyon->GetTimeDelta();
            break;
        }
    }
}
```

### 5. Handle Event Completion

```cpp
void OnEventFinished()
{
    EventResults results = EventManager::Get().GetCurrentEvent()->GetResults();
    
    if (results.isWin())
    {
        // Award rewards
        int cash = EventManager::Get().GetCurrentConfig().rewardCash;
        int rep = EventManager::Get().GetCurrentConfig().rewardRep;
        
        // Unlock new events
        EventManager::Get().AddReputation(rep);
        
        // Show victory screen
        ShowVictoryScreen(results);
    }
    else
    {
        // Show defeat screen
        ShowDefeatScreen(results);
    }
}
```

---

## Points System

```cpp
// In BaseEvent::GetResults()
if (playerPosition == 1) 
    results.points = 10;
else if (playerPosition == 2) 
    results.points = 7;
else if (playerPosition == 3) 
    results.points = 5;
else if (playerPosition <= 5) 
    results.points = 3;
else if (playerPosition <= 10) 
    results.points = 1;
```

**Bonus Points:**
- Drift target reached: +5 points
- Drift combo > 2.0: +3 points
- Boss defeated: ×2 multiplier
- Flawless victory (no damage): +2 points

---

## Known Limitations

1. **AI Opponents Not Implemented**
   - Opponent cars listed but no AI controller
   - Need integration with RaceAI system

2. **Boss AI is Placeholder**
   - BossEvent::UpdateBossAI() is stub
   - Need optimal racing line following

3. **Event XML Loading is Basic**
   - No file enumeration
   - Only loads single example file

4. **No Event Progression**
   - Reputation system exists but not integrated with career
   - Event unlocking not connected to FE

5. **UI Integration Missing**
   - No countdown display
   - No position indicator
   - No drift score UI

---

## Next Steps

### Immediate
- [ ] AI opponent integration
- [ ] Boss AI implementation
- [ ] Event XML database loading
- [ ] UI integration (countdown, position, score)

### Future
- **Phase 6**: Career progression integration
- **Phase 6**: World map event selection
- **Phase 7**: Pursuit event implementation

---

## References

- `NFS Docs/Implementation Plan.md` - Original Phase 4 spec
- `src/game/EventMode.h/cpp` - Full implementation
- `data/events/events_example.xml` - Event examples
