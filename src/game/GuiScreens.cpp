#include "pch.h"
#include "GuiScreens.h"
#include "Career.h"
#include "EventMode.h"
#include "VehicleCustomization.h"
#include "Def_Str.h"

//--------------------------------------------------------------------------------------------------------------------------
// GuiManager
//--------------------------------------------------------------------------------------------------------------------------

GuiManager& GuiManager::Get()
{
    static GuiManager instance;
    return instance;
}

bool GuiManager::Initialize()
{
    mainMenu.Initialize();
    careerMap.Initialize();
    eventSelect.Initialize();
    garage.Initialize();
    results.Initialize();
    
    ShowScreen(FE_MAIN_MENU);
    
    return true;
}

void GuiManager::Shutdown()
{
    mainMenu.Shutdown();
    careerMap.Shutdown();
    eventSelect.Shutdown();
    garage.Shutdown();
    results.Shutdown();
    
    elements.clear();
    elementMap.clear();
}

void GuiManager::ShowScreen(FeScreenState screen)
{
    HideCurrentScreen();
    currentScreen = screen;
    
    switch (screen)
    {
        case FE_MAIN_MENU:
            mainMenu.Initialize();
            break;
        case FE_CAREER_MAP:
            careerMap.Initialize();
            break;
        case FE_EVENT_SELECT:
            eventSelect.Initialize();
            break;
        case FE_GARAGE:
        case FE_CUSTOMIZATION:
            garage.Initialize();
            break;
        case FE_RESULTS:
            results.Initialize();
            break;
    }
}

void GuiManager::HideCurrentScreen()
{
    switch (currentScreen)
    {
        case FE_MAIN_MENU:
            mainMenu.Shutdown();
            break;
        case FE_CAREER_MAP:
            careerMap.Shutdown();
            break;
        case FE_EVENT_SELECT:
            eventSelect.Shutdown();
            break;
        case FE_GARAGE:
        case FE_CUSTOMIZATION:
            garage.Shutdown();
            break;
        case FE_RESULTS:
            results.Shutdown();
            break;
    }
}

bool GuiManager::OnMouseClick(float x, float y)
{
    switch (currentScreen)
    {
        case FE_MAIN_MENU:
            return mainMenu.OnButtonClick("");
        case FE_CAREER_MAP:
            return careerMap.OnDistrictClick(0);
        case FE_EVENT_SELECT:
            return eventSelect.OnStartRace();
        case FE_GARAGE:
        case FE_CUSTOMIZATION:
            return garage.OnPaintClick(0);
        case FE_RESULTS:
            return results.OnContinue();
    }
    return false;
}

bool GuiManager::OnMouseHover(float x, float y)
{
    // Check all visible elements for hover
    for (auto& elem : elements)
    {
        if (!elem.visible) continue;
        
        if (x >= elem.x && x <= elem.x + elem.width &&
            y >= elem.y && y <= elem.y + elem.height)
        {
            elem.hovered = true;
            if (!elem.onHover.empty())
            {
                // Trigger hover callback
            }
        }
        else
        {
            elem.hovered = false;
        }
    }
    return false;
}

bool GuiManager::OnKeyPress(int keyCode)
{
    // Handle keyboard navigation
    return false;
}

GuiElement* GuiManager::CreateElement(GuiElementType type, const std::string& id)
{
    GuiElement elem;
    elem.type = type;
    elem.id = id;
    
    elements.push_back(elem);
    elementMap[id] = &elements.back();
    
    return &elements.back();
}

void GuiManager::DestroyElement(const std::string& id)
{
    auto it = elementMap.find(id);
    if (it != elementMap.end())
    {
        elements.erase(std::find(elements.begin(), elements.end(), *it->second));
        elementMap.erase(it);
    }
}

GuiElement* GuiManager::GetElement(const std::string& id)
{
    auto it = elementMap.find(id);
    if (it != elementMap.end())
        return it->second;
    return nullptr;
}

void GuiManager::RenderElement(const GuiElement& elem)
{
    if (!elem.visible) return;
    
    // Render background
    if (elem.type == GUI_PANEL || elem.type == GUI_BUTTON)
    {
        // TODO: Render colored rectangle
    }
    
    // Render text
    if (!elem.text.empty())
    {
        RenderText(elem.x, elem.y, elem.text, elem.height * 0.7f);
    }
    
    // Render image
    if (!elem.texture.empty())
    {
        RenderImage(elem.x, elem.y, elem.width, elem.height, elem.texture);
    }
}

