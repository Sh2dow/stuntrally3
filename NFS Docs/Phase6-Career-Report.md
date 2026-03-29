# SR3 Phase 6: Career and World Map Shell - Implementation Report

## Overview

Phase 6 implements a Carbon-style career progression system with district ownership, boss progression, and a menu-driven world map shell. This provides the meta-game structure that ties all event types together into a cohesive career experience.

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `src/game/Career.h` | 234 | Career system declarations |
| `src/game/Career.cpp` | 780 | Full career implementation |
| `data/career/career.xml` | 150 | Career data (districts, rivals) |

---

## Career Progression System

### CareerProgress Structure

```cpp
struct CareerProgress
{
    // Player stats
    int reputation;           // Total rep earned
    int cash;                 // Current cash
    int level;                // Career level (1-11+)
    
    // Career progress
    int currentDistrict;      // Current district index
    int bossesDefeated;       // Total bosses beaten
    
    // Event completion
    std::vector<std::string> completedEvents;
    std::vector<std::string> unlockedEvents;
    
    // Vehicle ownership
    std::vector<std::string> ownedCars;
    std::string primaryCar;
    
    // District ownership
    std::map<int, float> districtControl;  // districtId -> control %
    
    // Best times/scores
    std::map<std::string, float> bestLapTimes;
    std::map<std::string, float> bestDriftScores;
};
```

### CareerManager Singleton

```cpp
class CareerManager
{
public:
    static CareerManager& Get();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    bool LoadCareer(const std::string& file);
    bool SaveCareer(const std::string& file);
    void NewCareer();
    
    // Career data access
    CareerProgress& GetProgress();
    
    // District management
    const District* GetDistrict(int id) const;
    const District* GetCurrentDistrict() const;
    std::vector<const District*> GetAllDistricts() const;
    
    float GetDistrictControl(int districtId) const;
    void UpdateDistrictControl(int districtId, float delta);
    bool IsDistrictComplete(int districtId) const;
    
    // Event management
    bool IsEventCompleted(const std::string& eventId) const;
    bool IsEventUnlocked(const std::string& eventId) const;
    void MarkEventCompleted(const std::string& eventId);
    void UnlockEvent(const std::string& eventId);
    
    std::vector<std::string> GetAvailableEvents() const;
    std::vector<std::string> GetDistrictEvents(int districtId) const;
    
    // Rival management
    const Rival* GetRival(const std::string& id) const;
    const Rival* GetDistrictBoss(int districtId) const;
    bool IsRivalDefeated(const std::string& rivalId) const;
    void MarkRivalDefeated(const std::string& rivalId);
    
    // Progression
    void AddReputation(int rep);
    void AddCash(int cash);
    void SpendCash(int cash);
    
    bool CanAccessDistrict(int districtId) const;
    bool CanChallengeBoss(int districtId) const;
    
    // Vehicle ownership
    bool OwnsCar(const std::string& carId) const;
    void AddCar(const std::string& carId);
    void SetPrimaryCar(const std::string& carId);
    
    // Best times/scores
    float GetBestLapTime(const std::string& eventId) const;
    float GetBestDriftScore(const std::string& eventId) const;
    void SetBestLapTime(const std::string& eventId, float time);
    void SetBestDriftScore(const std::string& eventId, float score);
    
    // Level system
    int GetLevel() const;
    int GetRepForNextLevel() const;
    float GetLevelProgress() const;  // 0.0 to 1.0
};
```

---

## District System

### District Structure

```cpp
struct District
{
    int id;
    std::string name;
    std::string description;
    
    // Visual
    std::string minimapImage;
    Ogre::Vector3 centerPos;
    float cameraDistance;
    
    // Events
    std::vector<std::string> events;
    std::string bossEventId;
    
    // Control
    float playerControl;      // 0-100%
    float rivalControl;       // 0-100%
    
    // Requirements
    int requiredRep;
    int requiredBosses;
    
    // Rewards
    int completionCash;
    int completionRep;
    std::string unlockCar;
    
    bool IsUnlocked(const CareerProgress& career) const;
    bool IsComplete() const;
};
```

### District Control Mechanics

