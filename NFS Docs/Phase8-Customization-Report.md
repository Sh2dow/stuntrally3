# SR3 Phase 8: Vehicle Identity and Customization - Implementation Report

## Overview

Phase 8 implements a Carbon-style vehicle customization system with vehicle class profiles (Tuner/Muscle/Exotic), visual customization (paint, vinyls), performance upgrades, and class-specific handling characteristics.

## Files Created

| File | Lines | Description |
|------|-------|-------------|
| `src/game/VehicleCustomization.h` | 264 | Customization system declarations |
| `src/game/VehicleCustomization.cpp` | 731 | Full customization implementation |

---

## Vehicle Class System

### VehicleClass Enum

```cpp
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
```

### VehicleClassProfile Structure

```cpp
struct VehicleClassProfile
{
    VehicleClass type;
    
    // Handling characteristics
    float steerSensitivity;     // Steering response
    float gripBonus;            // Extra grip
    float weightMult;           // Weight multiplier
    float powerMult;            // Power multiplier
    float brakeMult;            // Braking multiplier
    
    // Presentation
    float cameraDistance;       // Camera distance multiplier
    float cameraHeight;         // Camera height multiplier
    float fovModifier;          // FOV adjustment
    
    // Audio
    float enginePitchMod;       // Engine pitch modifier
    std::string engineSoundType; // Sound type
    
    // AI behavior
    float aiAggression;         // AI aggression level
    float aiSkill;              // AI skill level
    
    void ApplyToArcadeAssists(ArcadeAssistParams& params) const;
};
```

### Default Class Profiles

| Class | Steer | Grip | Weight | Power | Brakes | Camera | FOV | Engine |
|-------|-------|------|--------|-------|--------|--------|-----|--------|
| Tuner | 1.2x | +5% | 0.9x | 0.95x | 1.05x | 0.9x | +5° | import (1.15x) |
| Muscle | 0.85x | - | 1.2x | 1.3x | 0.9x | 1.1x | -3° | v8 (0.85x) |
| Exotic | 1.0x | +10% | 0.85x | 1.15x | 1.15x | 1.0x | 0° | v10 (1.1x) |
| Sport | 1.0x | +5% | 0.95x | 1.05x | 1.05x | 1.0x | 0° | sport (1.0x) |
| Luxury | 0.9x | - | 1.1x | 1.0x | 1.0x | 1.05x | -2° | smooth (0.95x) |
| Offroad | 0.95x | +15% | 1.15x | 0.95x | 0.95x | 1.15x | +3° | truck (0.9x) |

### Class Effects on Arcade Handling

```cpp
void VehicleClassProfile::ApplyToArcadeAssists(ArcadeAssistParams& params) const
{
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
            
        // ... other classes
    }
}
```

---

## Visual Customization

### PaintJob Structure

```cpp
struct PaintJob
{
    std::string id;
    std::string name;
    
    // Primary colors
    Ogre::ColourValue primary;
    Ogre::ColourValue secondary;
    
    // Special effects
    bool isMetallic;
    bool isPearl;
    bool isMatte;
    float metallicFlake;
    
    // Rims
    Ogre::ColourValue rimColor;
    int rimStyle;
    
    // Windows
    Ogre::ColourValue windowTint;
    
    // Unlock requirements
    int requiredLevel;
    int requiredRep;
    int cost;
};
```

### Default Paint Jobs

| ID | Name | Type | Cost | Unlock |
|----|------|------|------|--------|
| solid_white | Arctic White | Solid | $0 | Default |
| solid_black | Phantom Black | Solid | $0 | Default |
| solid_red | Victory Red | Solid | $0 | Default |
| metallic_blue | Laser Blue | Metallic | $500 | Level 2 |
| metallic_silver | Sterling Silver | Metallic | $500 | Level 2 |
| pearl_white | Pearl White | Pearl | $1000 | Level 4 |
| matte_black | Matte Black | Matte | $1500 | Level 6 |

### VinylDecal Structure

```cpp
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
    
    VinylType type;
    
    // Placement
    int layer;                // Layer order (0 = bottom)
    float posX;               // X position (-1 to 1)
    float posY;               // Y position (-1 to 1)
    float width;              // Width scale
    float height;             // Height scale
    float rotation;           // Rotation in degrees
    
    // Appearance
    Ogre::ColourValue color;
    float opacity;
    bool mirrorX;             // Mirror on X axis
    bool mirrorY;             // Mirror on Y axis
    
    // Unlock requirements
    int requiredLevel;
    int cost;
};
```

### Default Vinyl Decals

