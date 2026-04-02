#include "pch.h"
#include "EventMode.h"
#include "Pursuit.h"
#include "game.h"
#include "car.h"
#include "tinyxml2.h"
#include "Def_Str.h"
#include "paths.h"
#include <OgreStringConverter.h>
#include <filesystem>

using namespace tinyxml2;
namespace fs = std::filesystem;


//--------------------------------------------------------------------------------------------------------------------------
// EventResults
//--------------------------------------------------------------------------------------------------------------------------

bool EventResults::isWin() const
{
	switch (position)
	{
		case 1: return true;  // First place is always a win
		case 2: case 3: return points > 0;  // Podium with points
		default: return false;
	}
}

std::string EventResults::GetResultString() const
{
	char buf[256];
	
	if (position == 1)
		snprintf(buf, sizeof(buf), "1st Place - Time: %.2fs", time);
	else if (position == 2)
		snprintf(buf, sizeof(buf), "2nd Place - Time: %.2fs", time);
	else if (position == 3)
		snprintf(buf, sizeof(buf), "3rd Place - Time: %.2fs", time);
	else
		snprintf(buf, sizeof(buf), "%dth Place - Time: %.2fs", position, time);
	
	return std::string(buf);
}


//--------------------------------------------------------------------------------------------------------------------------
// EventConfig
//--------------------------------------------------------------------------------------------------------------------------

bool EventConfig::LoadFromXml(const std::string& file)
{
	XMLDocument doc;
	XMLError er = doc.LoadFile(file.c_str());
	if (er != XML_SUCCESS) return false;
	
	XMLElement* root = doc.RootElement();
	if (!root) return false;
	
	const char* a;
	
	// Core
	a = root->Attribute("type");
	if (a)
	{
		std::string typeName = std::string(a);
		for (int i = 0; i < EVENT_COUNT; ++i)
		{
			if (EventTypeNames[i] == typeName)
			{
				type = (EventType)i;
				break;
			}
		}
	}
	
	a = root->Attribute("id");        if (a) id = std::string(a);
	a = root->Attribute("name");      if (a) name = std::string(a);
	a = root->Attribute("desc");      if (a) description = std::string(a);
	a = root->Attribute("track");     if (a) trackId = std::string(a);
	a = root->Attribute("reversed");  if (a) trackReversed = Ogre::StringConverter::parseBool(a);
	
	// Race params
	a = root->Attribute("laps");       if (a) laps = s2i(a);
	a = root->Attribute("timeLimit");  if (a) timeLimit = s2r(a);
	a = root->Attribute("difficulty"); if (a) difficulty = s2i(a);
	a = root->Attribute("rewardCash"); if (a) rewardCash = s2i(a);
	a = root->Attribute("rewardRep");  if (a) rewardRep = s2i(a);
	
	// Opponents
	XMLElement* opponents = root->FirstChildElement("opponents");
	if (opponents)
	{
		XMLElement* car = opponents->FirstChildElement("car");
		while (car)
		{
			a = car->GetText();
			if (a) opponentCars.push_back(std::string(a));
			car = car->NextSiblingElement("car");
		}
		opponentCount = (int)opponentCars.size();
	}
	
	// Event-specific
	a = root->Attribute("driftTarget");  if (a) driftTarget = s2r(a);
	a = root->Attribute("canyonTarget"); if (a) canyonTarget = s2r(a);
	a = root->Attribute("tollCount");    if (a) tollCount = s2i(a);
	a = root->Attribute("pursuitHeat");  if (a) pursuitHeat = s2i(a);
	
	// Requirements
	a = root->Attribute("requiredRep");  if (a) requiredRep = s2i(a);
	
	return true;
}

