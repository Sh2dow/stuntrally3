#pragma once
#include <string>
#include <vector>
#include <OgreVector3.h>
#include <OgreColourValue.h>

class EventConfig;
class District;

/// 🎨 GUI Element Types
enum GuiElementType
{
    GUI_BUTTON,
    GUI_LABEL,
    GUI_IMAGE,
    GUI_PANEL,
    GUI_LIST,
    GUI_MAP,
    GUI_GAUGE,
    GUI_PROGRESS
};

/// 🎨 Base GUI Element
struct GuiElement
{
    GuiElementType type;
    std::string id;
    std::string text;
    
    // Position/Size (normalized 0-1)
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.1f;
    float height = 0.05f;
    
    // Appearance
    Ogre::ColourValue color{1.0f, 1.0f, 1.0f, 1.0f};
    Ogre::ColourValue bgColor{0.0f, 0.0f, 0.0f, 0.5f};
    std::string texture;
    
    // State
    bool visible = true;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    
    // Callbacks
    std::string onClick;
    std::string onHover;
    
    // Equality operator for std::find
    bool operator==(const GuiElement& other) const { return id == other.id; }
};

/// 🎮 FE Screen States
enum FeScreenState
{
    FE_MAIN_MENU,
    FE_CAREER_MAP,
    FE_EVENT_SELECT,
    FE_GARAGE,
    FE_CUSTOMIZATION,
    FE_RESULTS
};

/// 🏠 Main Menu Screen
class MainMenuScreen
{
public:
    void Initialize();
    void Update(float dt);
    void Render();
    void Shutdown();
    
    bool OnButtonClick(const std::string& buttonId);
    
private:
    std::vector<GuiElement> elements;
    FeScreenState currentState;
};

/// 🗺️ Career World Map Screen
class CareerMapScreen
{
public:
    void Initialize();
    void Update(float dt);
    void Render();
    void Shutdown();
    
    void SetCurrentDistrict(int districtId);
    void ShowDistrictEvents(int districtId);
    
    bool OnDistrictClick(int districtId);
    bool OnEventClick(const std::string& eventId);
    
private:
    std::vector<GuiElement> districtMarkers;
    std::vector<GuiElement> eventMarkers;
    int currentDistrict = 0;
    int unlockedDistricts = 1;
};

/// 🏁 Event Selection Screen
class EventSelectScreen
{
public:
    void Initialize();
    void Update(float dt);
    void Render();
    void Shutdown();
    
    void SetDistrict(int districtId);
    void ShowEventDetails(const std::string& eventId);
    
    bool OnEventClick(const std::string& eventId);
    bool OnStartRace();
    
private:
    std::vector<GuiElement> eventCards;
    GuiElement eventDetails;
    int currentDistrict = 0;
    std::string selectedEvent;
};

/// 🚗 Garage/Customization Screen
class GarageScreen
{
public:
    void Initialize();
    void Update(float dt);
    void Render();
    void Shutdown();
    
    void SetCurrentCar(int carId);
    void ShowPaintJobs();
    void ShowVinyls();
    void ShowUpgrades();
    
    bool OnPaintClick(int paintId);
    bool OnVinylClick(int vinylId);
    bool OnUpgradeClick(const std::string& upgradeId);
    
private:
    std::vector<GuiElement> carPreviews;
    std::vector<GuiElement> paintSwatches;
    std::vector<GuiElement> vinylIcons;
    std::vector<GuiElement> upgradeCards;
    int currentCar = 0;
};

/// 🏆 Results Screen
class ResultsScreen
{
public:
    void Initialize();
    void Update(float dt);
    void Render();
    void Shutdown();
    
    void ShowResults(int position, int total, float time, int repEarned, int cashEarned);
    void ShowPursuitResults(int heat, int copsDisabled, bool escaped);
    
    bool OnContinue();
    
private:
    GuiElement positionDisplay;
    GuiElement timeDisplay;
    GuiElement repDisplay;
    GuiElement cashDisplay;
    GuiElement continueButton;
};

/// 🎮 GUI Manager
class GuiManager
{
public:
    static GuiManager& Get();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Screen management
    void ShowScreen(FeScreenState screen);
    void HideCurrentScreen();
    
    // Current screen access
    MainMenuScreen* GetMainMenu() { return &mainMenu; }
    CareerMapScreen* GetCareerMap() { return &careerMap; }
    EventSelectScreen* GetEventSelect() { return &eventSelect; }
    GarageScreen* GetGarage() { return &garage; }
    ResultsScreen* GetResults() { return &results; }
    
    // Input handling
    bool OnMouseClick(float x, float y);
    bool OnMouseHover(float x, float y);
    bool OnKeyPress(int keyCode);
    
    // Element creation
    GuiElement* CreateElement(GuiElementType type, const std::string& id);
    void DestroyElement(const std::string& id);
    GuiElement* GetElement(const std::string& id);
    
    // Get all elements for rendering
    const std::vector<GuiElement>& GetElements() const { return elements; }
    
    // Rendering
    void RenderElement(const GuiElement& elem);
    void RenderText(float x, float y, const std::string& text, float size);
    void RenderImage(float x, float y, float w, float h, const std::string& texture);
    
private:
    GuiManager() = default;
    
    MainMenuScreen mainMenu;
    CareerMapScreen careerMap;
    EventSelectScreen eventSelect;
    GarageScreen garage;
    ResultsScreen results;
    
    FeScreenState currentScreen = FE_MAIN_MENU;
    std::vector<GuiElement> elements;
    std::map<std::string, GuiElement*> elementMap;
};
