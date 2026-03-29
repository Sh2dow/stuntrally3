#include "pch.h"
#include "VehicleCustomization.h"
#include "car.h"
#include "ArcadeHandling.h"
#include "Def_Str.h"

//--------------------------------------------------------------------------------------------------------------------------
// VehicleClassProfile
//--------------------------------------------------------------------------------------------------------------------------

void VehicleClassProfile::ApplyToArcadeAssists(ArcadeAssistParams& params) const
{
	// Modify arcade handling based on vehicle class
	switch (type)
	{
		case CLASS_TUNER:
			// Tuners: high steering sensitivity, good drift
			params.steerSpeedFactor *= 0.8f;  // Less speed sensitivity
			params.driftAssistEnabled = 1.0f;
			params.driftYawGain *= 1.2f;
			break;
			
		case CLASS_MUSCLE:
			// Muscle: high speed, less cornering
			params.downforceSpeedMult *= 1.3f;  // More downforce
			params.yawStabGain *= 1.2f;  // More stability
			params.driftAssistEnabled = 0.5f;  // Less drift assist
			break;
			
		case CLASS_EXOTIC:
			// Exotics: balanced, high grip
			params.gripSpeedMult *= 1.2f;
			params.weightTransferEnabled = 1.0f;
			params.weightTransferLat *= 0.8f;  // Less body roll
			break;
			
		case CLASS_SPORT:
			// Sports: all-around good
			params.assistStrength = 0.8f;  // Slightly less assists
			break;
			
		case CLASS_LUXURY:
			// Luxury: comfortable, soft handling
			params.yawStabGain *= 1.3f;  // More stability
			params.weightTransferLong *= 0.7f;  // Less dive/squat
			break;
			
		case CLASS_OFFROAD:
			// Offroad: high grip on rough surfaces
			params.gripFrontBase *= 1.1f;
			params.gripRearBase *= 1.1f;
			break;
			
		default:
			break;
	}
}


//--------------------------------------------------------------------------------------------------------------------------
// VehicleBuild
//--------------------------------------------------------------------------------------------------------------------------

void VehicleBuild::CalculateStats()
{
	// Base stats would come from car's base configuration
	// Upgrades modify these values
	
	float basePower = 100.0f;
	float baseHandling = 100.0f;
	float baseAcceleration = 100.0f;
	float baseTopSpeed = 100.0f;
	
	// Apply upgrade bonuses
	for (const auto& pair : upgrades)
	{
		// TODO: Get upgrade data and apply bonuses
		// const PerformanceUpgrade* upgrade = VehicleCustomization::Get().GetUpgrade(pair.first);
		// if (upgrade)
		// {
		//     power += basePower * upgrade->powerBonus * (upgrade->level / 3.0f);
		//     handling += baseHandling * upgrade->gripBonus * (upgrade->level / 3.0f);
		//     // etc.
		// }
	}
	
	power = basePower;
	handling = baseHandling;
	acceleration = baseAcceleration;
	topSpeed = baseTopSpeed;
}


//--------------------------------------------------------------------------------------------------------------------------
// VehicleCustomization
//--------------------------------------------------------------------------------------------------------------------------

VehicleCustomization& VehicleCustomization::Get()
{
	static VehicleCustomization instance;
	return instance;
}

bool VehicleCustomization::Initialize()
{
	InitializeClassProfiles();
	InitializeDefaultPaintJobs();
	InitializeDefaultVinyls();
	InitializeDefaultUpgrades();
	
	return true;
}

void VehicleCustomization::Shutdown()
{
	classProfiles.clear();
	paintJobs.clear();
	vinyls.clear();
	liveries.clear();
	upgrades.clear();
	vehicleBuilds.clear();
}