bool EventConfig::SaveToXml(const std::string& file) const
{
	XMLDocument xml;
	XMLElement* root = xml.NewElement("event");
	
	root->SetAttribute("type", EventTypeNames[type].c_str());
	root->SetAttribute("id", id.c_str());
	root->SetAttribute("name", name.c_str());
	root->SetAttribute("desc", description.c_str());
	root->SetAttribute("track", trackId.c_str());
	root->SetAttribute("reversed", trackReversed);
	
	root->SetAttribute("laps", laps);
	root->SetAttribute("timeLimit", fToStr(timeLimit).c_str());
	root->SetAttribute("difficulty", difficulty);
	root->SetAttribute("rewardCash", rewardCash);
	root->SetAttribute("rewardRep", rewardRep);
	
	// Opponents
	if (!opponentCars.empty())
	{
		XMLElement* oppElem = xml.NewElement("opponents");
		for (const auto& car : opponentCars)
		{
			XMLElement* carElem = xml.NewElement("car");
			carElem->SetText(car.c_str());
			oppElem->InsertEndChild(carElem);
		}
		root->InsertEndChild(oppElem);
	}
	
	// Event-specific
	if (driftTarget > 0) root->SetAttribute("driftTarget", fToStr(driftTarget).c_str());
	if (canyonTarget > 0) root->SetAttribute("canyonTarget", fToStr(canyonTarget).c_str());
	if (tollCount > 0) root->SetAttribute("tollCount", tollCount);
	if (pursuitHeat > 0) root->SetAttribute("pursuitHeat", pursuitHeat);
	
	root->SetAttribute("requiredRep", requiredRep);
	
	xml.InsertEndChild(root);
	return xml.SaveFile(file.c_str());
}


//--------------------------------------------------------------------------------------------------------------------------
// BaseEvent
//--------------------------------------------------------------------------------------------------------------------------

BaseEvent::BaseEvent(GAME* game) : pGame(game)
{
}

BaseEvent::~BaseEvent()
{
}

bool BaseEvent::Init(const EventConfig& cfg)
{
	config = cfg;
	state = EVENT_STATE_IDLE;
	raceTime = 0.0f;
	countdownTime = 0.0f;
	currentLap = 0;
	playerPosition = 1;
	lapTimes.clear();
	bestLapTime = 0.0f;
	
	return true;
}

void BaseEvent::Start()
{
	state = EVENT_STATE_COUNTDOWN;
	countdownTime = 3.0f;  // 3 second countdown
	
	// Position cars at start
	if (pGame && pGame->cars.size() > 0)
	{
		// TODO: Position player car at start line
		// TODO: Position opponent cars
	}
}

void BaseEvent::Update(float dt)
{
	switch (state)
	{
		case EVENT_STATE_COUNTDOWN:
			UpdateCountdown(dt);
			break;
			
		case EVENT_STATE_RACING:
			UpdateRace(dt);
			break;
			
		case EVENT_STATE_FINISHED:
		case EVENT_STATE_FAILED:
		case EVENT_STATE_CANCELLED:
			// Event ended, waiting for cleanup
			break;
			
		default:
			break;
	}
}

void BaseEvent::End()
{
	state = EVENT_STATE_FINISHED;
	
	// TODO: Show results screen
	// TODO: Award rewards
}

int BaseEvent::GetPlayerPosition() const
{
	return playerPosition;
}

int BaseEvent::GetTotalRacers() const
{
	if (!pGame) return 0;
	return (int)pGame->cars.size();
}

float BaseEvent::GetRaceTime() const
{
	return raceTime;
}

float BaseEvent::GetBestLap() const
{
	return bestLapTime;
}

EventResults BaseEvent::GetResults() const
{
	EventResults results;
	results.position = playerPosition;
	results.totalRacers = GetTotalRacers();
	results.time = raceTime;
	results.bestLap = bestLapTime;
	
	// Calculate points based on position
	if (playerPosition == 1) results.points = 10;
	else if (playerPosition == 2) results.points = 7;
	else if (playerPosition == 3) results.points = 5;
	else if (playerPosition <= 5) results.points = 3;
	else if (playerPosition <= 10) results.points = 1;
	
	return results;
}

