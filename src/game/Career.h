#pragma once
#include <string>
#include <vector>
#include <map>
#include <OgreVector3.h>

/// 🏆 Career Progression State
/// Tracks player's progress through career mode
struct CareerProgress
{
    // Player stats
    int reputation = 0;       // Total rep earned
    int cash = 0;             // Current cash
    int level = 1;            // Career level
    
    // Career progress
    int currentDistrict = 0;  // Current district index
    int bossesDefeated = 0;   // Total bosses beaten
    
    // Event completion
    std::vector<std::string> completedEvents;  // Event IDs
    std::vector<std::string> unlockedEvents;   // Event IDs
    
    // Vehicle ownership
    std::vector<std::string> ownedCars;  // Car IDs
    std::string primaryCar;              // Current primary car
    
    // District ownership
    std::map<int, float> districtControl;  // districtId -> control % (0-100)
    
    // Best times/scores
    std::map<std::string, float> bestLapTimes;    // eventId -> time
    std::map<std::string, float> bestDriftScores; // eventId -> score
    
    // Save/load
    bool LoadFromXml(const std::string& file);
    bool SaveToXml(const std::string& file) const;
};

/// 🗺️ District Data
/// Represents a Carbon-style district/territory
struct District
{
    int id = 0;
    std::string name;
    std::string description;
    
    // Visual
    std::string minimapImage;      // District minimap
    Ogre::Vector3 centerPos;       // Center position for camera
    float cameraDistance = 500.0f; // Camera distance for view
    
    // Events
    std::vector<std::string> events;     // Available events
    std::string bossEventId;             // Boss event (empty = no boss)
    
    // Control
    float playerControl = 0.0f;    // Player control % (0-100)
    float rivalControl = 100.0f;   // Rival control % (0-100)
    
    // Requirements
    int requiredRep = 0;           // Rep needed to challenge
    int requiredBosses = 0;        // Bosses needed to unlock
    
    // Rewards
    int completionCash = 0;        // Cash for 100% control
    int completionRep = 0;         // Rep for 100% control
    std::string unlockCar;         // Car unlocked at 100%
    
    bool IsUnlocked(const CareerProgress& career) const;
    bool IsComplete() const;
};

/// 👤 Rival Data
/// Represents a Carbon-style rival racer
struct Rival
{
    std::string id;
    std::string name;
    std::string description;
    
    int districtId = 0;        // Associated district
    std::string bossCar;       // Boss's car
    int difficulty = 5;        // 1-5 difficulty
    
    std::string introDialogue; // Pre-race dialogue
    std::string outroDialogue; // Post-race dialogue
    
    bool isDefeated = false;
    
    // Rewards
    int defeatCash = 1000;
    int defeatRep = 200;
    std::string unlockEvent;   // Event unlocked on defeat
};

/// 📋 Career Manager
/// Manages career progression and world map state
class CareerManager
{
public:
    static CareerManager& Get();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Career data
    bool LoadCareer(const std::string& file);
    bool SaveCareer(const std::string& file);
    void NewCareer();
    
    CareerProgress& GetProgress() { return progress; }
    const CareerProgress& GetProgress() const { return progress; }
    
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
    const std::string& GetPrimaryCar() const { return progress.primaryCar; }
    
    // Best times/scores
    float GetBestLapTime(const std::string& eventId) const;
    float GetBestDriftScore(const std::string& eventId) const;
    void SetBestLapTime(const std::string& eventId, float time);
    void SetBestDriftScore(const std::string& eventId, float score);
    
    // Level calculation
    int GetLevel() const { return progress.level; }
    int GetRepForNextLevel() const;
    float GetLevelProgress() const;  // 0.0 to 1.0
    
private:
    CareerManager() = default;
    
    CareerProgress progress;
    std::vector<District> districts;
    std::map<int, int> districtMap;  // id -> index
    std::vector<Rival> rivals;
    std::map<std::string, int> rivalMap;  // id -> index
    
    // Level thresholds (rep needed for each level)
    std::vector<int> levelThresholds;
    
    void CalculateLevel();
    void CheckUnlocks();
};