```cpp
// After completing an event, update district control
void OnEventCompleted(const std::string& eventId, int districtId)
{
    // Each event gives 20% control (5 events = 100%)
    CareerManager::Get().UpdateDistrictControl(districtId, 20.0f);
    
    // Check for completion
    if (CareerManager::Get().IsDistrictComplete(districtId))
    {
        const District* district = CareerManager::Get().GetDistrict(districtId);
        
        // Award completion rewards
        CareerManager::Get().AddCash(district->completionCash);
        CareerManager::Get().AddReputation(district->completionRep);
        
        // Unlock bonus car
        if (!district->unlockCar.empty())
        {
            CareerManager::Get().AddCar(district->unlockCar);
        }
    }
}
```

### Default Districts

| ID | Name | Required Rep | Required Bosses | Rewards |
|----|------|--------------|-----------------|---------|
| 1 | Downtown | 0 | 0 | $2000, 500 rep, FN |
| 2 | Industrial | 500 | 1 | $3000, 750 rep, LF |
| 3 | Harbor | 1500 | 2 | $4000, 1000 rep, MO |
| 4 | Airport | 3000 | 3 | $5000, 1500 rep, ES |
| 5 | Mountain Hills | 5000 | 4 | $7500, 2000 rep, R1 |

---

## Rival/Boss System

### Rival Structure

```cpp
struct Rival
{
    std::string id;
    std::string name;
    std::string description;
    
    int districtId;           // Associated district
    std::string bossCar;      // Boss's car
    int difficulty;           // 1-5
    
    std::string introDialogue;
    std::string outroDialogue;
    
    bool isDefeated;
    
    // Rewards
    int defeatCash;
    int defeatRep;
    std::string unlockEvent;  // Event unlocked on defeat
};
```

### Boss Challenge Flow

```cpp
// Check if player can challenge boss
if (CareerManager::Get().CanChallengeBoss(districtId))
{
    const Rival* boss = CareerManager::Get().GetDistrictBoss(districtId);
    
    // Show boss intro dialogue
    ShowDialogue(boss->introDialogue);
    
    // Start boss event
    EventConfig cfg;
    cfg.LoadFromXml("data/events/" + boss->id + ".xml");
    EventManager::Get().StartEvent(cfg);
}

// After boss race completion
void OnBossRaceFinished(const std::string& bossId, bool won)
{
    if (won)
    {
        const Rival* boss = CareerManager::Get().GetRival(bossId);
        
        // Mark boss as defeated
        CareerManager::Get().MarkRivalDefeated(bossId);
        
        // Award rewards
        CareerManager::Get().AddCash(boss->defeatCash);
        CareerManager::Get().AddReputation(boss->defeatRep);
        
        // Unlock next content
        if (!boss->unlockEvent.empty())
        {
            CareerManager::Get().UnlockEvent(boss->unlockEvent);
        }
        
        // Show outro dialogue
        ShowDialogue(boss->outroDialogue);
    }
}
```

### Default Boss Rivals

| Boss | District | Car | Difficulty | Rewards |
|------|----------|-----|------------|---------|
| Wolf | Downtown | R3 | 3 | $2000, 500 rep |
| Razor | Industrial | FR4 | 4 | $3000, 750 rep |
| Kaze | Harbor | HI | 4 | $4000, 1000 rep |
| Jax | Airport | H2 | 5 | $5000, 1500 rep |
| Darius | Mountain Hills | S8 | 5 | $10000, 5000 rep |

---

## Level/Reputation System

### Level Thresholds

```cpp
// Rep needed for each level
std::vector<int> levelThresholds = {
    0,      // Level 1
    500,    // Level 2
    1500,   // Level 3
    3000,   // Level 4
    5000,   // Level 5
    8000,   // Level 6
    12000,  // Level 7
    17000,  // Level 8
    23000,  // Level 9
    30000,  // Level 10
    50000   // Level 11+
};
```

### Level Calculation

```cpp
void CareerManager::CalculateLevel()
{
    int rep = progress.reputation;
    int newLevel = 1;
    
    for (size_t i = 0; i < levelThresholds.size(); ++i)
    {
        if (rep >= levelThresholds[i])
            newLevel = (int)i + 1;
        else
            break;
    }
    
    progress.level = newLevel;
}

float CareerManager::GetLevelProgress() const
{
    int currentRep = progress.reputation;
    int prevThreshold = (progress.level > 1) ? 
        levelThresholds[progress.level - 2] : 0;
    int nextThreshold = GetRepForNextLevel();
    
    if (nextThreshold <= prevThreshold)
        return 1.0f;
    
    float prog = (float)(currentRep - prevThreshold) / 
                 (float)(nextThreshold - prevThreshold);
    return std::max(0.0f, std::min(1.0f, prog));
}
```