void BaseEvent::OnPlayerFinish(int position)
{
	playerPosition = position;
	
	if (position == 1)
	{
		// Player won
		state = EVENT_STATE_FINISHED;
	}
	else
	{
		// Check if race is over for all opponents
		CheckFinishCondition();
	}
}

void BaseEvent::OnPlayerFail()
{
	state = EVENT_STATE_FAILED;
}

void BaseEvent::OnLapComplete(int lap, float lapTime)
{
	currentLap = lap;
	lapTimes.push_back(lapTime);
	
	if (bestLapTime == 0.0f || lapTime < bestLapTime)
		bestLapTime = lapTime;
}

void BaseEvent::OnCheckpoint(int index)
{
	// Checkpoint logic - override in derived classes
}

void BaseEvent::UpdateCountdown(float dt)
{
	countdownTime -= dt;
	
	if (countdownTime <= 0.0f)
	{
		state = EVENT_STATE_RACING;
		// TODO: Play GO! sound
	}
	else
	{
		// TODO: Show countdown number (3, 2, 1)
	}
}

void BaseEvent::UpdateRace(float dt)
{
	raceTime += dt;
	
	// Update player position
	playerPosition = CalculatePosition();
	
	// Check for race completion
	CheckFinishCondition();
	
	// Check time limit
	if (config.timeLimit > 0.0f && raceTime > config.timeLimit)
	{
		OnPlayerFail();
	}
}

void BaseEvent::CheckFinishCondition()
{
	if (IsRaceComplete())
	{
		End();
	}
}

bool BaseEvent::IsLapComplete() const
{
	// Override in derived classes
	return false;
}

bool BaseEvent::IsRaceComplete() const
{
	// Override in derived classes
	return state == EVENT_STATE_FINISHED || state == EVENT_STATE_FAILED;
}

int BaseEvent::CalculatePosition() const
{
	// Simple position calculation - override for proper race logic
	if (!pGame || pGame->cars.empty())
		return 1;
	
	// TODO: Calculate based on checkpoint progress
	return playerPosition;
}


//--------------------------------------------------------------------------------------------------------------------------
// SprintEvent
//--------------------------------------------------------------------------------------------------------------------------

SprintEvent::SprintEvent(GAME* game) : BaseEvent(game)
{
}

void SprintEvent::Start()
{
	BaseEvent::Start();
	
	// Get track start/end positions from metadata
	if (pGame)
	{
		// TODO: Get track metadata and extract start/end positions
		// TrackMetadata* track = TrackDatabase::Get().GetTrack(config.trackId);
		// if (track)
		// {
		//     startPos = track->engagePos;  // Already MATHVECTOR<float,3>
		//     // endPos would be last checkpoint
		// }
	}
}

void SprintEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		// Update player progress
		if (pGame && pGame->cars.size() > 0)
		{
			playerProgress = CalculateProgress(pGame->cars[0]);
		}
	}
}

float SprintEvent::CalculateProgress(CAR* car) const
{
	if (!car) return 0.0f;
	
	// Calculate progress along track (0.0 to 1.0)
	// TODO: Use checkpoint system
	MATHVECTOR<float,3> pos = car->GetPosition();
	
	// Simple distance-based progress (placeholder)
	if (trackLength > 0.0f)
	{
		float dist = std::sqrt(
			(pos[0]-startPos[0])*(pos[0]-startPos[0]) +
			(pos[1]-startPos[1])*(pos[1]-startPos[1]) +
			(pos[2]-startPos[2])*(pos[2]-startPos[2]));
		return std::min(1.0f, dist / trackLength);
	}
	
	return 0.0f;
}


//--------------------------------------------------------------------------------------------------------------------------
// CircuitEvent
//--------------------------------------------------------------------------------------------------------------------------

CircuitEvent::CircuitEvent(GAME* game) : BaseEvent(game)
{
}

