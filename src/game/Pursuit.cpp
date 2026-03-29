#include "pch.h"
#include "Pursuit.h"
#include "game.h"
#include "car.h"
#include "EventMode.h"

// Heat level thresholds
const int HeatLevel::thresholds[6] = {0, 500, 1500, 3000, 6000, 10000};
const int HeatLevel::repMultipliers[6] = {10, 12, 15, 20, 30, 50};
const int HeatLevel::cashMultipliers[6] = {10, 12, 15, 20, 30, 50};


//--------------------------------------------------------------------------------------------------------------------------
// HeatLevel
//--------------------------------------------------------------------------------------------------------------------------

void HeatLevel::AddScore(float points)
{
    score += points;
    
    // Update level based on score
    for (int i = 5; i >= 1; --i)
    {
        if (score >= thresholds[i])
        {
            level = i;
            break;
        }
    }
}

void HeatLevel::Decay(float dt)
{
    if (level > 0)
    {
        score -= decayRate * dt;
        
        if (score < 0)
            score = 0;
        
        // Drop level if below threshold
        for (int i = level - 1; i >= 0; --i)
        {
            if (score < thresholds[i])
            {
                level = i;
                break;
            }
        }
    }
}

void HeatLevel::Reset()
{
    level = 0;
    score = 0.0f;
}

float HeatLevel::GetProgressToNext() const
{
    if (level >= 5)
        return 1.0f;
    
    int current = thresholds[level];
    int next = thresholds[level + 1];
    
    if (next <= current)
        return 1.0f;
    
    float progress = (score - current) / (float)(next - current);
    return std::max(0.0f, std::min(1.0f, progress));
}


//--------------------------------------------------------------------------------------------------------------------------
// PursuitManager
//--------------------------------------------------------------------------------------------------------------------------

PursuitManager& PursuitManager::Get()
{
    static PursuitManager instance;
    return instance;
}

bool PursuitManager::Initialize(GAME* game)
{
    pGame = game;
    return true;
}

void PursuitManager::Shutdown()
{
    policeCars.clear();
    policeCarMap.clear();
    roadblocks.clear();
    roadblockMap.clear();
    heatLevel.Reset();
    currentState = PURSUIT_NONE;
}

bool PursuitManager::StartPursuit(int heat)
{
    if (currentState != PURSUIT_NONE)
        return false;  // Already in pursuit
    
    heatLevel.level = heat;
    currentState = PURSUIT_EVADE;
    pursuitTime = 0.0f;
    pursuitDistance = 0.0f;
    copsDisabled = 0;
    evadeTime = 0.0f;
    
    // Spawn initial police cars
    SpawnPoliceForHeat();
    
    return true;
}

void PursuitManager::Update(float dt)
{
    switch (currentState)
    {
        case PURSUIT_EVADE:
        case PURSUIT_SPIKED:
            UpdatePursuit(dt);
            break;
            
        case PURSUIT_COOLDOWN:
            UpdateCooldown(dt);
            break;
            
        default:
            break;
    }
}

void PursuitManager::UpdatePursuit(float dt)
{
    // Update pursuit time
    pursuitTime += dt;
    
    // Update evade time (for escape detection)
    if (!IsInPursuit())
        evadeTime += dt;
    else
        evadeTime = 0.0f;
    
    // Update police AI
    UpdatePoliceAI(dt);
    
    // Update roadblocks
    UpdateRoadblocks(dt);
    
    // Check for busted/escaped conditions
    CheckBusted();
    CheckEscaped();
    
    // Spawn additional cops based on heat
    if (GetActiveCopCount() < GetMaxCopCount())
    {
        float spawnChance = 0.01f * heatLevel.level;
        if (rand() / (float)RAND_MAX < spawnChance)
        {
            SpawnPoliceForHeat();
        }
    }
    
    // Deploy roadblocks at high heat
    if (heatLevel.level >= 3)
    {
        float roadblockChance = 0.005f * (heatLevel.level - 2);
        if (rand() / (float)RAND_MAX < roadblockChance)
        {
            SpawnRoadblockForHeat();
        }
    }
}

void PursuitManager::UpdateCooldown(float dt)
{
    cooldownTime -= dt;
    
    if (cooldownTime <= 0.0f)
    {
        // Cooldown over, heat can decay normally
        heatLevel.Decay(1.0f);
        
        if (heatLevel.level == 0)
        {
            currentState = PURSUIT_NONE;
        }
    }
}

void PursuitManager::EndPursuit(bool escaped)
{
    if (escaped)
    {
        currentState = PURSUIT_COOLDOWN;
        cooldownTime = maxCooldownTime;
        
        // Award escape bonus
        int escapeBonus = heatLevel.level * 100;
        if (pGame)
        {
            // TODO: Add to career
        }
    }
    else
    {
        currentState = PURSUIT_BUSTED;
        
        // Penalty for being busted
        heatLevel.score *= 0.5f;  // Lose half heat score
    }
    
    // Clear active police
    for (auto& cop : policeCars)
    {
        cop.isActive = false;
    }
}