void VehicleCustomization::InitializeClassProfiles()
{
	// Tuner Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_TUNER;
		profile.steerSensitivity = 1.2f;
		profile.gripBonus = 0.05f;
		profile.weightMult = 0.9f;
		profile.powerMult = 0.95f;
		profile.brakeMult = 1.05f;
		profile.cameraDistance = 0.9f;
		profile.cameraHeight = 0.95f;
		profile.fovModifier = 5.0f;
		profile.enginePitchMod = 1.15f;
		profile.engineSoundType = "import";
		profile.aiAggression = 0.6f;
		profile.aiSkill = 0.7f;
		
		classProfiles.push_back(profile);
	}
	
	// Muscle Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_MUSCLE;
		profile.steerSensitivity = 0.85f;
		profile.gripBonus = 0.0f;
		profile.weightMult = 1.2f;
		profile.powerMult = 1.3f;
		profile.brakeMult = 0.9f;
		profile.cameraDistance = 1.1f;
		profile.cameraHeight = 1.0f;
		profile.fovModifier = -3.0f;
		profile.enginePitchMod = 0.85f;
		profile.engineSoundType = "v8";
		profile.aiAggression = 0.8f;
		profile.aiSkill = 0.5f;
		
		classProfiles.push_back(profile);
	}
	
	// Exotic Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_EXOTIC;
		profile.steerSensitivity = 1.0f;
		profile.gripBonus = 0.1f;
		profile.weightMult = 0.85f;
		profile.powerMult = 1.15f;
		profile.brakeMult = 1.15f;
		profile.cameraDistance = 1.0f;
		profile.cameraHeight = 0.9f;
		profile.fovModifier = 0.0f;
		profile.enginePitchMod = 1.1f;
		profile.engineSoundType = "v10";
		profile.aiAggression = 0.7f;
		profile.aiSkill = 0.8f;
		
		classProfiles.push_back(profile);
	}
	
	// Sport Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_SPORT;
		profile.steerSensitivity = 1.0f;
		profile.gripBonus = 0.05f;
		profile.weightMult = 0.95f;
		profile.powerMult = 1.05f;
		profile.brakeMult = 1.05f;
		profile.cameraDistance = 1.0f;
		profile.cameraHeight = 1.0f;
		profile.fovModifier = 0.0f;
		profile.enginePitchMod = 1.0f;
		profile.engineSoundType = "sport";
		profile.aiAggression = 0.5f;
		profile.aiSkill = 0.6f;
		
		classProfiles.push_back(profile);
	}
	
	// Luxury Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_LUXURY;
		profile.steerSensitivity = 0.9f;
		profile.gripBonus = 0.0f;
		profile.weightMult = 1.1f;
		profile.powerMult = 1.0f;
		profile.brakeMult = 1.0f;
		profile.cameraDistance = 1.05f;
		profile.cameraHeight = 1.05f;
		profile.fovModifier = -2.0f;
		profile.enginePitchMod = 0.95f;
		profile.engineSoundType = "smooth";
		profile.aiAggression = 0.3f;
		profile.aiSkill = 0.5f;
		
		classProfiles.push_back(profile);
	}
	
	// Offroad Class
	{
		VehicleClassProfile profile;
		profile.type = CLASS_OFFROAD;
		profile.steerSensitivity = 0.95f;
		profile.gripBonus = 0.15f;
		profile.weightMult = 1.15f;
		profile.powerMult = 0.95f;
		profile.brakeMult = 0.95f;
		profile.cameraDistance = 1.15f;
		profile.cameraHeight = 1.2f;
		profile.fovModifier = 3.0f;
		profile.enginePitchMod = 0.9f;
		profile.engineSoundType = "truck";
		profile.aiAggression = 0.4f;
		profile.aiSkill = 0.6f;
		
		classProfiles.push_back(profile);
	}
}

void VehicleCustomization::InitializeDefaultPaintJobs()
{
	// Default paint jobs
	PaintJob job;
	
	// Solid colors
	job.id = "solid_white";
	job.name = "Arctic White";
	job.primary = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
	job.isMetallic = false;
	job.cost = 0;
	paintJobs.push_back(job);
	
	job.id = "solid_black";
	job.name = "Phantom Black";
	job.primary = Ogre::ColourValue(0.05f, 0.05f, 0.05f, 1.0f);
	paintJobs.push_back(job);
	
	job.id = "solid_red";
	job.name = "Victory Red";
	job.primary = Ogre::ColourValue(0.8f, 0.1f, 0.1f, 1.0f);
	paintJobs.push_back(job);
	
	// Metallic
	job.id = "metallic_blue";
	job.name = "Laser Blue";
	job.primary = Ogre::ColourValue(0.1f, 0.3f, 0.8f, 1.0f);
	job.isMetallic = true;
	job.metallicFlake = 0.5f;
	job.cost = 500;
	paintJobs.push_back(job);
	
	job.id = "metallic_silver";
	job.name = "Sterling Silver";
	job.primary = Ogre::ColourValue(0.75f, 0.75f, 0.75f, 1.0f);
	job.isMetallic = true;
	job.metallicFlake = 0.7f;
	job.cost = 500;
	paintJobs.push_back(job);
	
	// Pearl
	job.id = "pearl_white";
	job.name = "Pearl White";
	job.primary = Ogre::ColourValue(0.95f, 0.95f, 0.98f, 1.0f);
	job.isPearl = true;
	job.cost = 1000;
	paintJobs.push_back(job);
	
	// Matte
	job.id = "matte_black";
	job.name = "Matte Black";
	job.primary = Ogre::ColourValue(0.1f, 0.1f, 0.1f, 1.0f);
	job.isMatte = true;
	job.cost = 1500;
	paintJobs.push_back(job);
}