---

## Event Unlocking System

### Unlock Logic

```cpp
void CareerManager::CheckUnlocks()
{
    // Check district unlocks
    for (const auto& district : districts)
    {
        if (district.IsUnlocked(progress))
        {
            // Unlock district events
            for (const auto& eventId : district.events)
            {
                UnlockEvent(eventId);
            }
        }
    }
    
    // Check rival unlocks
    for (const auto& rival : rivals)
    {
        if (CanChallengeBoss(rival.districtId))
        {
            UnlockEvent(rival.id);  // Use rival ID as event ID
        }
    }
}

bool District::IsUnlocked(const CareerProgress& career) const
{
    // Check reputation requirement
    if (career.reputation < requiredRep)
        return false;
    
    // Check boss requirement
    if (career.bossesDefeated < requiredBosses)
        return false;
    
    return true;
}
```

### Getting Available Events

```cpp
// Get all available (unlocked but not completed) events
std::vector<std::string> CareerManager::GetAvailableEvents() const
{
    std::vector<std::string> events;
    
    for (const auto& eventId : progress.unlockedEvents)
    {
        if (!IsEventCompleted(eventId))
        {
            events.push_back(eventId);
        }
    }
    
    return events;
}

// Get events for specific district
std::vector<std::string> CareerManager::GetDistrictEvents(int districtId) const
{
    std::vector<std::string> events;
    
    const District* district = GetDistrict(districtId);
    if (district)
    {
        for (const auto& eventId : district->events)
        {
            if (IsEventUnlocked(eventId))
            {
                events.push_back(eventId);
            }
        }
        
        // Add boss event if district is ready
        if (!district->bossEventId.empty() && 
            CanChallengeBoss(districtId))
        {
            events.push_back(district->bossEventId);
        }
    }
    
    return events;
}
```

---

## Career XML Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<career>

    <!-- Districts -->
    <districts>
        <district id="1" name="Downtown" desc="The heart of the city"
                  minimap="data/career/district_1.png"
                  cameraDist="400"
                  requiredRep="0"
                  requiredBosses="0"
                  completionCash="2000"
                  completionRep="500"
                  unlockCar="FN">
            <events>
                <event>sprint_01</event>
                <event>circuit_01</event>
                <event>drift_01</event>
            </events>
            <bossEvent>boss_downtown</bossEvent>
        </district>
    </districts>
    
    <!-- Rivals -->
    <rivals>
        <rival id="boss_downtown" name="Wolf" 
               desc="King of the downtown streets"
               district="1" car="R3" difficulty="3"
               defeatCash="2000" defeatRep="500"
               unlockEvent="district_2_unlock">
            <dialogue intro="You think you can take me on?"
                      outro="Not bad... but this isn't over."/>
        </rival>
    </rivals>
    
</career>
```

---

## Save/Load System

### Save Career

```cpp
// Save career progress
bool CareerProgress::SaveToXml(const std::string& file) const
{
    XMLDocument xml;
    XMLElement* root = xml.NewElement("career");
    
    // Player stats
    root->SetAttribute("reputation", reputation);
    root->SetAttribute("cash", cash);
    root->SetAttribute("level", level);
    
    // Career progress
    root->SetAttribute("currentDistrict", currentDistrict);
    root->SetAttribute("bossesDefeated", bossesDefeated);
    
    // Completed events
    if (!completedEvents.empty())
    {
        XMLElement* eventsElem = xml.NewElement("completedEvents");
        for (const auto& eventId : completedEvents)
        {
            XMLElement* eventElem = xml.NewElement("event");
            eventElem->SetText(eventId.c_str());
            eventsElem->InsertEndChild(eventElem);
        }
        root->InsertEndChild(eventsElem);
    }
    
    // ... (saved owned cars, district control, best times, etc.)
    
    xml.InsertEndChild(root);
    return xml.SaveFile(file.c_str());
}