void PursuitManager::AddHeat(float points)
{
    heatLevel.AddScore(points);
    
    // Spawn more cops if heat increased
    SpawnPoliceForHeat();
}

PoliceCar* PursuitManager::SpawnPoliceCar(const Ogre::Vector3& position)
{
    PoliceCar cop;
    cop.id = nextPoliceId++;
    cop.position = MATHVECTOR<float,3>(position.x, position.y, position.z);
    cop.heatLevel = heatLevel.level;
    cop.difficulty = 2 + (heatLevel.level / 2);  // Higher difficulty at higher heat
    cop.isActive = true;
    cop.aggression = 0.3f + (heatLevel.level * 0.1f);
    
    policeCars.push_back(cop);
    policeCarMap[cop.id] = (int)(policeCars.size() - 1);
    
    return &policeCars.back();
}

void PursuitManager::DespawnPoliceCar(int id)
{
    auto it = policeCarMap.find(id);
    if (it != policeCarMap.end())
    {
        policeCars[it->second].isActive = false;
        policeCars[it->second].isDisabled = true;
    }
}

PoliceCar* PursuitManager::GetPoliceCar(int id)
{
    auto it = policeCarMap.find(id);
    if (it != policeCarMap.end())
        return &policeCars[it->second];
    return nullptr;
}

std::vector<PoliceCar*> PursuitManager::GetActivePoliceCars()
{
    std::vector<PoliceCar*> active;
    for (auto& cop : policeCars)
    {
        if (cop.isActive && !cop.isDisabled)
        {
            active.push_back(&cop);
        }
    }
    return active;
}

int PursuitManager::GetActiveCopCount() const
{
    int count = 0;
    for (const auto& cop : policeCars)
    {
        if (cop.isActive && !cop.isDisabled)
            count++;
    }
    return count;
}

int PursuitManager::GetMaxCopCount() const
{
    // More cops at higher heat levels
    switch (heatLevel.level)
    {
        case 1: return 2;
        case 2: return 4;
        case 3: return 6;
        case 4: return 8;
        case 5: return 10;
        default: return 0;
    }
}

Roadblock* PursuitManager::DeployRoadblock(const Ogre::Vector3& position,
                                           const Ogre::Vector3& direction,
                                           int strength)
{
    Roadblock rb;
    rb.id = nextRoadblockId++;
    rb.position = position;
    rb.direction = direction.normalisedCopy();
    rb.strength = strength;
    rb.heatLevel = heatLevel.level;
    rb.isDeployed = true;
    rb.deployTime = pursuitTime;
    
    roadblocks.push_back(rb);
    roadblockMap[rb.id] = (int)(roadblocks.size() - 1);
    
    return &roadblocks.back();
}

void PursuitManager::RemoveRoadblock(int id)
{
    auto it = roadblockMap.find(id);
    if (it != roadblockMap.end())
    {
        roadblocks[it->second].isDeployed = false;
        roadblocks[it->second].isDestroyed = true;
    }
}

Roadblock* PursuitManager::GetRoadblock(int id)
{
    auto it = roadblockMap.find(id);
    if (it != roadblockMap.end())
        return &roadblocks[it->second];
    return nullptr;
}

std::vector<Roadblock*> PursuitManager::GetActiveRoadblocks()
{
    std::vector<Roadblock*> active;
    for (auto& rb : roadblocks)
    {
        if (rb.isDeployed && !rb.isDestroyed)
        {
            active.push_back(&rb);
        }
    }
    return active;
}

void PursuitManager::OnPlayerRamPolice(int policeId, float damage)
{
    PoliceCar* cop = GetPoliceCar(policeId);
    if (!cop) return;
    
    cop->health -= damage;
    cop->ramAttempts++;
    cop->lastRamTime = pursuitTime;
    
    // Add heat for attacking police
    AddHeat(50.0f);
    
    // Check if cop is disabled
    if (cop->health <= 0.0f)
    {
        OnPoliceDisabled(policeId);
    }
}

void PursuitManager::OnPlayerEvade(float dt)
{
    // Player is evading - reduce visibility
    evadeTime += dt;
}

void PursuitManager::OnPlayerSpiked()
{
    currentState = PURSUIT_SPIKED;
    
    // Player took spike strip damage
    // TODO: Apply tire damage to player car
}

void PursuitManager::OnPoliceDisabled(int policeId)
{
    PoliceCar* cop = GetPoliceCar(policeId);
    if (!cop) return;
    
    cop->isDisabled = true;
    cop->isActive = false;
    copsDisabled++;
    
    // Award points for disabling cop
    int copBounty = heatLevel.level * 200;
    AddHeat(copBounty / 10.0f);  // Some heat for taking out cops
}

void PursuitManager::TriggerBusted()
{
    EndPursuit(false);
    
    // Busted penalty
    if (pGame)
    {
        // TODO: Lose rep/cash
        // TODO: Impound car
        // TODO: Restart from last checkpoint
    }
}

void PursuitManager::TriggerEscaped()
{
    EndPursuit(true);
    
    // Escape bonus
    if (pGame)
    {
        // TODO: Award escape bonus
    }
}

