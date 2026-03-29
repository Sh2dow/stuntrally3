#pragma once
#include "dbl.h"
#include "mathvector.h"
#include "quaternion.h"

// Forward declarations
class SETTINGS;
class CONFIGFILE;

/// 🎮 Arcade Handling Assist Layer
/// 
/// This system provides Carbon-style arcade handling on top of SR3's existing physics.
/// All assists are tunable and can be enabled/disabled independently.
/// 
/// Systems included:
/// 1. Speed-sensitive steering
/// 2. Downforce / high-speed grip
/// 3. Yaw stabilization
/// 4. Brake-to-drift / drift assist
/// 5. Nitro boost enhancement
/// 6. Weight transfer / grip shaping
/// 7. Air control

struct ArcadeAssistParams
{
	// ========== 1. Speed-Sensitive Steering ==========
	// Reduces steering angle at high speeds for stability
	// while maintaining responsiveness at low speeds.
	float steerSpeedFactor = 1.0f;      // 0 = no reduction, 1 = full Carbon-style reduction
	float steerSpeedKnee = 30.0f;       // m/s - speed at which reduction begins
	float steerSpeedMax = 60.0f;        // m/s - speed at which max reduction is reached
	float steerSpeedMinMult = 0.4f;     // minimum steering multiplier at high speed

	// ========== 2. Downforce / High-Speed Grip ==========
	// Adds artificial downforce that scales with speed squared.
	float downforceBase = 0.0f;         // Base downforce coefficient (N per m/s²)
	float downforceSpeedMult = 0.02f;   // Downforce scaling with speed²
	float downforceMax = 3000.0f;       // Maximum downforce force (N)
	float gripSpeedMult = 0.15f;        // Additional tire grip at high speed (0-1)
	float gripSpeedKnee = 25.0f;        // Speed at which grip boost begins

	// ========== 3. Yaw Stabilization ==========
	// Applies counter-yaw torque to prevent uncontrolled spins.
	float yawStabEnabled = 1.0f;        // 0 = off, 1 = full
	float yawStabGain = 800.0f;         // Stabilization torque gain
	float yawStabDamping = 200.0f;      // Yaw rate damping
	float yawStabDeadzone = 0.1f;       // Deadzone for small yaw rates
	float yawStabSpeedMin = 10.0f;      // Minimum speed for activation

	// ========== 4. Brake-to-Drift / Drift Assist ==========
	// Helps initiate and maintain controlled drifts.
	float driftAssistEnabled = 1.0f;    // 0 = off, 1 = full
	float driftBrakeThreshold = 0.7f;   // Brake pressure to initiate drift assist
	float driftSteerMult = 1.3f;        // Steering multiplier during drift
	float driftYawTarget = 0.3f;        // Target yaw rate for sustained drift
	float driftYawGain = 500.0f;        // Gain for yaw rate control
	float driftRearGripReduction = 0.6f; // Rear grip reduction during drift (0-1)
	float driftSpeedMin = 15.0f;        // Minimum speed for drift assist

	// ========== 5. Nitro Boost Enhancement ==========
	// Enhances the existing boost system with Carbon-style forward force.
	float nitroForceMult = 1.0f;        // Multiplier for nitro forward force
	float nitroSteerReduction = 0.5f;   // Steering reduction during nitro (0-1)
	float nitroGripBoost = 0.3f;        // Temporary grip increase during nitro
	float nitroDuration = 0.0f;         // Optional: force duration override (0 = use default)

	// ========== 6. Weight Transfer / Front-Rear Grip Shaping ==========
	// Simulates dynamic weight transfer during acceleration/braking.
	float weightTransferEnabled = 1.0f; // 0 = off, 1 = full
	float weightTransferLong = 0.25f;   // Longitudinal weight transfer factor
	float weightTransferLat = 0.15f;    // Lateral weight transfer factor
	float gripFrontBase = 1.0f;         // Front axle grip multiplier
	float gripRearBase = 1.0f;          // Rear axle grip multiplier
	float gripLoadSensitivity = 0.3f;   // How much grip changes with load