// Usage
CareerManager::Get().SaveCareer("data/career/save.xml");
```

### Load Career

```cpp
// Load career progress
bool CareerProgress::LoadFromXml(const std::string& file)
{
    XMLDocument doc;
    XMLError er = doc.LoadFile(file.c_str());
    if (er != XML_SUCCESS) return false;
    
    XMLElement* root = doc.RootElement();
    if (!root) return false;
    
    const char* a;
    
    // Player stats
    a = root->Attribute("reputation");
    if (a) reputation = s2i(a);
    
    a = root->Attribute("cash");
    if (a) cash = s2i(a);
    
    a = root->Attribute("level");
    if (a) level = s2i(a);
    
    // ... (load completed events, owned cars, district control, etc.)
    
    return true;
}

// Usage
CareerManager::Get().LoadCareer("data/career/save.xml");
```

---

## Integration Guide

### 1. Initialize Career System

```cpp
// In game initialization
CareerManager::Get().Initialize();
CareerManager::Get().LoadCareer("data/career/career.xml");

// Check if save exists
if (FileExists("data/career/save.xml"))
{
    CareerManager::Get().LoadCareer("data/career/save.xml");
}
else
{
    CareerManager::Get().NewCareer();
}
```

### 2. Event Completion Callback

```cpp
void OnEventFinished(const std::string& eventId, EventResults& results)
{
    if (results.isWin())
    {
        // Mark event complete
        CareerManager::Get().MarkEventCompleted(eventId);
        
        // Award rewards
        int repGain = results.points * 10;
        int cashGain = results.points * 5;
        
        CareerManager::Get().AddReputation(repGain);
        CareerManager::Get().AddCash(cashGain);
        
        // Save best time/score
        if (results.bestLap > 0)
        {
            CareerManager::Get().SetBestLapTime(eventId, results.bestLap);
        }
        if (results.driftScore > 0)
        {
            CareerManager::Get().SetBestDriftScore(eventId, results.driftScore);
        }
        
        // Save career
        CareerManager::Get().SaveCareer("data/career/save.xml");
        
        // Show progression UI
        ShowCareerProgress(repGain, cashGain);
    }
}
```

### 3. World Map Event Selection

```cpp
void ShowWorldMap()
{
    // Get current district
    const District* district = CareerManager::Get().GetCurrentDistrict();
    
    // Show district minimap
    ShowMinimap(district->minimapImage);
    
    // Position camera
    SetCameraPosition(district->centerPos, district->cameraDistance);
    
    // Get available events
    auto events = CareerManager::Get().GetDistrictEvents(district->id);
    
    // Show event markers on map
    for (const auto& eventId : events)
    {
        bool completed = CareerManager::Get().IsEventCompleted(eventId);
        ShowEventMarker(eventId, completed);
    }
    
    // Show boss marker if available
    if (CareerManager::Get().CanChallengeBoss(district->id))
    {
        const Rival* boss = CareerManager::Get().GetDistrictBoss(district->id);
        ShowBossMarker(boss->id);
    }
}
```

### 4. District Selection

```cpp
void OnDistrictSelected(int districtId)
{
    if (CareerManager::Get().CanAccessDistrict(districtId))
    {
        // Switch to selected district
        CareerManager::Get().progress.currentDistrict = districtId;
        
        // Show district on world map
        ShowWorldMap();
    }
    else
    {
        // Show lock info
        const District* district = CareerManager::Get().GetDistrict(districtId);
        ShowLockInfo(district->requiredRep, district->requiredBosses);
    }
}
```

---

## Known Limitations

1. **No Visual World Map**
   - Career data exists but no 3D/2D map UI
   - Need GUI integration for district selection

2. **Save System is Basic**
   - Manual save only (on event completion)
   - No auto-save or multiple save slots

3. **No Career Restart**
   - No option to reset career progress
   - Need confirmation dialog for new game

4. **Limited Event Types**
   - Only basic event types implemented
   - Pursuit, toll booth events not connected

5. **No Multiplayer Career**
   - Career is single-player only
   - No online progression sync

---

## Next Steps

### Immediate
- [ ] GUI integration for world map display
- [ ] District minimap rendering
- [ ] Event marker UI on map
- [ ] Save slot system

### Future
- **Phase 7**: Pursuit and Failure States
- **Phase 8**: Vehicle customization integration
- **Phase 9**: Full Carbon streaming/open world

---

## References

- `NFS Docs/Implementation Plan.md` - Original Phase 6 spec
- `src/game/Career.h/cpp` - Full implementation
- `data/career/career.xml` - Career data