bool CircuitEvent::Init(const EventConfig& cfg)
{
	BaseEvent::Init(cfg);
	totalLaps = cfg.laps;
	opponentLaps.resize(cfg.opponentCount, 0);
	return true;
}

void CircuitEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		CheckLapComplete();
	}
}

void CircuitEvent::CheckLapComplete()
{
	if (!pGame || pGame->cars.empty()) return;
	
	// Check if player completed a lap
	// TODO: Use checkpoint system to verify lap completion
	// if (IsLapComplete())
	// {
	//     currentLap++;
	//     OnLapComplete(currentLap, lapTime);
	//     
	//     if (currentLap >= totalLaps)
	//     {
	//         OnPlayerFinish(playerPosition);
	//     }
	// }
}


//--------------------------------------------------------------------------------------------------------------------------
// DriftEvent
//--------------------------------------------------------------------------------------------------------------------------

DriftEvent::DriftEvent(GAME* game) : BaseEvent(game)
{
}

void DriftEvent::Start()
{
	BaseEvent::Start();
	
	driftScore = 0.0f;
	driftCombo = 0.0f;
	currentDrift = 0.0f;
	isDrifting = false;
	
	// Count drift zones from track
	// TODO: Get drift zones from track metadata
	// TrackMetadata* track = TrackDatabase::Get().GetTrack(config.trackId);
	// if (track)
	// {
	//     driftZonesTotal = track->GetZonesByType(ZONE_DRIFT).size();
	// }
}

void DriftEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		UpdateDriftScore(dt);
		
		// Check if target score reached
		if (config.driftTarget > 0.0f && driftScore >= config.driftTarget)
		{
			OnPlayerFinish(1);
		}
		
		// Check time limit
		if (config.timeLimit > 0.0f && raceTime >= config.timeLimit)
		{
			if (driftScore >= config.driftTarget * 0.5f)  // At least 50% of target
				OnPlayerFinish(2);
			else
				OnPlayerFail();
		}
	}
}

float DriftEvent::GetDriftScore() const
{
	return driftScore;
}

float DriftEvent::GetDriftCombo() const
{
	return driftCombo;
}

EventResults DriftEvent::GetResults() const
{
	EventResults results = BaseEvent::GetResults();
	results.driftScore = driftScore;
	results.driftCombo = driftCombo;
	results.driftZones = driftZonesHit;
	
	// Bonus points for drift performance
	if (driftScore >= config.driftTarget)
		results.points += 5;  // Target bonus
	if (driftCombo > 2.0f)
		results.points += 3;  // Combo bonus
	
	return results;
}

void DriftEvent::UpdateDriftScore(float dt)
{
	if (!pGame || pGame->cars.empty()) return;
	
	CAR* car = pGame->cars[0];
	currentDrift = CalculateDriftAmount(car);
	
	if (currentDrift > 0.1f)  // Drift threshold
	{
		if (!isDrifting)
		{
			// Start new drift
			isDrifting = true;
			driftStartTime = raceTime;
			driftCombo = 1.0f;
		}
		
		// Calculate drift score
		float speed = car->GetSpeed();
		float angle = currentDrift;  // Radians
		
		// Score = speed * angle * combo
		float scoreGain = speed * angle * driftCombo * dt;
		driftScore += scoreGain;
		
		// Increase combo
		driftCombo += dt * 0.1f;  // Combo builds over time
		if (driftCombo > 5.0f) driftCombo = 5.0f;  // Max combo
	}
	else
	{
		if (isDrifting)
		{
			// End drift
			isDrifting = false;
			driftCombo = 0.0f;
		}
	}
}

