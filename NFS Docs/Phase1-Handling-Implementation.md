# SR3 Phase 1: Handling Foundation - Implementation Summary

## Overview

This implementation adds a Carbon-style arcade handling layer on top of SR3's existing physics system. The system is designed to be:
- **Non-destructive**: Works alongside existing physics without modifying core systems
- **Configurable**: All parameters are tunable per-car via .car files
- **Optional**: Can be enabled/disabled per vehicle
- **Modular**: Each assist system can be tuned independently

## Files Created

### Core Implementation
1. **`src/vdrift/ArcadeHandling.h`** (462 lines)
   - `ArcadeAssistParams` struct with all tunable parameters
   - `ArcadeHandling` class declaration

2. **`src/vdrift/ArcadeHandling.cpp`** (466 lines)
   - Full implementation of all 7 assist systems
   - Parameter loading/saving
   - Debug output support

### Documentation & Presets
3. **`NFS Docs/ArcadeHandlingPreset.txt`**
   - Detailed parameter documentation with explanations
   - Recommended values for Carbon-style handling

4. **`NFS Docs/ArcadeHandlingPreset-car.txt`**
   - Copy-paste preset for .car files
   - Section-based format matching SR3's .car structure

## Files Modified

1. **`src/vdrift/cardynamics.h`**
   - Added `#include "ArcadeHandling.h"`
   - Added `arcadeHandling` member variable
   - Added `arcadeAssistsEnabled` flag
   - Added accessor methods

2. **`src/vdrift/cardynamics_load.cpp`**
   - Added `arcadeHandling.LoadParams(c)` call
   - Added enable/disable logic from .car file

3. **`src/vdrift/cardynamics_update.cpp`**
   - Added `#include "ArcadeHandling.h"`
   - Integrated arcade handling in `UpdateBody()`
   - Applied forces and torques from arcade systems

## Seven Assist Systems

### 1. Speed-Sensitive Steering
Reduces steering angle at high speeds for stability while maintaining low-speed responsiveness.

**Key Parameters:**
- `steerSpeedFactor` - Overall strength (0-1)
- `steerSpeedKnee` - Speed where reduction begins (m/s)
- `steerSpeedMinMult` - Minimum steering multiplier at high speed

### 2. Downforce / High-Speed Grip
Adds artificial downforce scaling with speed² for Carbon-style high-speed stability.

**Key Parameters:**
- `downforceSpeedMult` - Downforce per speed²
- `gripSpeedMult` - Additional grip at high speed
- `downforceMax` - Maximum downforce limit

### 3. Yaw Stabilization
Applies counter-yaw torque to prevent uncontrolled spins without eliminating all oversteer.

**Key Parameters:**
- `yawStabEnabled` - On/off switch
- `yawStabGain` - Correction strength
- `yawStabDeadzone` - Tolerance for small yaw rates

### 4. Drift Assist
Helps initiate and maintain controlled drifts when braking and steering.

**Key Parameters:**
- `driftAssistEnabled` - On/off switch
- `driftBrakeThreshold` - Brake pressure to activate
- `driftYawTarget` - Target yaw rate for drifts
- `driftRearGripReduction` - Rear grip reduction during drift

### 5. Nitro Boost Enhancement
Enhances existing boost system with additional forward force and effects.

**Key Parameters:**
- `nitroForceMult` - Force multiplier
- `nitroSteerReduction` - Steering reduction during boost
- `nitroGripBoost` - Temporary grip increase

### 6. Weight Transfer / Grip Shaping
Simulates dynamic load transfer affecting front/rear grip balance.

**Key Parameters:**
- `weightTransferLong` - Longitudinal transfer factor
- `weightTransferLat` - Lateral transfer factor
- `gripLoadSensitivity` - Grip change per load change

### 7. Air Control
Provides limited yaw/steering control while airborne for better game feel.

**Key Parameters:**
- `airControlEnabled` - On/off switch
- `airSteerMult` - Steering effectiveness in air
- `airYawTorque` - Available yaw torque

## How to Enable

### For a Specific Car

Add this section to the car's `.car` file (e.g., `data/carsim/normal/cars/R1.car`):

```ini
[ arcade-assists ]
enabled = 1
strength = 1.0

steerSpeedFactor = 1.0
steerSpeedKnee = 30.0
steerSpeedMax = 60.0
steerSpeedMinMult = 0.4

yawStabEnabled = 1.0
yawStabGain = 800.0

driftAssistEnabled = 1.0
driftBrakeThreshold = 0.7
```

### Quick Start (Copy-Paste)

Copy the entire `[ arcade-assists ]` section from `NFS Docs/ArcadeHandlingPreset-car.txt` and append it to any .car file.

## Debug Output

Enable debug text in-game by setting:
```
car_dbgtxtcnt = 1  (in settings or via console)
arcade-assists.debugOutput = 1  (in .car file)
```

This will display:
- Current steering multiplier
- Downforce being applied
- Yaw/drift torque values
- Drift state and angle
- Grip multipliers

## Tuning Guidelines

### For More Arcade Feel (Carbon-style)
- Increase `steerSpeedFactor` to 1.0
- Increase `downforceSpeedMult` to 0.02-0.03
- Increase `yawStabGain` to 800-1200
- Enable `driftAssistEnabled`

### For More Sim Feel (SR3 default)
- Set `strength = 0` to disable all assists
- Or set individual `*Enabled` parameters to 0

### For Drift-Focused Cars
- Increase `driftSteerMult` to 1.3-1.5
- Decrease `driftRearGripReduction` to 0.5-0.6
- Increase `driftYawGain` to 600-800

## Integration Points

The arcade handling system integrates at these points in the physics update:

1. **After aerodynamics** - Arcade forces are applied
2. **Before tire forces** - Grip multipliers affect tire calculations
3. **Each physics substep** - Runs at `dyn_iter` frequency (default 60Hz)

## Performance Impact

Minimal - the system adds approximately:
- 50-100 simple float operations per physics substep
- No memory allocations during runtime
- No additional collision checks

## Next Steps (Phase 2+)

After validating Phase 1 handling:
1. **Presentation Foundation** - Camera, HUD, post-processing
2. **Track Usage Foundation** - Track metadata, zones, barriers
3. **Event Mode Layer** - Sprint, circuit, drift, canyon duel
4. **Racing AI** - Racing lines, AI behavior
5. **Career/World Map** - Progression shell

## Testing Checklist

- [ ] Cars load without errors
- [ ] Arcade assists can be enabled/disabled
- [ ] Speed-sensitive steering is noticeable
- [ ] Yaw stabilization prevents spins
- [ ] Drift assist works with handbrake
- [ ] Nitro boost feels stronger
- [ ] No performance regression
- [ ] Debug output displays correctly

## Known Limitations

1. Air control doesn't track actual air time (always 0)
2. Weight transfer uses simplified load estimation
3. Drift detection is based on slip angle threshold only
4. No per-axle assist tuning (global only)

These can be enhanced in future iterations based on testing feedback.