	// ========== 7. Air Control ==========
	// Allows limited steering/yaw control while airborne.
	float airControlEnabled = 1.0f;     // 0 = off, 1 = full
	float airSteerMult = 0.3f;          // Steering effectiveness in air (0-1)
	float airYawTorque = 300.0f;        // Yaw torque available in air
	float airPitchTorque = 200.0f;      // Pitch torque available in air
	float airRollDamping = 50.0f;       // Roll damping in air

	// ========== Global Settings ==========
	float assistStrength = 1.0f;        // Global multiplier for all assists (0-1)
	bool debugOutput = false;           // Enable debug printing
};


class ArcadeHandling
{
public:
	ArcadeHandling();
	~ArcadeHandling();

	// Initialize with parameters
	void Init(const ArcadeAssistParams& params, SETTINGS* pSet);
	void LoadParams(CONFIGFILE& cf);
	void SaveParams(CONFIGFILE& cf) const;

	// Main update - called each physics step
	// Returns modified steering value and additional forces/torques to apply
	void Update(
		float dt,
		float speed,                    // Current speed (m/s)
		float steerInput,               // Raw steering input (-1 to 1)
		float brakeInput,               // Brake pressure (0 to 1)
		float throttleInput,            // Throttle input (0 to 1)
		float yawRate,                  // Current yaw rate (rad/s)
		float slipAngle,                // Current body slip angle (rad)
		bool isAirborne,                // True if car is in the air
		float airTime,                  // Time spent airborne (seconds)
		const MATHVECTOR<float,3>& velocity,   // World-space velocity
		const MATHVECTOR<float,3>& angularVel, // World-space angular velocity
		const QUATERNION<float>& orientation,  // Car orientation
		float frontLoad,                // Normalized front axle load (0-1)
		float rearLoad,                 // Normalized rear axle load (0-1)
		float nitroActive               // Nitro boost active (0 or 1)
	);

	// Get processed outputs
	float GetProcessedSteering() const { return processedSteer; }
	float GetDownforce() const { return downforce; }
	float GetYawStabTorque() const { return yawStabTorque; }
	float GetDriftAssistTorque() const { return driftAssistTorque; }
	float GetNitroForce() const { return nitroForce; }
	float GetFrontGripMult() const { return frontGripMult; }
	float GetRearGripMult() const { return rearGripMult; }
	float GetAirControlTorque() const { return airControlTorque; }
	
	// Get params for modification
	ArcadeAssistParams& GetParams() { return params; }
	const ArcadeAssistParams& GetParams() const { return params; }

	// State queries
	bool IsDrifting() const { return isDrifting; }
	float GetDriftAngle() const { return driftAngle; }
	float GetSpeedSteerMult() const { return speedSteerMult; }
	float GetDownforceCoeff() const { return downforceCoeff; }

	// Reset transient state (called on car reset)
	void Reset();

private:
	ArcadeAssistParams params;
	SETTINGS* pSet = nullptr;

	// Processed outputs
	float processedSteer = 0.0f;
	float downforce = 0.0f;
	float yawStabTorque = 0.0f;
	float driftAssistTorque = 0.0f;
	float nitroForce = 0.0f;
	float frontGripMult = 1.0f;
	float rearGripMult = 1.0f;
	float airControlTorque = 0.0f;

	// State tracking
	bool isDrifting = false;
	float driftAngle = 0.0f;
	float speedSteerMult = 1.0f;
	float downforceCoeff = 0.0f;
	float airTime = 0.0f;

	// Internal processing functions
	float ProcessSpeedSensitiveSteering(float steerInput, float speed);
	float CalculateDownforce(float speed);
	float CalculateYawStabilization(float yawRate, float speed, float slipAngle);
	float CalculateDriftAssist(float brakeInput, float steerInput, float yawRate, float speed);
	float CalculateNitroForce(float nitroActive, float speed);
	void CalculateWeightTransfer(float throttleInput, float brakeInput, float steerInput, float frontLoad, float rearLoad);
	float CalculateAirControl(float steerInput, float airTime, const MATHVECTOR<float,3>& angularVel);

	// Helper functions
	float Clamp(float value, float min, float max) const;
	float MapRange(float value, float inMin, float inMax, float outMin, float outMax) const;
	float SmoothStep(float edge0, float edge1, float x) const;
};