float DriftEvent::CalculateDriftAmount(CAR* car) const
{
	if (!car) return 0.0f;
	
	// Calculate drift angle from velocity and orientation
	MATHVECTOR<float,3> vel = car->GetVelocity();
	
	// Get forward vector from car orientation
	QUATERNION<float> orient = car->GetOrientation();
	MATHVECTOR<float,3> forward(0, 0, -1);
	orient.RotateVector(forward);
	
	if (vel.Magnitude() < 0.1f) return 0.0f;
	
	// Normalize
	MATHVECTOR<float,3> velNorm = vel;
	velNorm.Normalize();
	MATHVECTOR<float,3> fwdNorm = forward;
	fwdNorm.Normalize();
	
	// Calculate angle using dot product
	float dot = velNorm[0]*fwdNorm[0] + velNorm[1]*fwdNorm[1] + velNorm[2]*fwdNorm[2];
	float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
	
	return angle;  // Radians
}

void DriftEvent::OnDriftZoneEnter(int zoneIndex)
{
	// Enter drift zone - score multiplier
	driftCombo *= 1.5f;
}

void DriftEvent::OnDriftZoneExit(int zoneIndex, float score)
{
	driftZonesHit++;
}


//--------------------------------------------------------------------------------------------------------------------------
// CanyonDuelEvent
//--------------------------------------------------------------------------------------------------------------------------

CanyonDuelEvent::CanyonDuelEvent(GAME* game) : BaseEvent(game)
{
}

void CanyonDuelEvent::Start()
{
	BaseEvent::Start();
	
	phase = PHASE_PLAYER_RUN;
	playerBestTime = 0.0f;
	rivalBestTime = 0.0f;
	currentTime = 0.0f;
	playerKOs = 0;
	rivalKOs = 0;
	
	StartPlayerRun();
}

void CanyonDuelEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		currentTime += dt;
		
		switch (phase)
		{
			case PHASE_PLAYER_RUN:
				// Check if player finished or crashed
				// TODO: Check player progress
				if (CheckKnockout())
				{
					playerKOs++;
					// Restart player run
					currentTime = 0.0f;
				}
				break;
				
			case PHASE_RIVAL_RUN:
				UpdateRival(dt);
				if (CheckKnockout())
				{
					rivalKOs++;
					// Restart rival run
					currentTime = 0.0f;
				}
				break;
				
			default:
				break;
		}
	}
}

float CanyonDuelEvent::GetTimeDelta() const
{
	if (phase == PHASE_PLAYER_RUN)
	{
		// Show time to beat
		return rivalBestTime > 0 ? rivalBestTime - currentTime : 0.0f;
	}
	else
	{
		// Show rival's deficit
		return currentTime - playerBestTime;
	}
}

EventResults CanyonDuelEvent::GetResults() const
{
	EventResults results = BaseEvent::GetResults();
	results.playerTime = playerBestTime;
	results.rivalTime = rivalBestTime;
	results.timeDiff = std::abs(playerBestTime - rivalBestTime);
	results.wonByKO = (playerKOs > rivalKOs);
	
	// Determine winner
	if (playerKOs > rivalKOs)
		results.position = 1;  // Won by KO
	else if (rivalBestTime > 0 && playerBestTime < rivalBestTime)
		results.position = 1;  // Won by time
	else
		results.position = 2;  // Lost
	
	return results;
}

void CanyonDuelEvent::StartPlayerRun()
{
	phase = PHASE_PLAYER_RUN;
	currentTime = 0.0f;
	
	// TODO: Reset player car to start position
	// TODO: Show "YOUR RUN" message
}

void CanyonDuelEvent::StartRivalRun()
{
	phase = PHASE_RIVAL_RUN;
	currentTime = 0.0f;
	
	// TODO: Spawn rival car at start
	// TODO: Show "RIVAL RUN" message
}

void CanyonDuelEvent::UpdateRival(float dt)
{
	// Simple rival AI - follows ideal line
	// TODO: Implement rival AI that follows racing line
	
	// Placeholder: rival completes run in target time
	float rivalTarget = config.canyonTarget > 0 ? config.canyonTarget : 60.0f;
	
	if (currentTime >= rivalTarget)
	{
		rivalBestTime = currentTime;
		
		// Determine winner
		if (playerBestTime > 0)
		{
			if (playerBestTime < rivalBestTime)
				OnPlayerFinish(1);
			else
				OnPlayerFinish(2);
		}
	}
}