void GuiManager::RenderText(float x, float y, const std::string& text, float size)
{
    // TODO: Use MyGUI or Ogre overlay to render text
    // For now, this is a stub
}

void GuiManager::RenderImage(float x, float y, float w, float h, const std::string& texture)
{
    // TODO: Use Ogre to render textured quad
    // For now, this is a stub
}


//--------------------------------------------------------------------------------------------------------------------------
// MainMenuScreen
//--------------------------------------------------------------------------------------------------------------------------

void MainMenuScreen::Initialize()
{
    currentState = FE_MAIN_MENU;
    
    // Create main menu buttons
    auto& gui = GuiManager::Get();
    
    // Career button
    GuiElement* careerBtn = gui.CreateElement(GUI_BUTTON, "btn_career");
    careerBtn->text = "Career";
    careerBtn->x = 0.4f;
    careerBtn->y = 0.3f;
    careerBtn->width = 0.2f;
    careerBtn->height = 0.08f;
    careerBtn->onClick = "show_career_map";
    
    // Quick race button
    GuiElement* quickBtn = gui.CreateElement(GUI_BUTTON, "btn_quick");
    quickBtn->text = "Quick Race";
    quickBtn->x = 0.4f;
    quickBtn->y = 0.4f;
    quickBtn->width = 0.2f;
    quickBtn->height = 0.08f;
    quickBtn->onClick = "show_quick_race";
    
    // Garage button
    GuiElement* garageBtn = gui.CreateElement(GUI_BUTTON, "btn_garage");
    garageBtn->text = "Garage";
    garageBtn->x = 0.4f;
    garageBtn->y = 0.5f;
    garageBtn->width = 0.2f;
    garageBtn->height = 0.08f;
    garageBtn->onClick = "show_garage";
    
    // Options button
    GuiElement* optionsBtn = gui.CreateElement(GUI_BUTTON, "btn_options");
    optionsBtn->text = "Options";
    optionsBtn->x = 0.4f;
    optionsBtn->y = 0.6f;
    optionsBtn->width = 0.2f;
    optionsBtn->height = 0.08f;
    optionsBtn->onClick = "show_options";
    
    // Exit button
    GuiElement* exitBtn = gui.CreateElement(GUI_BUTTON, "btn_exit");
    exitBtn->text = "Exit";
    exitBtn->x = 0.4f;
    exitBtn->y = 0.7f;
    exitBtn->width = 0.2f;
    exitBtn->height = 0.08f;
    exitBtn->onClick = "exit_game";
}

void MainMenuScreen::Update(float dt)
{
    // Update button states
}

void MainMenuScreen::Render()
{
    auto& gui = GuiManager::Get();
    
    // Render title
    gui.RenderText(0.5f, 0.15f, "STUNT RALLY 3", 0.1f);
    
    // Render all elements
    for (const auto& elem : gui.GetElements())
    {
        if (elem.visible)
            gui.RenderElement(elem);
    }
}

void MainMenuScreen::Shutdown()
{
    auto& gui = GuiManager::Get();
    // Don't clear all elements, just let them be overwritten by next screen
}

bool MainMenuScreen::OnButtonClick(const std::string& buttonId)
{
    auto& gui = GuiManager::Get();
    
    if (buttonId == "btn_career" || buttonId.empty())
    {
        gui.ShowScreen(FE_CAREER_MAP);
        return true;
    }
    else if (buttonId == "btn_garage")
    {
        gui.ShowScreen(FE_GARAGE);
        return true;
    }
    else if (buttonId == "btn_exit")
    {
        // Exit game
        return true;
    }
    
    return false;
}


//--------------------------------------------------------------------------------------------------------------------------
// CareerMapScreen
//--------------------------------------------------------------------------------------------------------------------------