void PursuitManager::UpdatePoliceAI(float dt)
{
    // Simple police AI - follow player
    if (!pGame || pGame->cars.empty())
        return;
    
    CAR* playerCar = pGame->cars[0];
    if (!playerCar) return;
    
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
            float speed = 20.0f + (cop.difficulty * 5.0f);  // Cop speed
            float moveX = (dx / dist) * speed * dt;
            float moveY = (dy / dist) * speed * dt;
            float moveZ = (dz / dist) * speed * dt;
            
            cop.position[0] += moveX;
            cop.position[1] += moveY;
            cop.position[2] += moveZ;
        }
        
        // Update pursuit distance
        pursuitDistance += dist * dt;
    }
}

void PursuitManager::UpdateRoadblocks(float dt)
{
    // Update roadblock deployment
    for (auto& rb : roadblocks)
    {
        if (rb.isDeployed && !rb.isDestroyed)
        {
            // Roadblock active for limited time
            if (pursuitTime - rb.deployTime > 60.0f)  // 60 seconds
            {
                rb.isDeployed = false;
            }
        }
    }
}

void PursuitManager::CheckBusted()
{
    if (!pGame || pGame->cars.empty())
        return;
    
    CAR* playerCar = pGame->cars[0];
    if (!playerCar) return;
    
    // Busted if:
    // 1. Car is too damaged
    // 2. Surrounded by cops
    // 3. Hit roadblock at high heat
    
    // Check car damage
    // TODO: if (playerCar->GetDamage() >= 100.f)
    //     TriggerBusted();
    
    // Check if surrounded
    int nearbyCops = 0;
    MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
    
    for (const auto& cop : policeCars)
    {
        if (!cop.isActive || cop.isDisabled)
            continue;
        
        float dx = playerPos[0] - cop.position[0];
        float dy = playerPos[1] - cop.position[1];
        float dz = playerPos[2] - cop.position[2];
        
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist < 10.0f)  // Within 10 meters
        {
            nearbyCops++;
        }
    }
    
    // Busted if 3+ cops nearby for 5 seconds
    static float surroundedTime = 0.0f;
    if (nearbyCops >= 3)
    {
        surroundedTime += 1.0f/60.0f;  // Assuming 60 FPS
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

void PursuitManager::CheckEscaped()
{
    // Escaped if no cops nearby for 10 seconds
    if (GetActiveCopCount() == 0)
    {
        evadeTime += 1.0f/60.0f;  // Assuming 60 FPS
        
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

void PursuitManager::SpawnPoliceForHeat()
{
    int copsNeeded = GetMaxCopCount() - GetActiveCopCount();
    
    for (int i = 0; i < copsNeeded && i < 2; ++i)  // Spawn max 2 at a time
    {
        if (!pGame || pGame->cars.empty())
            break;
        
        CAR* playerCar = pGame->cars[0];
        if (!playerCar) break;
        
        MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
        
        // Spawn cop behind player
        MATHVECTOR<float,3> spawnPos = playerPos;
        spawnPos[0] -= 50.0f;  // Behind player
        spawnPos[1] += (rand() % 20) - 10;  // Random offset
        
        Ogre::Vector3 ogreSpawn(spawnPos[0], spawnPos[1], spawnPos[2]);
        SpawnPoliceCar(ogreSpawn);
    }
}

void PursuitManager::SpawnRoadblockForHeat()
{
    if (!pGame || pGame->cars.empty())
        return;
    
    CAR* playerCar = pGame->cars[0];
    if (!playerCar) return;
    
    MATHVECTOR<float,3> playerPos = playerCar->GetPosition();
    MATHVECTOR<float,3> playerVel = playerCar->GetVelocity();
    
    // Deploy roadblock ahead of player
    MATHVECTOR<float,3> roadblockPos = playerPos;
    roadblockPos[0] += 100.0f;  // 100m ahead
    
    Ogre::Vector3 direction(playerVel[0], playerVel[1], playerVel[2]);
    direction.normalise();
    
    int strength = heatLevel.level >= 5 ? 3 : (heatLevel.level >= 3 ? 2 : 1);
    
    Ogre::Vector3 ogrePos(roadblockPos[0], roadblockPos[1], roadblockPos[2]);
    DeployRoadblock(ogrePos, direction, strength);
}


//--------------------------------------------------------------------------------------------------------------------------
// PursuitResults
//--------------------------------------------------------------------------------------------------------------------------

void PursuitResults::CalculateRewards()
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
    float timeMultiplier = std::min(2.0f, pursuitTime / 60.0f);  // Up to 2x for 2+ min
    float copsMultiplier = 1.0f + (copsDisabled * 0.2f);  // +20% per cop
    float distanceMultiplier = std::min(2.0f, distanceTraveled / 5000.0f);  // Up to 2x for 5km+
    
    repEarned = (int)(baseRep * timeMultiplier * copsMultiplier * distanceMultiplier);
    cashEarned = (int)(baseCash * timeMultiplier * copsMultiplier * distanceMultiplier);
}