bool CanyonDuelEvent::CheckKnockout()
{
	// Check if car crashed/stopped
	if (!pGame || pGame->cars.empty()) return false;
	
	CAR* car = pGame->cars[0];
	
	// Check if car is stopped (crashed)
	if (car->GetSpeed() < 0.1f)
	{
		// Car stopped - check if it's a knockout
		// TODO: Check if car is on track or off cliff
		return false;  // Placeholder
	}
	
	return false;
}


//--------------------------------------------------------------------------------------------------------------------------
// CanyonRunEvent
//--------------------------------------------------------------------------------------------------------------------------

CanyonRunEvent::CanyonRunEvent(GAME* game) : BaseEvent(game)
{
}

void CanyonRunEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		currentTime = raceTime;
		CheckTargetTime();
	}
}

void CanyonRunEvent::CheckTargetTime()
{
	if (config.canyonTarget <= 0) return;
	
	if (currentTime <= config.canyonTarget)
	{
		// Beat target time
		OnPlayerFinish(1);
	}
	else if (currentTime > config.canyonTarget * 1.5f)
	{
		// Too slow - fail
		OnPlayerFail();
	}
}


//--------------------------------------------------------------------------------------------------------------------------
// BossEvent
//--------------------------------------------------------------------------------------------------------------------------

BossEvent::BossEvent(GAME* game) : BaseEvent(game)
{
}

void BossEvent::Start()
{
	BaseEvent::Start();
	
	phase = PHASE_INTRO;
	phaseTime = 0.0f;
	bossDefeated = false;
	
	StartIntro();
}

void BossEvent::Update(float dt)
{
	switch (phase)
	{
		case PHASE_INTRO:
			phaseTime += dt;
			if (phaseTime >= 3.0f)  // 3 second intro
			{
				phase = PHASE_RACE;
				BaseEvent::Start();  // Start actual race
			}
			break;
			
		case PHASE_RACE:
			BaseEvent::Update(dt);
			
			if (state == EVENT_STATE_RACING)
			{
				UpdateBossAI(dt);
				
				// Check if boss is defeated
				if (CheckBossDefeated())
				{
					bossDefeated = true;
					phase = PHASE_OUTRO;
					StartOutro();
				}
			}
			else if (state == EVENT_STATE_FINISHED || state == EVENT_STATE_FAILED)
			{
				phase = PHASE_OUTRO;
				StartOutro();
			}
			break;
			
		case PHASE_OUTRO:
			phaseTime += dt;
			if (phaseTime >= 3.0f)  // 3 second outro
			{
				End();
			}
			break;
			
		default:
			break;
	}
}

EventResults BossEvent::GetResults() const
{
	EventResults results = BaseEvent::GetResults();
	
	if (bossDefeated)
	{
		results.position = 1;
		results.points *= 2;  // Double points for boss defeat
	}
	
	return results;
}

void BossEvent::StartIntro()
{
	// TODO: Show boss intro dialogue
	// TODO: Display boss name and car
	// TODO: Play boss intro cutscene
}

void BossEvent::StartOutro()
{
	// TODO: Show boss reaction dialogue
	// TODO: Award special boss rewards
}

void BossEvent::UpdateBossAI(float dt)
{
	if (!pGame || pGame->cars.empty()) return;
	
	// Boss AI follows ideal racing line
	// TODO: Implement boss AI that:
	// - Follows optimal racing line
	// - Blocks player overtakes
	// - Has superior car performance
	
	// Placeholder: boss maintains lead unless player is significantly faster
}

bool BossEvent::CheckBossDefeated() const
{
	// Boss is defeated if:
	// 1. Player finishes ahead of boss, OR
	// 2. Boss crashes/stops (KO)
	
	if (playerPosition == 1 && state == EVENT_STATE_FINISHED)
		return true;
	
	// TODO: Check if boss car is stopped/crashed
	// if (bossCar && bossCar->GetSpeed() < 0.1f && bossCar->GetDamage() >= 100.f)
	//     return true;
	
	return false;
}