void CareerMapScreen::Initialize()
{
    auto& gui = GuiManager::Get();
    
    // Get career data
    auto& career = CareerManager::Get();
    auto districts = career.GetAllDistricts();
    
    // Create district markers on map
    float mapX = 0.1f;
    float mapY = 0.2f;
    float spacing = 0.15f;
    
    for (size_t i = 0; i < districts.size() && i < 5; ++i)
    {
        const District* district = districts[i];
        
        std::string markerId = "district_" + std::to_string(district->id);
        GuiElement* marker = gui.CreateElement(GUI_BUTTON, markerId);
        
        marker->x = mapX + (i % 3) * spacing;
        marker->y = mapY + (i / 3) * spacing;
        marker->width = 0.1f;
        marker->height = 0.1f;
        marker->text = district->name;
        
        // Check if unlocked
        if (career.CanAccessDistrict(district->id))
        {
            marker->enabled = true;
            marker->color = Ogre::ColourValue::Green;
        }
        else
        {
            marker->enabled = false;
            marker->color = Ogre::ColourValue::Red;
            marker->text += " (Locked)";
        }
        
        districtMarkers.push_back(*marker);
    }
    
    // Map background
    GuiElement* mapBg = gui.CreateElement(GUI_PANEL, "map_bg");
    mapBg->x = 0.05f;
    mapBg->y = 0.15f;
    mapBg->width = 0.9f;
    mapBg->height = 0.7f;
    mapBg->bgColor = Ogre::ColourValue(0.2f, 0.2f, 0.3f, 0.8f);
}

void CareerMapScreen::Update(float dt)
{
    // Update district markers based on career progress
}

void CareerMapScreen::Render()
{
    auto& gui = GuiManager::Get();
    
    // Render map title
    gui.RenderText(0.5f, 0.1f, "Career Map", 0.06f);
    
    // Render district markers
    for (const auto& marker : districtMarkers)
    {
        if (marker.visible)
            gui.RenderElement(marker);
    }
}

void CareerMapScreen::Shutdown()
{
    districtMarkers.clear();
    eventMarkers.clear();
    // Elements will be overwritten by next screen
}

void CareerMapScreen::SetCurrentDistrict(int districtId)
{
    currentDistrict = districtId;
}

void CareerMapScreen::ShowDistrictEvents(int districtId)
{
    auto& career = CareerManager::Get();
    auto events = career.GetDistrictEvents(districtId);
    
    // Create event markers
    // TODO: Position them on the map
}

bool CareerMapScreen::OnDistrictClick(int districtId)
{
    auto& career = CareerManager::Get();
    
    if (career.CanAccessDistrict(districtId))
    {
        currentDistrict = districtId;
        ShowDistrictEvents(districtId);
        
        // Switch to event select screen
        auto& gui = GuiManager::Get();
        gui.ShowScreen(FE_EVENT_SELECT);
        return true;
    }
    
    return false;
}

bool CareerMapScreen::OnEventClick(const std::string& eventId)
{
    // Show event details or start event
    return false;
}


//--------------------------------------------------------------------------------------------------------------------------
// EventSelectScreen
//--------------------------------------------------------------------------------------------------------------------------

void EventSelectScreen::Initialize()
{
    auto& gui = GuiManager::Get();
    auto& career = CareerManager::Get();
    
    // Get events for current district
    auto events = career.GetDistrictEvents(currentDistrict);
    
    // Create event cards
    float cardX = 0.1f;
    float cardY = 0.25f;
    float cardSpacing = 0.22f;
    
    for (size_t i = 0; i < events.size() && i < 6; ++i)
    {
        const std::string& eventId = events[i];
        const EventConfig* config = EventManager::Get().GetEventConfig(eventId);
        
        if (!config) continue;
        
        std::string cardId = "event_" + eventId;
        GuiElement* card = gui.CreateElement(GUI_BUTTON, cardId);
        
        card->x = cardX + (i % 3) * cardSpacing;
        card->y = cardY + (i / 3) * cardSpacing * 1.5f;
        card->width = 0.2f;
        card->height = 0.15f;
        card->text = config->name;
        
        // Show event type icon
        std::string typeIcon;
        switch (config->type)
        {
            case EVENT_SPRINT: typeIcon = "sprint_icon"; break;
            case EVENT_CIRCUIT: typeIcon = "circuit_icon"; break;
            case EVENT_DRIFT: typeIcon = "drift_icon"; break;
            case EVENT_CANYON_DUEL: typeIcon = "canyon_icon"; break;
            case EVENT_PURSUIT: typeIcon = "pursuit_icon"; break;
            case EVENT_BOSS: typeIcon = "boss_icon"; break;
            default: typeIcon = "race_icon"; break;
        }
        
        eventCards.push_back(*card);
    }
    
    // Event details panel
    GuiElement* details = gui.CreateElement(GUI_PANEL, "event_details");
    details->x = 0.55f;
    details->y = 0.25f;
    details->width = 0.35f;
    details->height = 0.5f;
    details->bgColor = Ogre::ColourValue(0.1f, 0.1f, 0.2f, 0.9f);
    
    eventDetails = *details;
}

