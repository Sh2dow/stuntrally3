#pragma once
#include <string>
#include <vector>
#include <map>
#include <OgreColourValue.h>

/// 🚗 Vehicle Class Types
enum VehicleClass
{
	CLASS_TUNER = 0,      // Japanese imports, agile handling
	CLASS_MUSCLE,         // American muscle, high speed
	CLASS_EXOTIC,         // European exotics, balanced
	CLASS_SPORT,          // Sports cars, all-around
	CLASS_LUXURY,         // Luxury cars, comfort
	CLASS_OFFROAD,        // Off-road vehicles
	CLASS_COUNT
};

const static std::string VehicleClassNames[CLASS_COUNT] = {
	"Tuner", "Muscle", "Exotic", "Sport", "Luxury", "Offroad"
};

/// 🎨 Vehicle Class Profiles
/// Defines handling and presentation characteristics per class
struct VehicleClassProfile
{
	VehicleClass type = CLASS_TUNER;
	
	// Handling characteristics
	float steerSensitivity = 1.0f;     // Steering response
	float gripBonus = 0.0f;            // Extra grip
	float weightMult = 1.0f;           // Weight multiplier
	float powerMult = 1.0f;            // Power multiplier
	float brakeMult = 1.0f;            // Braking multiplier
	
	// Presentation
	float cameraDistance = 1.0f;       // Camera distance multiplier
	float cameraHeight = 1.0f;         // Camera height multiplier
	float fovModifier = 0.0f;          // FOV adjustment
	
	// Audio
	float enginePitchMod = 1.0f;       // Engine pitch modifier
	std::string engineSoundType;       // Sound type identifier
	
	// AI behavior
	float aiAggression = 0.5f;         // AI aggression level
	float aiSkill = 0.5f;              // AI skill level
	
	void ApplyToArcadeAssists(class ArcadeAssistParams& params) const;
};

/// 🎨 Paint Job Definition
struct PaintJob
{
	std::string id;
	std::string name;
	
	// Primary colors
	Ogre::ColourValue primary;
	Ogre::ColourValue secondary;
	
	// Special effects
	bool isMetallic = false;
	bool isPearl = false;
	bool isMatte = false;
	float metallicFlake = 0.0f;
	
	// Rims
	Ogre::ColourValue rimColor;
	int rimStyle = 0;
	
	// Windows
	Ogre::ColourValue windowTint;
	
	// Unlock requirements
	int requiredLevel = 0;
	int requiredRep = 0;
	int cost = 0;
};

/// 🏷️ Vinyl/Decal Definition
struct VinylDecal
{
	std::string id;
	std::string name;
	
	enum VinylType
	{
		VINYL_SHAPE,      // Basic shapes
		VINYL_STRIPE,     // Racing stripes
		VINYL_FLAME,      // Flame graphics
		VINYL_TRIBAL,     // Tribal patterns
		VINYL_NUMBER,     // Racing numbers
		VINYL_SPONSOR,    // Sponsor logos
		VINYL_CUSTOM      // Custom graphics
	};
	
	VinylType type = VINYL_SHAPE;
	
	// Placement
	int layer = 0;                    // Layer order (0 = bottom)
	float posX = 0.0f;                // X position (-1 to 1)
	float posY = 0.0f;                // Y position (-1 to 1)
	float width = 1.0f;               // Width scale
	float height = 1.0f;              // Height scale
	float rotation = 0.0f;            // Rotation in degrees
	
	// Appearance
	Ogre::ColourValue color;
	float opacity = 1.0f;
	bool mirrorX = false;             // Mirror on X axis
	bool mirrorY = false;             // Mirror on Y axis
	
	// Unlock requirements
	int requiredLevel = 0;
	int cost = 0;
};

/// 🎨 Full Vehicle Livery
struct VehicleLivery
{
	std::string id;
	std::string name;
	
	int paintJobId = 0;
	std::vector<VinylDecal> decals;   // Applied decals
	
	// Preset flag
	bool isPreset = true;
	
	// Unlock requirements
	int requiredLevel = 0;
	int requiredRep = 0;
};

/// ⚡ Performance Upgrade
struct PerformanceUpgrade
{
	std::string id;
	std::string name;
	