//--------------------------------------------------------------------------------------------------------------------------
// PursuitEvent
//--------------------------------------------------------------------------------------------------------------------------

PursuitEvent::PursuitEvent(GAME* game) : BaseEvent(game)
{
}

void PursuitEvent::Start()
{
	BaseEvent::Start();
	
	isEscaped = false;
	isBusted = false;
	
	// Start pursuit with target heat level
	targetHeat = config.pursuitHeat > 0 ? config.pursuitHeat : 3;
	
	// Start the pursuit
	PursuitManager::Get().StartPursuit(targetHeat);
}

void PursuitEvent::Update(float dt)
{
	BaseEvent::Update(dt);
	
	if (state == EVENT_STATE_RACING)
	{
		// Update pursuit manager
		PursuitManager::Get().Update(dt);
		
		// Check win/loss conditions
		CheckWinCondition();
		CheckLossCondition();
	}
}

EventResults PursuitEvent::GetResults() const
{
	EventResults results = BaseEvent::GetResults();
	
	// Get pursuit results
	PursuitResults pursuitResults;
	pursuitResults.heatLevel = PursuitManager::Get().GetHeat().GetLevel();
	pursuitResults.pursuitTime = PursuitManager::Get().GetPursuitTime();
	pursuitResults.copsDisabled = PursuitManager::Get().GetCopsDisabled();
	pursuitResults.distanceTraveled = PursuitManager::Get().GetDistanceTraveled();
	pursuitResults.escaped = isEscaped;
	pursuitResults.busted = isBusted;
	
	pursuitResults.CalculateRewards();
	
	results.points = pursuitResults.repEarned;
	
	return results;
}

void PursuitEvent::CheckWinCondition()
{
	// Win if:
	// 1. Reached target heat level AND escaped
	// 2. Disabled required number of cops
	
	PursuitManager& pursuit = PursuitManager::Get();
	
	if (pursuit.IsEscaped())
	{
		isEscaped = true;
		
		// Check if target heat reached
		if (pursuit.GetHeat().GetLevel() >= targetHeat)
		{
			OnPlayerFinish(1);
		}
	}
	
	if (pursuit.GetCopsDisabled() >= copsToDisable)
	{
		OnPlayerFinish(1);
	}
}

void PursuitEvent::CheckLossCondition()
{
	// Lose if:
	// 1. Busted by police
	// 2. Time limit exceeded
	
	PursuitManager& pursuit = PursuitManager::Get();
	
	if (pursuit.IsBusted())
	{
		isBusted = true;
		OnPlayerFail();
	}
	
	if (config.timeLimit > 0.0f && raceTime > config.timeLimit)
	{
		OnPlayerFail();
	}
}


//--------------------------------------------------------------------------------------------------------------------------
// EventManager
//--------------------------------------------------------------------------------------------------------------------------

EventManager& EventManager::Get()
{
	static EventManager instance;
	return instance;
}

bool EventManager::Initialize(GAME* game)
{
	pGame = game;
	
	// Load event database
	LoadEventDatabase(PATHS::Data() + "/events/");
	
	return true;
}

void EventManager::Shutdown()
{
	EndCurrentEvent();
	eventDatabase.clear();
	unlockedEvents.clear();
}

BaseEvent* EventManager::CreateEvent(EventType type)
{
	if (!pGame) return nullptr;
	
	switch (type)
	{
		case EVENT_SPRINT:
			return new SprintEvent(pGame);
			
		case EVENT_CIRCUIT:
			return new CircuitEvent(pGame);
			
		case EVENT_DRIFT:
			return new DriftEvent(pGame);
			
		case EVENT_CANYON_DUEL:
			return new CanyonDuelEvent(pGame);
			
		case EVENT_CANYON_RUN:
			return new CanyonRunEvent(pGame);
			
		case EVENT_PURSUIT:
			return new PursuitEvent(pGame);
			
		case EVENT_BOSS:
			return new BossEvent(pGame);
			
		default:
			return nullptr;
	}
}