void EventSelectScreen::Update(float dt)
{
    // Update event cards
}

void EventSelectScreen::Render()
{
    auto& gui = GuiManager::Get();
    
    // Render title
    gui.RenderText(0.5f, 0.1f, "Select Event", 0.06f);
    
    // Render event cards
    for (const auto& card : eventCards)
    {
        if (card.visible)
            gui.RenderElement(card);
    }
    
    // Render event details
    if (!selectedEvent.empty())
    {
        ShowEventDetails(selectedEvent);
    }
}

void EventSelectScreen::Shutdown()
{
    eventCards.clear();
    // Elements will be overwritten by next screen
}

void EventSelectScreen::SetDistrict(int districtId)
{
    currentDistrict = districtId;
}

void EventSelectScreen::ShowEventDetails(const std::string& eventId)
{
    const EventConfig* config = EventManager::Get().GetEventConfig(eventId);
    if (!config) return;
    
    auto& gui = GuiManager::Get();
    
    // Update details panel with event info
    std::string details = config->name + "\n";
    details += config->description + "\n\n";
    details += "Reward: $" + std::to_string(config->rewardCash) + "\n";
    details += "Rep: +" + std::to_string(config->rewardRep);
    
    // TODO: Render details text
}

bool EventSelectScreen::OnEventClick(const std::string& eventId)
{
    selectedEvent = eventId;
    ShowEventDetails(eventId);
    return true;
}

bool EventSelectScreen::OnStartRace()
{
    if (selectedEvent.empty())
        return false;
    
    const EventConfig* config = EventManager::Get().GetEventConfig(selectedEvent);
    if (!config)
        return false;
    
    // Start the event
    EventManager::Get().StartEvent(*config);
    
    return true;
}


//--------------------------------------------------------------------------------------------------------------------------
// GarageScreen
//--------------------------------------------------------------------------------------------------------------------------

void GarageScreen::Initialize()
{
    auto& gui = GuiManager::Get();
    auto& customization = VehicleCustomization::Get();
    
    // Car preview area
    GuiElement* preview = gui.CreateElement(GUI_PANEL, "car_preview");
    preview->x = 0.05f;
    preview->y = 0.15f;
    preview->width = 0.4f;
    preview->height = 0.6f;
    preview->bgColor = Ogre::ColourValue(0.1f, 0.1f, 0.15f, 0.8f);
    
    // Paint job selector
    auto paintJobs = customization.GetAvailablePaintJobs(1);  // Player level 1
    float paintX = 0.5f;
    float paintY = 0.2f;
    
    for (size_t i = 0; i < paintJobs.size() && i < 6; ++i)
    {
        const PaintJob* paint = paintJobs[i];
        
        std::string swatchId = "paint_" + std::to_string(i);
        GuiElement* swatch = gui.CreateElement(GUI_BUTTON, swatchId);
        
        swatch->x = paintX;
        swatch->y = paintY + i * 0.08f;
        swatch->width = 0.08f;
        swatch->height = 0.06f;
        swatch->text = paint->name;
        
        paintSwatches.push_back(*swatch);
    }
    
    // Upgrade tabs
    GuiElement* engineTab = gui.CreateElement(GUI_BUTTON, "tab_engine");
    engineTab->text = "Engine";
    engineTab->x = 0.5f;
    engineTab->y = 0.65f;
    engineTab->width = 0.1f;
    engineTab->height = 0.05f;
    
    GuiElement* tiresTab = gui.CreateElement(GUI_BUTTON, "tab_tires");
    tiresTab->text = "Tires";
    tiresTab->x = 0.62f;
    tiresTab->y = 0.65f;
    tiresTab->width = 0.1f;
    tiresTab->height = 0.05f;
}

void GarageScreen::Update(float dt)
{
    // Update garage UI
}

void GarageScreen::Render()
{
    auto& gui = GuiManager::Get();
    
    // Render title
    gui.RenderText(0.5f, 0.1f, "Garage", 0.06f);
    
    // Render all elements
}

