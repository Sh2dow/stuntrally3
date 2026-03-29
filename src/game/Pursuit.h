#pragma once
#include <string>
#include <vector>
#include <OgreVector3.h>
#include "mathvector.h"

class GAME;
class CAR;

/// 🚔 Police Car Data
struct PoliceCar
{
    int id = 0;
    std::string carType;              // Police car model
    MATHVECTOR<float,3> position;
    MATHVECTOR<float,3> velocity;
    
    int heatLevel = 1;                // Heat level this cop appears
    int difficulty = 3;               // AI difficulty (1-5)
    
    bool isActive = false;            // Currently in pursuit
    bool isDisabled = false;          // Knocked out
    
    float health = 100.0f;            // Cop car health
    float aggression = 0.5f;          // How aggressive
    
    // Pursuit state
    int ramAttempts = 0;              // Number of ram attempts
    float lastRamTime = 0.0f;         // Time of last ram
};

/// 🚧 Roadblock Data
struct Roadblock
{
    int id = 0;
    Ogre::Vector3 position;
    Ogre::Vector3 direction;          // Direction roadblock faces
    
    int heatLevel = 3;                // Minimum heat for this roadblock
    int strength = 1;                 // 1=light, 2=medium, 3=heavy
    
    bool isDeployed = false;          // Currently on road
    bool isDestroyed = false;         // Player broke through
    
    std::vector<int> policeCarIds;    // Cars manning roadblock
    
    float deployTime = 0.0f;          // Time when deployed
};

/// 🌡️ Heat Level System
/// Tracks player's notoriety with police
struct HeatLevel
{
    int level = 0;                    // 0-5 heat level
    float score = 0.0f;               // Current heat score
    float decayRate = 1.0f;           // Heat decay per second
    
    // Heat thresholds for each level
    static const int thresholds[6];   // 0, 500, 1500, 3000, 6000, 10000
    
    // Rewards per level
    static const int repMultipliers[6];  // 1.0, 1.2, 1.5, 2.0, 3.0, 5.0
    static const int cashMultipliers[6];
    
    void AddScore(float points);
    void Decay(float dt);
    void Reset();
    
    int GetLevel() const { return level; }
    float GetProgressToNext() const;  // 0.0 to 1.0
};

/// 🚓 Pursuit State
/// Current state of a police pursuit
enum PursuitState
{
    PURSUIT_NONE = 0,                 // No pursuit active
    PURSUIT_EVADE,                    // Player evading police
    PURSUIT_SPIKED,                   // Player spiked (tire damage)
    PURSUIT_BUSTED,                   // Player caught
    PURSUIT_ESCAPED,                  // Player escaped
    PURSUIT_COOLDOWN                  // Cooldown after escape
};

/// 🎯 Pursuit Manager
/// Manages all pursuit-related gameplay
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
    PursuitState GetState() const { return currentState; }
    bool IsInPursuit() const { return currentState != PURSUIT_NONE; }
    bool IsBusted() const { return currentState == PURSUIT_BUSTED; }
    bool IsEscaped() const { return currentState == PURSUIT_ESCAPED; }
    
    // Heat management
    HeatLevel& GetHeat() { return heatLevel; }
    const HeatLevel& GetHeat() const { return heatLevel; }
    void AddHeat(float points);
    
    // Police cars
    PoliceCar* SpawnPoliceCar(const Ogre::Vector3& position);
    void DespawnPoliceCar(int id);
    PoliceCar* GetPoliceCar(int id);
    std::vector<PoliceCar*> GetActivePoliceCars();
    
    int GetActiveCopCount() const;
    int GetMaxCopCount() const;
    
    // Roadblocks
    Roadblock* DeployRoadblock(const Ogre::Vector3& position, 
                               const Ogre::Vector3& direction,
                               int strength);
    void RemoveRoadblock(int id);
    Roadblock* GetRoadblock(int id);
    std::vector<Roadblock*> GetActiveRoadblocks();
    
    // Player actions
    void OnPlayerRamPolice(int policeId, float damage);
    void OnPlayerEvade(float dt);
    void OnPlayerSpiked();
    void OnPoliceDisabled(int policeId);
    
    // Busted/Escaped
    void TriggerBusted();
    void TriggerEscaped();
    
    // Pursuit stats
    float GetPursuitTime() const { return pursuitTime; }
    int GetCopsDisabled() const { return copsDisabled; }
    float GetDistanceTraveled() const { return pursuitDistance; }
    
    // Cooldown
    float GetCooldownTime() const { return cooldownTime; }
    float GetMaxCooldownTime() const { return maxCooldownTime; }
    
private:
    PursuitManager() = default;
    
    GAME* pGame = nullptr;
    PursuitState currentState = PURSUIT_NONE;
    HeatLevel heatLevel;
    
    std::vector<PoliceCar> policeCars;
    std::vector<Roadblock> roadblocks;
    std::map<int, int> policeCarMap;    // id -> index
    std::map<int, int> roadblockMap;    // id -> index
    
    // Pursuit tracking
    float pursuitTime = 0.0f;
    float pursuitDistance = 0.0f;
    int copsDisabled = 0;
    float evadeTime = 0.0f;
    
    // Cooldown
    float cooldownTime = 0.0f;
    float maxCooldownTime = 30.0f;
    
    // Spawning
    int nextPoliceId = 1;
    int nextRoadblockId = 1;
    
    // Internal methods
    void UpdatePursuit(float dt);
    void UpdatePoliceAI(float dt);
    void UpdateRoadblocks(float dt);
    void UpdateCooldown(float dt);
    void CheckBusted();
    void CheckEscaped();
    
    void SpawnRoadblockForHeat();
    void SpawnPoliceForHeat();
};

/// 🏁 Pursuit Event Results
struct PursuitResults
{
    int heatLevel = 0;
    float pursuitTime = 0.0f;
    int copsDisabled = 0;
    float distanceTraveled = 0.0f;
    bool escaped = false;
    bool busted = false;
    
    int repEarned = 0;
    int cashEarned = 0;
    
    // Calculate rewards
    void CalculateRewards();
};