bool EventManager::StartEvent(const EventConfig& cfg)
{
	if (currentEvent != nullptr)
		return false;  // Event already active
	
	currentConfig = cfg;
	currentEvent = CreateEvent(cfg.type);
	
	if (!currentEvent)
		return false;
	
	if (!currentEvent->Init(cfg))
	{
		delete currentEvent;
		currentEvent = nullptr;
		return false;
	}
	
	currentEvent->Start();
	return true;
}

void EventManager::Update(float dt)
{
	if (currentEvent)
	{
		currentEvent->Update(dt);
		
		// Check if event ended
		if (currentEvent->GetState() == EVENT_STATE_FINISHED ||
			currentEvent->GetState() == EVENT_STATE_FAILED ||
			currentEvent->GetState() == EVENT_STATE_CANCELLED)
		{
			// Process results
			EventResults results = currentEvent->GetResults();
			
			// Award rewards
			if (results.isWin())
			{
				playerCash += currentConfig.rewardCash;
				playerRep += currentConfig.rewardRep;
			}
			
			// Cleanup
			delete currentEvent;
			currentEvent = nullptr;
		}
	}
}

void EventManager::EndCurrentEvent()
{
	if (currentEvent)
	{
		currentEvent->End();
		delete currentEvent;
		currentEvent = nullptr;
	}
}

bool EventManager::LoadEventDatabase(const std::string& path)
{
	eventDatabase.clear();
	unlockedEvents.clear();

	fs::path dir(path);
	if (!fs::exists(dir) || !fs::is_directory(dir))
		return false;

	std::vector<fs::path> files;
	for (const auto& entry : fs::directory_iterator(dir))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".xml")
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end());

	for (const auto& file : files)
	{
		EventConfig cfg;
		if (!cfg.LoadFromXml(file.string()) || cfg.id.empty())
			continue;

		eventDatabase[cfg.id] = cfg;
		if (cfg.requiredRep <= playerRep)
			unlockedEvents.push_back(cfg.id);
	}

	return !eventDatabase.empty();
}

const EventConfig* EventManager::GetEventConfig(const std::string& eventId) const
{
	auto it = eventDatabase.find(eventId);
	if (it != eventDatabase.end())
		return &it->second;
	return nullptr;
}

std::vector<const EventConfig*> EventManager::GetAvailableEvents() const
{
	std::vector<const EventConfig*> events;
	
	for (const auto& pair : eventDatabase)
	{
		// Check if event is unlocked
		for (const auto& unlocked : unlockedEvents)
		{
			if (unlocked == pair.first)
			{
				events.push_back(&pair.second);
				break;
			}
		}
	}
	
	return events;
}

std::vector<const EventConfig*> EventManager::GetEventsByType(EventType type) const
{
	std::vector<const EventConfig*> events;
	
	auto all = GetAvailableEvents();
	for (const auto* cfg : all)
	{
		if (cfg->type == type)
			events.push_back(cfg);
	}
	
	return events;
}

bool EventManager::IsEventUnlocked(const std::string& eventId) const
{
	for (const auto& id : unlockedEvents)
	{
		if (id == eventId)
			return true;
	}
	return false;
}

void EventManager::UnlockEvent(const std::string& eventId)
{
	if (!IsEventUnlocked(eventId))
	{
		unlockedEvents.push_back(eventId);
	}
}

void EventManager::AddReputation(int rep)
{
	playerRep += rep;
	
	// Check for new unlocked events based on reputation
	for (const auto& pair : eventDatabase)
	{
		if (!IsEventUnlocked(pair.first) &&
			pair.second.requiredRep <= playerRep)
		{
			UnlockEvent(pair.first);
		}
	}
}