| ID | Name | Type | Cost | Unlock |
|----|------|------|------|--------|
| stripe_racing_center | Center Racing Stripe | Stripe | $200 | Default |
| stripe_dual | Dual Stripes | Stripe | $300 | Level 2 |
| flame_side_left | Side Flame (Left) | Flame | $500 | Level 3 |
| tribal_hood | Hood Tribal | Tribal | $400 | Level 3 |
| number_01 | Racing Number 01 | Number | $150 | Default |

### Vinyl Layering System

```cpp
// Add vinyl to car
VinylDecal stripe = *VehicleCustomization::Get().GetVinyl(0);
stripe.color = Ogre::ColourValue::Black;
stripe.posX = 0.0f;
stripe.width = 0.2f;
VehicleCustomization::Get().AddVinyl(carId, stripe);

// Move vinyl layer (for proper rendering order)
VehicleCustomization::Get().MoveVinylLayer(carId, vinylIndex, newLayer);

// Remove vinyl
VehicleCustomization::Get().RemoveVinyl(carId, vinylIndex);
```

---

## Performance Upgrades

### PerformanceUpgrade Structure

```cpp
struct PerformanceUpgrade
{
    std::string id;
    std::string name;
    
    enum UpgradeType
    {
        UPGRADE_ENGINE,       // Engine improvements
        UPGRADE_NITROUS,      // Nitrous system
        UPGRADE_TRANSMISSION, // Transmission
        UPGRADE_TIRES,        // Tires
        UPGRADE_BRAKES,       // Brakes
        UPGRADE_SUSPENSION,   // Suspension
        UPGRADE_WEIGHT,       // Weight reduction
        UPGRADE_AERO          // Aerodynamics
    };
    
    UpgradeType type;
    int level;                // 0-3 upgrade level
    
    // Performance bonuses
    float powerBonus;         // Power increase %
    float torqueBonus;        // Torque increase %
    float weightReduction;    // Weight reduction %
    float gripBonus;          // Grip increase %
    float brakeBonus;         // Braking increase %
    
    // Visual changes
    bool changesAppearance;
    std::string visualMesh;   // Mesh to swap
    
    // Requirements
    int requiredLevel;
    int cost;
    
    // Compatibility
    std::vector<VehicleClass> compatibleClasses;
};
```

### Default Performance Upgrades

| ID | Name | Type | Level | Bonus | Cost |
|----|------|------|-------|-------|------|
| engine_stage1 | Stage 1 Engine Tune | Engine | 1 | +10% power | $2000 |
| engine_stage2 | Stage 2 Engine Tune | Engine | 2 | +20% power | $4000 |
| engine_stage3 | Stage 3 Engine Tune | Engine | 3 | +30% power | $8000 |
| nitrous_50shot | 50 Shot Nitrous | Nitrous | 1 | +15% power | $1500 |
| tires_sport | Sport Tires | Tires | 1 | +5% grip | $1000 |
| tires_race | Race Tires | Tires | 2 | +10% grip | $2500 |
| brakes_sport | Sport Brakes | Brakes | 1 | +10% braking | $1200 |
| weight_stage1 | Weight Reduction | Weight | 1 | -5% weight | $3000 |

### Upgrade Installation

```cpp
// Install upgrade
VehicleCustomization::Get().InstallUpgrade(carId, "engine_stage2");

// Upgrade level increases if already installed
VehicleCustomization::Get().InstallUpgrade(carId, "engine_stage2");  // Now level 2

// Remove upgrade
VehicleCustomization::Get().RemoveUpgrade(carId, "engine_stage2");

// Get upgrades by type
auto engineUpgrades = VehicleCustomization::Get().GetUpgradesByType(
    PerformanceUpgrade::UPGRADE_ENGINE);
```

---

## Vehicle Build System

### VehicleBuild Structure

```cpp
struct VehicleBuild
{
    std::string carId;
    std::string buildName;
    
    // Visual
    int paintJobId;
    std::vector<VinylDecal> decals;
    
    // Performance
    std::map<std::string, int> upgrades;  // upgradeId -> level
    
    // Stats (calculated)
    float power;
    float handling;
    float acceleration;
    float topSpeed;
    
    // Calculate stats from upgrades
    void CalculateStats();
};
```

### Build Management

```cpp
// Get vehicle build
VehicleBuild& build = VehicleCustomization::Get().GetVehicleBuild(carId);

// Apply customization
VehicleCustomization::Get().ApplyPaintJob(carId, paintJobId);
VehicleCustomization::Get().AddVinyl(carId, vinylDecal);
VehicleCustomization::Get().InstallUpgrade(carId, "engine_stage3");

// Calculate stats
VehicleCustomization::Get().CalculateVehicleStats(carId);

// Save build
VehicleCustomization::Get().SaveBuild(carId, "Track Day Setup");

// Load build
VehicleCustomization::Get().LoadBuild(carId, "Track Day Setup");
```