void VehicleCustomization::InitializeDefaultVinyls()
{
	VinylDecal vinyl;
	
	// Stripes
	vinyl.id = "stripe_racing_center";
	vinyl.name = "Center Racing Stripe";
	vinyl.type = VinylDecal::VINYL_STRIPE;
	vinyl.width = 0.2f;
	vinyl.height = 3.0f;
	vinyl.posX = 0.0f;
	vinyl.posY = 0.0f;
	vinyl.rotation = 0.0f;
	vinyl.cost = 200;
	vinyls.push_back(vinyl);
	
	vinyl.id = "stripe_dual";
	vinyl.name = "Dual Stripes";
	vinyl.type = VinylDecal::VINYL_STRIPE;
	vinyl.width = 0.15f;
	vinyl.height = 3.0f;
	vinyl.posX = 0.0f;
	vinyl.posY = 0.0f;
	vinyl.mirrorX = true;
	vinyl.cost = 300;
	vinyls.push_back(vinyl);
	
	// Flames
	vinyl.id = "flame_side_left";
	vinyl.name = "Side Flame (Left)";
	vinyl.type = VinylDecal::VINYL_FLAME;
	vinyl.width = 0.5f;
	vinyl.height = 0.8f;
	vinyl.posX = -0.5f;
	vinyl.posY = 0.3f;
	vinyl.rotation = -30.0f;
	vinyl.cost = 500;
	vinyls.push_back(vinyl);
	
	// Tribal
	vinyl.id = "tribal_hood";
	vinyl.name = "Hood Tribal";
	vinyl.type = VinylDecal::VINYL_TRIBAL;
	vinyl.width = 0.6f;
	vinyl.height = 0.4f;
	vinyl.posX = 0.0f;
	vinyl.posY = 0.6f;
	vinyl.cost = 400;
	vinyls.push_back(vinyl);
	
	// Numbers
	vinyl.id = "number_01";
	vinyl.name = "Racing Number 01";
	vinyl.type = VinylDecal::VINYL_NUMBER;
	vinyl.width = 0.3f;
	vinyl.height = 0.4f;
	vinyl.posX = 0.0f;
	vinyl.posY = 0.0f;
	vinyl.cost = 150;
	vinyls.push_back(vinyl);
}

