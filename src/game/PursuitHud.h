#pragma once
#include <string>
#include <OgreColourValue.h>

/// 🚓 Pursuit HUD Elements
class PursuitHud
{
public:
    static PursuitHud& Get();
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Visibility
    void Show(bool show);
    bool IsVisible() const { return visible; }
    
    // Update
    void Update(float dt);
    
    // Heat level display
    void SetHeatLevel(int level);
    int GetHeatLevel() const { return currentHeat; }
    
    // Cooldown display
    void SetCooldownTime(float time, float maxTime);
    float GetCooldownPercent() const { return cooldownPercent; }
    
    // Police count
    void SetPoliceCount(int count, int maxCount);
    
    // Busted/Escaped messages
    void ShowBustedMessage();
    void ShowEscapedMessage();
    void ShowHeatIncreasedMessage();
    
    // Roadblock warning
    void ShowRoadblockWarning(float distance);
    
private:
    PursuitHud() = default;
    
    bool visible = false;
    int currentHeat = 0;
    float cooldownPercent = 0.0f;
    int currentCops = 0;
    int maxCops = 0;
    
    // HUD element positions (normalized 0-1)
    float heatMeterX = 0.85f;
    float heatMeterY = 0.1f;
    float heatMeterWidth = 0.12f;
    float heatMeterHeight = 0.08f;
    
    float cooldownBarX = 0.5f;
    float cooldownBarY = 0.05f;
    float cooldownBarWidth = 0.3f;
    float cooldownBarHeight = 0.03f;
    
    float policeCountX = 0.85f;
    float policeCountY = 0.2f;
};

/// 🎯 Pursuit Event HUD
class PursuitEventHud
{
public:
    void Initialize();
    void Shutdown();
    
    // Target display
    void SetTargetHeat(int heat);
    void SetTargetCops(int cops);
    void SetTimeLimit(float time);
    
    // Progress display
    void SetCurrentHeat(int heat);
    void SetCopsDisabled(int count);
    void SetElapsedTime(float time);
    
    // Win/Loss indicators
    void ShowWinIndicator();
    void ShowLossIndicator();
    
private:
    int targetHeat = 0;
    int targetCops = 0;
    float timeLimit = 0.0f;
};

/// 🌡️ Heat Level Colors
namespace HeatColors
{
    const Ogre::ColourValue Level1{0.0f, 1.0f, 0.0f, 1.0f};    // Green
    const Ogre::ColourValue Level2{0.5f, 1.0f, 0.0f, 1.0f};    // Yellow-Green
    const Ogre::ColourValue Level3{1.0f, 1.0f, 0.0f, 1.0f};    // Yellow
    const Ogre::ColourValue Level4{1.0f, 0.5f, 0.0f, 1.0f};    // Orange
    const Ogre::ColourValue Level5{1.0f, 0.0f, 0.0f, 1.0f};    // Red
}