void GarageScreen::Shutdown()
{
    paintSwatches.clear();
    vinylIcons.clear();
    upgradeCards.clear();
    // Elements will be overwritten by next screen
}

void GarageScreen::SetCurrentCar(int carId)
{
    currentCar = carId;
}

void GarageScreen::ShowPaintJobs()
{
    // Show paint job selector
}

void GarageScreen::ShowVinyls()
{
    // Show vinyl selector
}

void GarageScreen::ShowUpgrades()
{
    // Show upgrade selector
}

bool GarageScreen::OnPaintClick(int paintId)
{
    auto& customization = VehicleCustomization::Get();
    customization.ApplyPaintJob(currentCar, paintId);
    return true;
}

bool GarageScreen::OnVinylClick(int vinylId)
{
    // Apply vinyl
    return true;
}

bool GarageScreen::OnUpgradeClick(const std::string& upgradeId)
{
    auto& customization = VehicleCustomization::Get();
    customization.InstallUpgrade(currentCar, upgradeId);
    return true;
}


//--------------------------------------------------------------------------------------------------------------------------
// ResultsScreen
//--------------------------------------------------------------------------------------------------------------------------

void ResultsScreen::Initialize()
{
    auto& gui = GuiManager::Get();
    
    // Position display
    positionDisplay = *gui.CreateElement(GUI_LABEL, "result_position");
    positionDisplay.x = 0.4f;
    positionDisplay.y = 0.2f;
    positionDisplay.width = 0.2f;
    positionDisplay.height = 0.1f;
    
    // Time display
    timeDisplay = *gui.CreateElement(GUI_LABEL, "result_time");
    timeDisplay.x = 0.4f;
    timeDisplay.y = 0.35f;
    timeDisplay.width = 0.2f;
    timeDisplay.height = 0.08f;
    
    // Rep display
    repDisplay = *gui.CreateElement(GUI_LABEL, "result_rep");
    repDisplay.x = 0.4f;
    repDisplay.y = 0.45f;
    repDisplay.width = 0.2f;
    repDisplay.height = 0.06f;
    
    // Cash display
    cashDisplay = *gui.CreateElement(GUI_LABEL, "result_cash");
    cashDisplay.x = 0.4f;
    cashDisplay.y = 0.53f;
    cashDisplay.width = 0.2f;
    cashDisplay.height = 0.06f;
    
    // Continue button
    continueButton = *gui.CreateElement(GUI_BUTTON, "btn_continue");
    continueButton.x = 0.4f;
    continueButton.y = 0.65f;
    continueButton.width = 0.2f;
    continueButton.height = 0.08f;
    continueButton.text = "Continue";
    continueButton.onClick = "continue_from_results";
}

void ResultsScreen::Update(float dt)
{
    // Update results display
}

void ResultsScreen::Render()
{
    auto& gui = GuiManager::Get();
    
    // Render title
    gui.RenderText(0.5f, 0.1f, "Race Results", 0.06f);
    
    // Render all elements
}

void ResultsScreen::Shutdown()
{
    // Elements will be overwritten by next screen
}

void ResultsScreen::ShowResults(int position, int total, float time, int repEarned, int cashEarned)
{
    auto& gui = GuiManager::Get();
    
    // Update position
    std::string posText;
    if (position == 1) posText = "1st Place";
    else if (position == 2) posText = "2nd Place";
    else if (position == 3) posText = "3rd Place";
    else posText = std::to_string(position) + "th Place";
    
    // Update time
    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "Time: %.2fs", time);
    
    // Update rewards
    char repBuf[32];
    snprintf(repBuf, sizeof(repBuf), "Rep: +%d", repEarned);
    
    char cashBuf[32];
    snprintf(cashBuf, sizeof(cashBuf), "Cash: $%d", cashEarned);
    
    // TODO: Update label texts
}

void ResultsScreen::ShowPursuitResults(int heat, int copsDisabled, bool escaped)
{
    auto& gui = GuiManager::Get();
    
    std::string title = escaped ? "ESCAPED!" : "BUSTED!";
    
    // TODO: Update title and details
}

bool ResultsScreen::OnContinue()
{
    auto& gui = GuiManager::Get();
    gui.ShowScreen(FE_CAREER_MAP);
    return true;
}