void VehicleCustomization::InitializeDefaultUpgrades()
{
	PerformanceUpgrade upgrade;
	
	// Engine upgrades
	upgrade.id = "engine_stage1";
	upgrade.name = "Stage 1 Engine Tune";
	upgrade.type = PerformanceUpgrade::UPGRADE_ENGINE;
	upgrade.level = 1;
	upgrade.powerBonus = 0.1f;
	upgrade.torqueBonus = 0.08f;
	upgrade.cost = 2000;
	upgrade.changesAppearance = false;
	upgrades.push_back(upgrade);
	
	upgrade.id = "engine_stage2";
	upgrade.name = "Stage 2 Engine Tune";
	upgrade.type = PerformanceUpgrade::UPGRADE_ENGINE;
	upgrade.level = 2;
	upgrade.powerBonus = 0.2f;
	upgrade.torqueBonus = 0.15f;
	upgrade.cost = 4000;
	upgrade.changesAppearance = true;
	upgrade.visualMesh = "engine_intake";
	upgrades.push_back(upgrade);
	
	upgrade.id = "engine_stage3";
	upgrade.name = "Stage 3 Engine Tune";
	upgrade.type = PerformanceUpgrade::UPGRADE_ENGINE;
	upgrade.level = 3;
	upgrade.powerBonus = 0.3f;
	upgrade.torqueBonus = 0.25f;
	upgrade.cost = 8000;
	upgrade.changesAppearance = true;
	upgrade.visualMesh = "engine_turbo";
	upgrades.push_back(upgrade);
	
	// Nitrous
	upgrade.id = "nitrous_50shot";
	upgrade.name = "50 Shot Nitrous";
	upgrade.type = PerformanceUpgrade::UPGRADE_NITROUS;
	upgrade.level = 1;
	upgrade.powerBonus = 0.15f;
	upgrade.cost = 1500;
	upgrade.changesAppearance = true;
	upgrade.visualMesh = "nitrous_bottle";
	upgrades.push_back(upgrade);
	
	// Tires
	upgrade.id = "tires_sport";
	upgrade.name = "Sport Tires";
	upgrade.type = PerformanceUpgrade::UPGRADE_TIRES;
	upgrade.level = 1;
	upgrade.gripBonus = 0.05f;
	upgrade.cost = 1000;
	upgrades.push_back(upgrade);
	
	upgrade.id = "tires_race";
	upgrade.name = "Race Tires";
	upgrade.type = PerformanceUpgrade::UPGRADE_TIRES;
	upgrade.level = 2;
	upgrade.gripBonus = 0.1f;
	upgrade.cost = 2500;
	upgrades.push_back(upgrade);
	
	// Brakes
	upgrade.id = "brakes_sport";
	upgrade.name = "Sport Brakes";
	upgrade.type = PerformanceUpgrade::UPGRADE_BRAKES;
	upgrade.level = 1;
	upgrade.brakeBonus = 0.1f;
	upgrade.cost = 1200;
	upgrades.push_back(upgrade);
	
	// Weight reduction
	upgrade.id = "weight_stage1";
	upgrade.name = "Weight Reduction Stage 1";
	upgrade.type = PerformanceUpgrade::UPGRADE_WEIGHT;
	upgrade.level = 1;
	upgrade.weightReduction = 0.05f;
	upgrade.cost = 3000;
	upgrades.push_back(upgrade);
}

const VehicleClassProfile& VehicleCustomization::GetClassProfile(VehicleClass type) const
{
	for (const auto& profile : classProfiles)
	{
		if (profile.type == type)
			return profile;
	}
	
	// Return default (tuner) if not found
	return classProfiles[0];
}

void VehicleCustomization::ApplyClassEffects(VehicleClass type, CAR* car) const
{
	if (!car) return;
	
	const VehicleClassProfile& profile = GetClassProfile(type);
	
	// Apply to arcade handling if enabled
	if (car->dynamics.arcadeAssistsEnabled)
	{
		ArcadeAssistParams& params = car->dynamics.arcadeHandling.GetParams();
		profile.ApplyToArcadeAssists(params);
	}
	
	// TODO: Apply power/weight/steering multipliers to car physics
	// These would need to be integrated with the car physics update
	// For now, the arcade handling profile modification handles most effects
}

const PaintJob* VehicleCustomization::GetPaintJob(int id) const
{
	if (id < 0 || id >= (int)paintJobs.size())
		return nullptr;
	return &paintJobs[id];
}

std::vector<const PaintJob*> VehicleCustomization::GetAvailablePaintJobs(int playerLevel) const
{
	std::vector<const PaintJob*> available;
	
	for (const auto& job : paintJobs)
	{
		if (playerLevel >= job.requiredLevel)
		{
			available.push_back(&job);
		}
	}
	
	return available;
}

void VehicleCustomization::ApplyPaintJob(int carId, int paintJobId)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	build.paintJobId = paintJobId;
	
	// TODO: Apply visual changes to car model
}

const VinylDecal* VehicleCustomization::GetVinyl(int id) const
{
	if (id < 0 || id >= (int)vinyls.size())
		return nullptr;
	return &vinyls[id];
}

std::vector<const VinylDecal*> VehicleCustomization::GetAvailableVinyls(int playerLevel) const
{
	std::vector<const VinylDecal*> available;
	
	for (const auto& vinyl : vinyls)
	{
		if (playerLevel >= vinyl.requiredLevel)
		{
			available.push_back(&vinyl);
		}
	}
	
	return available;
}

void VehicleCustomization::AddVinyl(int carId, const VinylDecal& decal)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	build.decals.push_back(decal);
	
	// TODO: Apply visual changes
}

void VehicleCustomization::RemoveVinyl(int carId, int index)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	
	if (index >= 0 && index < (int)build.decals.size())
	{
		build.decals.erase(build.decals.begin() + index);
	}
}

void VehicleCustomization::MoveVinylLayer(int carId, int index, int newLayer)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	
	if (index >= 0 && index < (int)build.decals.size())
	{
		build.decals[index].layer = newLayer;
		
		// Sort decals by layer
		std::sort(build.decals.begin(), build.decals.end(),
			[](const VinylDecal& a, const VinylDecal& b) {
				return a.layer < b.layer;
			});
	}
}

