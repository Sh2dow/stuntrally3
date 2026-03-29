#include "pch.h"
#include "ArcadeHandling.h"
#include "configfile.h"
#include "settings.h"
#include <algorithm>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

//--------------------------------------------------------------------------------------------------------------------------
ArcadeHandling::ArcadeHandling()
{
	Reset();
}

//--------------------------------------------------------------------------------------------------------------------------
ArcadeHandling::~ArcadeHandling()
{
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::Reset()
{
	processedSteer = 0.0f;
	downforce = 0.0f;
	yawStabTorque = 0.0f;
	driftAssistTorque = 0.0f;
	nitroForce = 0.0f;
	frontGripMult = 1.0f;
	rearGripMult = 1.0f;
	airControlTorque = 0.0f;
	
	isDrifting = false;
	driftAngle = 0.0f;
	speedSteerMult = 1.0f;
	downforceCoeff = 0.0f;
	airTime = 0.0f;
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::Init(const ArcadeAssistParams& params, SETTINGS* pSet)
{
	this->params = params;
	this->pSet = pSet;
	Reset();
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::LoadParams(CONFIGFILE& cf)
{
	// Speed-sensitive steering
	cf.GetParam("steerSpeedFactor", params.steerSpeedFactor);
	cf.GetParam("steerSpeedKnee", params.steerSpeedKnee);
	cf.GetParam("steerSpeedMax", params.steerSpeedMax);
	cf.GetParam("steerSpeedMinMult", params.steerSpeedMinMult);

	// Downforce / high-speed grip
	cf.GetParam("downforceBase", params.downforceBase);
	cf.GetParam("downforceSpeedMult", params.downforceSpeedMult);
	cf.GetParam("downforceMax", params.downforceMax);
	cf.GetParam("gripSpeedMult", params.gripSpeedMult);
	cf.GetParam("gripSpeedKnee", params.gripSpeedKnee);

	// Yaw stabilization
	cf.GetParam("yawStabEnabled", params.yawStabEnabled);
	cf.GetParam("yawStabGain", params.yawStabGain);
	cf.GetParam("yawStabDamping", params.yawStabDamping);
	cf.GetParam("yawStabDeadzone", params.yawStabDeadzone);
	cf.GetParam("yawStabSpeedMin", params.yawStabSpeedMin);

	// Drift assist
	cf.GetParam("driftAssistEnabled", params.driftAssistEnabled);
	cf.GetParam("driftBrakeThreshold", params.driftBrakeThreshold);
	cf.GetParam("driftSteerMult", params.driftSteerMult);
	cf.GetParam("driftYawTarget", params.driftYawTarget);
	cf.GetParam("driftYawGain", params.driftYawGain);
	cf.GetParam("driftRearGripReduction", params.driftRearGripReduction);
	cf.GetParam("driftSpeedMin", params.driftSpeedMin);

	// Nitro boost
	cf.GetParam("nitroForceMult", params.nitroForceMult);
	cf.GetParam("nitroSteerReduction", params.nitroSteerReduction);
	cf.GetParam("nitroGripBoost", params.nitroGripBoost);
	cf.GetParam("nitroDuration", params.nitroDuration);

	// Weight transfer
	cf.GetParam("weightTransferEnabled", params.weightTransferEnabled);
	cf.GetParam("weightTransferLong", params.weightTransferLong);
	cf.GetParam("weightTransferLat", params.weightTransferLat);
	cf.GetParam("gripFrontBase", params.gripFrontBase);
	cf.GetParam("gripRearBase", params.gripRearBase);
	cf.GetParam("gripLoadSensitivity", params.gripLoadSensitivity);

	// Air control
	cf.GetParam("airControlEnabled", params.airControlEnabled);
	cf.GetParam("airSteerMult", params.airSteerMult);
	cf.GetParam("airYawTorque", params.airYawTorque);
	cf.GetParam("airPitchTorque", params.airPitchTorque);
	cf.GetParam("airRollDamping", params.airRollDamping);

	// Global
	cf.GetParam("assistStrength", params.assistStrength);
	cf.GetParam("debugOutput", params.debugOutput);

	// Also try section-based names [arcade-assists]
	cf.GetParam("arcade-assists.strength", params.assistStrength);
	cf.GetParam("arcade-assists.debugOutput", params.debugOutput);
	
	// Enable flag from section
	int enabled = 1;
	if (cf.GetParam("arcade-assists.enabled", enabled))
	{
		// This will be used by CARDYNAMICS to enable/disable
	}
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::SaveParams(CONFIGFILE& cf) const
{
	cf.SetParam("steerSpeedFactor", params.steerSpeedFactor);
	cf.SetParam("steerSpeedKnee", params.steerSpeedKnee);
	cf.SetParam("steerSpeedMax", params.steerSpeedMax);
	cf.SetParam("steerSpeedMinMult", params.steerSpeedMinMult);

	cf.SetParam("downforceBase", params.downforceBase);
	cf.SetParam("downforceSpeedMult", params.downforceSpeedMult);
	cf.SetParam("downforceMax", params.downforceMax);
	cf.SetParam("gripSpeedMult", params.gripSpeedMult);
	cf.SetParam("gripSpeedKnee", params.gripSpeedKnee);

	cf.SetParam("yawStabEnabled", params.yawStabEnabled);
	cf.SetParam("yawStabGain", params.yawStabGain);
	cf.SetParam("yawStabDamping", params.yawStabDamping);
	cf.SetParam("yawStabDeadzone", params.yawStabDeadzone);
	cf.SetParam("yawStabSpeedMin", params.yawStabSpeedMin);

	cf.SetParam("driftAssistEnabled", params.driftAssistEnabled);
	cf.SetParam("driftBrakeThreshold", params.driftBrakeThreshold);
	cf.SetParam("driftSteerMult", params.driftSteerMult);
	cf.SetParam("driftYawTarget", params.driftYawTarget);
	cf.SetParam("driftYawGain", params.driftYawGain);
	cf.SetParam("driftRearGripReduction", params.driftRearGripReduction);
	cf.SetParam("driftSpeedMin", params.driftSpeedMin);

	cf.SetParam("nitroForceMult", params.nitroForceMult);
	cf.SetParam("nitroSteerReduction", params.nitroSteerReduction);
	cf.SetParam("nitroGripBoost", params.nitroGripBoost);
	cf.SetParam("nitroDuration", params.nitroDuration);

	cf.SetParam("weightTransferEnabled", params.weightTransferEnabled);
	cf.SetParam("weightTransferLong", params.weightTransferLong);
	cf.SetParam("weightTransferLat", params.weightTransferLat);
	cf.SetParam("gripFrontBase", params.gripFrontBase);
	cf.SetParam("gripRearBase", params.gripRearBase);
	cf.SetParam("gripLoadSensitivity", params.gripLoadSensitivity);

	cf.SetParam("airControlEnabled", params.airControlEnabled);
	cf.SetParam("airSteerMult", params.airSteerMult);
	cf.SetParam("airYawTorque", params.airYawTorque);
	cf.SetParam("airPitchTorque", params.airPitchTorque);
	cf.SetParam("airRollDamping", params.airRollDamping);

	cf.SetParam("assistStrength", params.assistStrength);
	cf.SetParam("debugOutput", params.debugOutput);
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::Update(
	float dt,
	float speed,
	float steerInput,
	float brakeInput,
	float throttleInput,
	float yawRate,
	float slipAngle,
	bool isAirborne,
	float _airTime,
	const MATHVECTOR<float,3>& velocity,
	const MATHVECTOR<float,3>& angularVel,
	const QUATERNION<float>& orientation,
	float frontLoad,
	float rearLoad,
	float nitroActive
)
{
	// Apply global assist strength multiplier
	float strength = Clamp(params.assistStrength, 0.0f, 1.0f);

	// Track air time
	if (isAirborne)
		airTime += dt;
	else
		airTime = 0.0f;

	// 1. Speed-sensitive steering
	speedSteerMult = ProcessSpeedSensitiveSteering(steerInput, speed);
	processedSteer = steerInput * speedSteerMult;

	// Apply nitro steering reduction
	if (nitroActive > 0.5f)
	{
		float nitroSteerReduct = 1.0f - (params.nitroSteerReduction * strength);
		processedSteer *= nitroSteerReduct;
	}

	// 2. Downforce / high-speed grip
	downforce = CalculateDownforce(speed);
	downforceCoeff = downforce / (speed > 1.0f ? speed * speed : 1.0f);

	// 3. Yaw stabilization (not in air)
	if (!isAirborne)
		yawStabTorque = CalculateYawStabilization(yawRate, speed, slipAngle) * strength;
	else
		yawStabTorque = 0.0f;

	// 4. Drift assist
	driftAssistTorque = CalculateDriftAssist(brakeInput, processedSteer, yawRate, speed) * strength;

	// Detect drift state
	isDrifting = (brakeInput > params.driftBrakeThreshold * 0.5f && 
	              std::abs(slipAngle) > 0.1f && 
	              speed > params.driftSpeedMin * 0.5f);
	driftAngle = isDrifting ? slipAngle : 0.0f;

	// 5. Nitro force
	nitroForce = CalculateNitroForce(nitroActive, speed) * strength;

	// 6. Weight transfer and grip shaping
	if (!isAirborne)
		CalculateWeightTransfer(throttleInput, brakeInput, processedSteer, frontLoad, rearLoad);
	else
	{
		frontGripMult = 1.0f;
		rearGripMult = 1.0f;
	}

	// 7. Air control
	if (isAirborne)
		airControlTorque = CalculateAirControl(processedSteer, airTime, angularVel) * strength;
	else
		airControlTorque = 0.0f;

	// Debug output
	if (params.debugOutput && pSet)
	{
		// Debug printing would go here - integrated with existing car debug system
	}
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::ProcessSpeedSensitiveSteering(float steerInput, float speed)
{
	if (params.steerSpeedFactor < 0.01f)
		return 1.0f;

	float absSpeed = std::abs(speed);
	
	// No reduction below knee speed
	if (absSpeed < params.steerSpeedKnee)
		return 1.0f;

	// Linear interpolation between knee and max speed
	float t = (absSpeed - params.steerSpeedKnee) / 
			  (params.steerSpeedMax - params.steerSpeedKnee);
	t = Clamp(t, 0.0f, 1.0f);

	// SmoothStep for smoother transition
	t = SmoothStep(0.0f, 1.0f, t);

	// Calculate multiplier: ranges from 1.0 down to steerSpeedMinMult
	float mult = 1.0f - (1.0f - params.steerSpeedMinMult) * t;

	// Apply steerSpeedFactor for partial assistance
	mult = 1.0f - (1.0f - mult) * params.steerSpeedFactor;

	return Clamp(mult, params.steerSpeedMinMult, 1.0f);
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::CalculateDownforce(float speed)
{
	if (params.downforceSpeedMult < 0.01f && params.downforceBase < 0.01f)
		return 0.0f;

	float absSpeed = std::abs(speed);
	
	// Base downforce + speed-squared term
	float down = params.downforceBase + params.downforceSpeedMult * absSpeed * absSpeed;

	// Add grip boost at high speed
	if (absSpeed > params.gripSpeedKnee)
	{
		float gripBoost = params.gripSpeedMult * 
			SmoothStep(params.gripSpeedKnee, params.gripSpeedKnee * 2.0f, absSpeed);
		down *= (1.0f + gripBoost);
	}

	return Clamp(down, 0.0f, params.downforceMax);
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::CalculateYawStabilization(float yawRate, float speed, float slipAngle)
{
	if (params.yawStabEnabled < 0.01f)
		return 0.0f;

	// Only activate above minimum speed
	if (std::abs(speed) < params.yawStabSpeedMin)
		return 0.0f;

	// Deadzone for small yaw rates
	if (std::abs(yawRate) < params.yawStabDeadzone)
		return 0.0f;

	// Counter-yaw torque: oppose the current yaw rate
	float torque = -yawRate * params.yawStabGain;

	// Add damping term
	torque += -yawRate * params.yawStabDamping;

	// Limit based on slip angle (more slip = less intervention allowed)
	float slipLimit = 1.0f - Clamp(std::abs(slipAngle), 0.0f, 0.5f) * 2.0f;
	torque *= slipLimit;

	return torque;
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::CalculateDriftAssist(float brakeInput, float steerInput, float yawRate, float speed)
{
	if (params.driftAssistEnabled < 0.01f)
		return 0.0f;

	// Check if conditions are right for drift assist
	if (brakeInput < params.driftBrakeThreshold || speed < params.driftSpeedMin)
		return 0.0f;

	// Calculate target yaw rate based on steering and speed
	float targetYawRate = steerInput * params.driftYawTarget * (speed / 20.0f);
	targetYawRate = Clamp(targetYawRate, -params.driftYawTarget, params.driftYawTarget);

	// Calculate error between current and target yaw rate
	float yawError = targetYawRate - yawRate;

	// Apply corrective torque to maintain drift
	float torque = yawError * params.driftYawGain;

	// Add steering multiplier effect
	torque *= params.driftSteerMult;

	return torque;
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::CalculateNitroForce(float nitroActive, float speed)
{
	if (nitroActive < 0.5f || params.nitroForceMult < 0.01f)
		return 0.0f;

	// Base nitro force (to be applied as forward force)
	// This integrates with the existing boost system in CARDYNAMICS
	float force = params.nitroForceMult * 15.0f; // Base force multiplier

	// Reduce at very high speeds (diminishing returns)
	float maxSpeed = 80.0f; // m/s (~288 km/h)
	if (std::abs(speed) > maxSpeed * 0.7f)
	{
		float reduction = SmoothStep(maxSpeed * 0.7f, maxSpeed, std::abs(speed));
		force *= (1.0f - reduction * 0.5f);
	}

	return force;
}

//--------------------------------------------------------------------------------------------------------------------------
void ArcadeHandling::CalculateWeightTransfer(
	float throttleInput, 
	float brakeInput, 
	float steerInput, 
	float frontLoad, 
	float rearLoad
)
{
	if (params.weightTransferEnabled < 0.01f)
	{
		frontGripMult = params.gripFrontBase;
		rearGripMult = params.gripRearBase;
		return;
	}

	// Longitudinal weight transfer (acceleration/braking)
	float longitudinal = (throttleInput - brakeInput) * params.weightTransferLong;

	// Lateral weight transfer (cornering)
	float lateral = steerInput * params.weightTransferLat;

	// Front axle: loses load under acceleration, gains under braking
	float frontLoadChange = -longitudinal * 0.6f + lateral * 0.4f;
	
	// Rear axle: gains load under acceleration, loses under braking
	float rearLoadChange = longitudinal * 0.6f - lateral * 0.4f;

	// Apply load sensitivity to grip
	float frontGripChange = frontLoadChange * params.gripLoadSensitivity;
	float rearGripChange = rearLoadChange * params.gripLoadSensitivity;

	// Calculate final grip multipliers
	frontGripMult = Clamp(params.gripFrontBase + frontGripChange, 0.3f, 1.5f);
	rearGripMult = Clamp(params.gripRearBase + rearGripChange, 0.3f, 1.5f);

	// Apply drift rear grip reduction if drifting
	if (isDrifting)
		rearGripMult *= params.driftRearGripReduction;
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::CalculateAirControl(
	float steerInput, 
	float airTime, 
	const MATHVECTOR<float,3>& angularVel
)
{
	if (params.airControlEnabled < 0.01f)
		return 0.0f;

	// Effectiveness decreases over time in air (limited control authority)
	float timeDecay = 1.0f - Clamp(airTime, 0.0f, 3.0f) / 3.0f;
	
	// Base air steering effectiveness
	float steerEffect = params.airSteerMult * timeDecay;

	// Yaw torque from steering input (simulates using brakes/drivetrain in air)
	float yawTorque = steerInput * params.airYawTorque * steerEffect;

	// Counter existing roll/pitch rates
	float rollDamping = -angularVel[0] * params.airRollDamping;
	
	// Combine effects (yaw control is primary, damping is secondary)
	float totalTorque = yawTorque + rollDamping * 0.3f;

	return totalTorque;
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::Clamp(float value, float min, float max) const
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::MapRange(float value, float inMin, float inMax, float outMin, float outMax) const
{
	return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

//--------------------------------------------------------------------------------------------------------------------------
float ArcadeHandling::SmoothStep(float edge0, float edge1, float x) const
{
	float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