	enum UpgradeType
	{
		UPGRADE_ENGINE,     // Engine improvements
		UPGRADE_NITROUS,    // Nitrous system
		UPGRADE_TRANSMISSION, // Transmission
		UPGRADE_TIRES,      // Tires
		UPGRADE_BRAKES,     // Brakes
		UPGRADE_SUSPENSION, // Suspension
		UPGRADE_WEIGHT,     // Weight reduction
		UPGRADE_AERO        // Aerodynamics
	};
	
	UpgradeType type = UPGRADE_ENGINE;
	int level = 0;                // 0-3 upgrade level
	
	// Performance bonuses
	float powerBonus = 0.0f;      // Power increase %
	float torqueBonus = 0.0f;     // Torque increase %
	float weightReduction = 0.0f; // Weight reduction %
	float gripBonus = 0.0f;       // Grip increase %
	float brakeBonus = 0.0f;      // Braking increase %
	
	// Visual changes
	bool changesAppearance = false;
	std::string visualMesh;       // Mesh to swap
	
	// Requirements
	int requiredLevel = 0;
	int cost = 0;
	
	// Compatibility
	std::vector<VehicleClass> compatibleClasses;
};

/// 🔧 Full Vehicle Build
struct VehicleBuild
{
	std::string carId;
	std::string buildName;
	
	// Visual
	int paintJobId = 0;
	std::vector<VinylDecal> decals;
	
	// Performance
	std::map<std::string, int> upgrades;  // upgradeId -> level
	
	// Stats (calculated)
	float power = 0.0f;
	float handling = 0.0f;
	float acceleration = 0.0f;
	float topSpeed = 0.0f;
	
	// Calculate stats from upgrades
	void CalculateStats();
};

/// 🏪 Customization Shop/Manager
class VehicleCustomization
{
public:
	static VehicleCustomization& Get();
	
	// Initialization
	bool Initialize();
	void Shutdown();
	
	// Vehicle class profiles
	const VehicleClassProfile& GetClassProfile(VehicleClass type) const;
	void ApplyClassEffects(VehicleClass type, class CAR* car) const;
	
	// Paint jobs
	const PaintJob* GetPaintJob(int id) const;
	std::vector<const PaintJob*> GetAvailablePaintJobs(int playerLevel) const;
	void ApplyPaintJob(int carId, int paintJobId);
	
	// Vinyls/Decals
	const VinylDecal* GetVinyl(int id) const;
	std::vector<const VinylDecal*> GetAvailableVinyls(int playerLevel) const;
	void AddVinyl(int carId, const VinylDecal& decal);
	void RemoveVinyl(int carId, int index);
	void MoveVinylLayer(int carId, int index, int newLayer);
	
	// Liveries (presets)
	const VehicleLivery* GetLivery(int id) const;
	std::vector<const VehicleLivery*> GetAllLiveries() const;
	void ApplyLivery(int carId, int liveryId);
	void SaveLivery(int carId, const std::string& name);
	
	// Performance upgrades
	const PerformanceUpgrade* GetUpgrade(const std::string& id) const;
	std::vector<const PerformanceUpgrade*> GetUpgradesByType(
		PerformanceUpgrade::UpgradeType type) const;
	void InstallUpgrade(int carId, const std::string& upgradeId);
	void RemoveUpgrade(int carId, const std::string& upgradeId);
	
	// Vehicle builds
	VehicleBuild& GetVehicleBuild(int carId);
	const VehicleBuild& GetVehicleBuild(int carId) const;
	void SaveBuild(int carId, const std::string& buildName);
	void LoadBuild(int carId, const std::string& buildName);
	
	// Stats calculation
	void CalculateVehicleStats(int carId);
	float GetClassBonus(VehicleClass type, const std::string& stat) const;
	
private:
	VehicleCustomization() = default;
	
	std::vector<VehicleClassProfile> classProfiles;
	std::vector<PaintJob> paintJobs;
	std::vector<VinylDecal> vinyls;
	std::vector<VehicleLivery> liveries;
	std::vector<PerformanceUpgrade> upgrades;
	
	std::map<int, VehicleBuild> vehicleBuilds;  // carId -> build
	
	void InitializeClassProfiles();
	void InitializeDefaultPaintJobs();
	void InitializeDefaultVinyls();
	void InitializeDefaultUpgrades();
};