const VehicleLivery* VehicleCustomization::GetLivery(int id) const
{
	if (id < 0 || id >= (int)liveries.size())
		return nullptr;
	return &liveries[id];
}

std::vector<const VehicleLivery*> VehicleCustomization::GetAllLiveries() const
{
	std::vector<const VehicleLivery*> all;
	
	for (const auto& livery : liveries)
	{
		all.push_back(&livery);
	}
	
	return all;
}

void VehicleCustomization::ApplyLivery(int carId, int liveryId)
{
	const VehicleLivery* livery = GetLivery(liveryId);
	if (!livery) return;
	
	VehicleBuild& build = GetVehicleBuild(carId);
	build.paintJobId = livery->paintJobId;
	build.decals = livery->decals;
	
	// TODO: Apply visual changes
}

void VehicleCustomization::SaveLivery(int carId, const std::string& name)
{
	const VehicleBuild& build = GetVehicleBuild(carId);
	
	VehicleLivery livery;
	livery.id = "livery_" + name;
	livery.name = name;
	livery.paintJobId = build.paintJobId;
	livery.decals = build.decals;
	livery.isPreset = false;
	
	liveries.push_back(livery);
}

const PerformanceUpgrade* VehicleCustomization::GetUpgrade(const std::string& id) const
{
	for (const auto& upgrade : upgrades)
	{
		if (upgrade.id == id)
			return &upgrade;
	}
	return nullptr;
}

std::vector<const PerformanceUpgrade*> VehicleCustomization::GetUpgradesByType(
	PerformanceUpgrade::UpgradeType type) const
{
	std::vector<const PerformanceUpgrade*> typed;
	
	for (const auto& upgrade : upgrades)
	{
		if (upgrade.type == type)
		{
			typed.push_back(&upgrade);
		}
	}
	
	return typed;
}

void VehicleCustomization::InstallUpgrade(int carId, const std::string& upgradeId)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	
	// Check if upgrade exists
	const PerformanceUpgrade* upgrade = GetUpgrade(upgradeId);
	if (!upgrade) return;
	
	// Install or upgrade
	auto it = build.upgrades.find(upgradeId);
	if (it != build.upgrades.end())
	{
		// Already installed, upgrade level
		if (it->second < 3)
			it->second++;
	}
	else
	{
		// New upgrade
		build.upgrades[upgradeId] = 1;
	}
	
	// Recalculate stats
	CalculateVehicleStats(carId);
}

void VehicleCustomization::RemoveUpgrade(int carId, const std::string& upgradeId)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	
	auto it = build.upgrades.find(upgradeId);
	if (it != build.upgrades.end())
	{
		build.upgrades.erase(it);
		CalculateVehicleStats(carId);
	}
}

VehicleBuild& VehicleCustomization::GetVehicleBuild(int carId)
{
	return vehicleBuilds[carId];
}

const VehicleBuild& VehicleCustomization::GetVehicleBuild(int carId) const
{
	auto it = vehicleBuilds.find(carId);
	if (it != vehicleBuilds.end())
		return it->second;
	
	// Return default build
	static VehicleBuild defaultBuild;
	return defaultBuild;
}

void VehicleCustomization::SaveBuild(int carId, const std::string& buildName)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	build.buildName = buildName;
	
	// TODO: Save to file/database
}

void VehicleCustomization::LoadBuild(int carId, const std::string& buildName)
{
	// TODO: Load from file/database
	VehicleBuild& build = GetVehicleBuild(carId);
	build.buildName = buildName;
}

void VehicleCustomization::CalculateVehicleStats(int carId)
{
	VehicleBuild& build = GetVehicleBuild(carId);
	build.CalculateStats();
}

float VehicleCustomization::GetClassBonus(VehicleClass type, const std::string& stat) const
{
	const VehicleClassProfile& profile = GetClassProfile(type);
	
	if (stat == "power")
		return profile.powerMult;
	else if (stat == "handling")
		return 1.0f + profile.gripBonus;
	else if (stat == "acceleration")
		return profile.powerMult / profile.weightMult;
	else if (stat == "top_speed")
		return profile.powerMult;
	else if (stat == "braking")
		return profile.brakeMult;
	else if (stat == "steering")
		return profile.steerSensitivity;
	
	return 1.0f;
}
