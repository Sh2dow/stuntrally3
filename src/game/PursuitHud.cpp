#include "pch.h"
#include "PursuitHud.h"
#include "Pursuit.h"
#include "GuiScreens.h"
#include "Def_Str.h"

//--------------------------------------------------------------------------------------------------------------------------
// PursuitHud
//--------------------------------------------------------------------------------------------------------------------------

PursuitHud& PursuitHud::Get()
{
    static PursuitHud instance;
    return instance;
}

void PursuitHud::Initialize()
{
    visible = false;
    currentHeat = 0;
    cooldownPercent = 0.0f;
    currentCops = 0;
    maxCops = 0;
}

void PursuitHud::Shutdown()
{
    visible = false;
}

void PursuitHud::Show(bool show)
{
    visible = show;
}

void PursuitHud::Update(float dt)
{
    if (!visible) return;
    
    // Get current pursuit state
    PursuitManager& pursuit = PursuitManager::Get();
    
    if (pursuit.IsInPursuit())
    {
        // Update heat level
        int heat = pursuit.GetHeat().GetLevel();
        if (heat != currentHeat)
        {
            SetHeatLevel(heat);
        }
        
        // Update police count
        int cops = pursuit.GetActiveCopCount();
        int max = pursuit.GetMaxCopCount();
        SetPoliceCount(cops, max);
        
        // Check for heat increase
        static int lastHeat = 0;
        if (heat > lastHeat)
        {
            ShowHeatIncreasedMessage();
        }
        lastHeat = heat;
    }
    else if (pursuit.GetState() == PURSUIT_COOLDOWN)
    {
        // Update cooldown
        float cooldown = pursuit.GetCooldownTime();
        float maxCooldown = pursuit.GetMaxCooldownTime();
        SetCooldownTime(cooldown, maxCooldown);
    }
}

void PursuitHud::SetHeatLevel(int level)
{
    currentHeat = level;
    
    // TODO: Update heat meter visual
    // - Change color based on level (see HeatColors namespace)
    // - Update number display
    // - Play sound effect on heat increase
}

void PursuitHud::SetCooldownTime(float time, float maxTime)
{
    if (maxTime > 0.0f)
    {
        cooldownPercent = time / maxTime;
    }
    else
    {
        cooldownPercent = 0.0f;
    }
    
    // TODO: Update cooldown bar visual
    // - Fill bar based on cooldownPercent
    // - Change color from red to green as it fills
}

void PursuitHud::SetPoliceCount(int count, int maxCount)
{
    currentCops = count;
    maxCops = maxCount;
    
    // TODO: Update police count display
    // - Show cop icons or counter
    // - Show max cops for current heat level
}

void PursuitHud::ShowBustedMessage()
{
    // TODO: Show large "BUSTED" text
    // - Red color
    // - Fade in/out animation
    // - Play busted sound
}

void PursuitHud::ShowEscapedMessage()
{
    // TODO: Show large "ESCAPED!" text
    // - Green color
    // - Fade in/out animation
    // - Play escape sound
}

void PursuitHud::ShowHeatIncreasedMessage()
{
    // TODO: Show "HEAT LEVEL UP" message
    // - Flash current heat level
    // - Play alert sound
}

void PursuitHud::ShowRoadblockWarning(float distance)
{
    // TODO: Show roadblock warning
    // - Distance-based urgency
    // - Red warning icon
    // - Play warning sound
}


//--------------------------------------------------------------------------------------------------------------------------
// PursuitEventHud
//--------------------------------------------------------------------------------------------------------------------------

void PursuitEventHud::Initialize()
{
    targetHeat = 0;
    targetCops = 0;
    timeLimit = 0.0f;
}

void PursuitEventHud::Shutdown()
{
}

void PursuitEventHud::SetTargetHeat(int heat)
{
    targetHeat = heat;
    // TODO: Display target heat
}

void PursuitEventHud::SetTargetCops(int cops)
{
    targetCops = cops;
    // TODO: Display target cops
}

void PursuitEventHud::SetTimeLimit(float time)
{
    timeLimit = time;
    // TODO: Display time limit
}

void PursuitEventHud::SetCurrentHeat(int heat)
{
    // TODO: Update current heat display
    // - Compare to target heat
    // - Show progress bar
}

void PursuitEventHud::SetCopsDisabled(int count)
{
    // TODO: Update cops disabled display
    // - Compare to target
    // - Show cop icons with checkmarks
}

void PursuitEventHud::SetElapsedTime(float time)
{
    // TODO: Update elapsed time display
    // - Compare to time limit
    // - Change color when running low
}

void PursuitEventHud::ShowWinIndicator()
{
    // TODO: Show "OBJECTIVE COMPLETE" message
    // - Green color
    // - Success sound
}

void PursuitEventHud::ShowLossIndicator()
{
    // TODO: Show "OBJECTIVE FAILED" message
    // - Red color
    // - Failure sound
}