---

## VehicleCustomization Manager

### Singleton Access

```cpp
class VehicleCustomization
{
public:
    static VehicleCustomization& Get();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Vehicle class profiles
    const VehicleClassProfile& GetClassProfile(VehicleClass type) const;
    void ApplyClassEffects(VehicleClass type, CAR* car) const;
    
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
    
    // Performance upgrades
    const PerformanceUpgrade* GetUpgrade(const std::string& id) const;
    std::vector<const PerformanceUpgrade*> GetUpgradesByType(UpgradeType type) const;
    void InstallUpgrade(int carId, const std::string& upgradeId);
    void RemoveUpgrade(int carId, const std::string& upgradeId);
    
    // Vehicle builds
    VehicleBuild& GetVehicleBuild(int carId);
    void SaveBuild(int carId, const std::string& buildName);
    void LoadBuild(int carId, const std::string& buildName);
    
    // Stats
    void CalculateVehicleStats(int carId);
    float GetClassBonus(VehicleClass type, const std::string& stat) const;
};
```

---

## Integration Guide

### 1. Initialize Customization System

```cpp
// In game initialization
VehicleCustomization::Get().Initialize();
```

### 2. Apply Class Effects to Car

```cpp
// Get car's class (would be set per car model)
VehicleClass carClass = CLASS_TUNER;

// Apply class effects to arcade handling
VehicleCustomization::Get().ApplyClassEffects(carClass, car);

// This modifies the arcade handling params:
// - Tuner: better drift, more responsive steering
// - Muscle: more downforce, more stability
// - Exotic: more grip, less body roll
```

### 3. Visual Customization

```cpp
// Apply paint job
VehicleCustomization::Get().ApplyPaintJob(carId, paintJobId);

// Add vinyl decal
VinylDecal stripe = *VehicleCustomization::Get().GetVinyl(0);
stripe.color = Ogre::ColourValue::Black;
stripe.posX = 0.0f;
stripe.width = 0.2f;
VehicleCustomization::Get().AddVinyl(carId, stripe);

// Save livery preset
VehicleCustomization::Get().SaveLivery(carId, "My Custom Ride");
```

### 4. Performance Upgrades

```cpp
// Install upgrades
VehicleCustomization::Get().InstallUpgrade(carId, "engine_stage3");
VehicleCustomization::Get().InstallUpgrade(carId, "nitrous_50shot");
VehicleCustomization::Get().InstallUpgrade(carId, "tires_race");

// Calculate stats
VehicleCustomization::Get().CalculateVehicleStats(carId);

// Get stats
const VehicleBuild& build = VehicleCustomization::Get().GetVehicleBuild(carId);
float power = build.power;
float handling = build.handling;
```

### 5. Class-Specific Bonuses

```cpp
// Get class bonus for specific stat
float powerBonus = VehicleCustomization::Get().GetClassBonus(
    CLASS_MUSCLE, "power");  // Returns 1.3 for muscle

float handlingBonus = VehicleCustomization::Get().GetClassBonus(
    CLASS_EXOTIC, "handling");  // Returns 1.1 for exotic
```

---

## Known Limitations

1. **Visual Changes Not Applied**
   - Paint jobs don't change car color
   - Vinyls don't render on car model
   - Upgrade visual meshes not swapped

2. **Performance Stats Not Integrated**
   - Upgrades don't affect actual car physics
   - Stats are calculated but not applied
   - Need integration with car physics update

3. **No GUI Integration**
   - No customization shop UI
   - No 3D preview of changes
   - No vinyl placement editor

4. **Limited Content**
   - Only 8 paint jobs
   - Only 5 vinyl decals
   - Only 8 performance upgrades
   - No wheel customization
   - No body kits

---

## Next Steps

### Immediate
- [ ] GUI for customization shop
- [ ] 3D car preview with real-time updates
- [ ] Vinyl placement editor (drag/drop/rotate)
- [ ] Apply paint colors to car model
- [ ] Render vinyls on car surface

### Future
- [ ] More paint jobs (50+)
- [ ] More vinyls (100+)
- [ ] Wheel customization
- [ ] Body kit visual upgrades
- [ ] Dyno testing for performance
- [ ] Car sharing/export liveries

---

## References

- `NFS Docs/Implementation Plan.md` - Original Phase 8 spec
- `src/game/VehicleCustomization.h/cpp` - Full implementation
