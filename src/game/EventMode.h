#pragma once
#include <string>
#include <vector>
#include <map>
#include <OgreVector3.h>
#include "mathvector.h"
#include "quaternion.h"

class GAME;
class CAR;
class TrackMetadata;

/// 🏁 Event Types
enum EventType
{
	EVENT_NONE = 0,
	EVENT_SPRINT,          // Point-to-point race
	EVENT_CIRCUIT,         // Multi-lap circuit
	EVENT_DRIFT,           // Drift competition
	EVENT_CANYON_DUEL,     // Two-car canyon battle
	EVENT_CANYON_RUN,      // Single canyon run
	EVENT_TOLL_BOOTH,      // Time trial through tolls
	EVENT_PURSUIT,         // Police pursuit
	EVENT_BOSS,            // Boss battle
	EVENT_COUNT
};

const static std::string EventTypeNames[EVENT_COUNT] = {
	"None", "Sprint", "Circuit", "Drift", "Canyon Duel", "Canyon Run",
	"Toll Booth", "Pursuit", "Boss"
};

/// 🏆 Event State
enum EventState
{
	EVENT_STATE_IDLE = 0,
	EVENT_STATE_COUNTDOWN,    // 3..2..1..GO!
	EVENT_STATE_RACING,       // Active racing
	EVENT_STATE_FINISHED,     // Completed
	EVENT_STATE_FAILED,       // Failed event
	EVENT_STATE_CANCELLED     // Cancelled by player
};

/// 📊 Event Results
struct EventResults
{
	int position = 0;         // Final position (1 = first)
	int totalRacers = 0;      // Total participants
	float time = 0.0f;        // Total time (seconds)
	float bestLap = 0.0f;     // Best lap time
	int points = 0;           // Points earned
	
	// Drift-specific
	float driftScore = 0.0f;
	float driftCombo = 0.0f;
	int driftZones = 0;
	
	// Canyon-specific
	float playerTime = 0.0f;
	float rivalTime = 0.0f;
	float timeDiff = 0.0f;
	bool wonByKO = false;     // Knocked out rival
	
	// Pursuit-specific
	int heatLevel = 0;
	float evasionTime = 0.0f;
	int busts = 0;
	
	bool isWin() const;
	std::string GetResultString() const;
};

/// 🎯 Event Configuration
struct EventConfig
{
	EventType type = EVENT_NONE;
	std::string id;           // Unique event ID
	std::string name;         // Display name
	std::string description;  // Description text
	
	std::string trackId;      // Track to race on
	bool trackReversed = false;
	
	int laps = 1;             // Number of laps (0 for sprint)
	float timeLimit = 0.0f;   // Time limit (0 = none)
	
	int difficulty = 1;       // 1-5 difficulty
	int rewardCash = 0;       // Cash prize
	int rewardRep = 0;        // Reputation points
	
	// Opponents
	std::vector<std::string> opponentCars;  // Car IDs
	int opponentCount = 0;
	
	// Event-specific
	float driftTarget = 0.0f;      // Drift target score
	float canyonTarget = 0.0f;     // Canyon target time
	int tollCount = 0;             // Toll booth count
	int pursuitHeat = 0;           // Pursuit heat level
	
	// Requirements
	int requiredRep = 0;           // Required reputation
	std::vector<std::string> requiredCars;  // Required car types
	
	bool LoadFromXml(const std::string& file);
	bool SaveToXml(const std::string& file) const;
};

/// 🎮 Base Event Class
class BaseEvent
{
public:
	BaseEvent(GAME* game);
	virtual ~BaseEvent();
	
	// Lifecycle
	virtual bool Init(const EventConfig& config);
	virtual void Start();
	virtual void Update(float dt);
	virtual void End();
	
	// State
	EventState GetState() const { return state; }
	EventType GetType() const { return config.type; }
	const EventConfig& GetConfig() const { return config; }
	
	// Race data
	virtual int GetPlayerPosition() const;
	virtual int GetTotalRacers() const;
	virtual float GetRaceTime() const;
	virtual float GetBestLap() const;
	
	// Event-specific (override in derived classes)
	virtual float GetDriftScore() const { return 0.0f; }
	virtual float GetDriftCombo() const { return 0.0f; }
	virtual float GetTimeDelta() const { return 0.0f; }  // Canyon duel
	virtual int GetHeatLevel() const { return 0; }       // Pursuit
	
	// Results
	virtual EventResults GetResults() const;
	
	// Callbacks
	virtual void OnPlayerFinish(int position);
	virtual void OnPlayerFail();
	virtual void OnLapComplete(int lap, float lapTime);
	virtual void OnCheckpoint(int index);
	
protected:
	GAME* pGame = nullptr;
	EventConfig config;
	EventState state = EVENT_STATE_IDLE;
	
	float raceTime = 0.0f;
	float countdownTime = 0.0f;
	int currentLap = 0;
	int playerPosition = 1;
	
	// Timing
	std::vector<float> lapTimes;
	float bestLapTime = 0.0f;
	
	// Helpers
	virtual void UpdateCountdown(float dt);
	virtual void UpdateRace(float dt);
	virtual void CheckFinishCondition();
	
	virtual bool IsLapComplete() const;
	virtual bool IsRaceComplete() const;
	virtual int CalculatePosition() const;
};

/// 🏁 Sprint Event (Point-to-Point)
class SprintEvent : public BaseEvent
{
public:
	SprintEvent(GAME* game);
	
	void Start() override;
	void Update(float dt) override;
	
protected:
	MATHVECTOR<float,3> startPos;
	MATHVECTOR<float,3> endPos;
	float trackLength = 0.0f;
	float playerProgress = 0.0f;  // 0.0 to 1.0
	
	virtual float CalculateProgress(CAR* car) const;
};

/// 🏎️ Circuit Event (Multi-Lap)
class CircuitEvent : public BaseEvent
{
public:
	CircuitEvent(GAME* game);
	
	bool Init(const EventConfig& config) override;
	void Update(float dt) override;
	
protected:
	int totalLaps = 0;
	std::vector<int> opponentLaps;
	
	virtual void CheckLapComplete();
};

/// 💨 Drift Event
class DriftEvent : public BaseEvent
{
public:
	DriftEvent(GAME* game);
	
	void Start() override;
	void Update(float dt) override;
	
	float GetDriftScore() const override;
	float GetDriftCombo() const override;
	EventResults GetResults() const override;
	
protected:
	float driftScore = 0.0f;
	float driftCombo = 0.0f;
	float currentDrift = 0.0f;
	float driftStartTime = 0.0f;
	bool isDrifting = false;
	
	int driftZonesHit = 0;
	int driftZonesTotal = 0;
	
	virtual void UpdateDriftScore(float dt);
	virtual float CalculateDriftAmount(CAR* car) const;
	virtual void OnDriftZoneEnter(int zoneIndex);
	virtual void OnDriftZoneExit(int zoneIndex, float score);
};

/// ⛰️ Canyon Duel Event
class CanyonDuelEvent : public BaseEvent
{
public:
	CanyonDuelEvent(GAME* game);
	
	void Start() override;
	void Update(float dt) override;
	
	float GetTimeDelta() const override;
	EventResults GetResults() const override;
	
protected:
	enum DuelPhase
	{
		PHASE_PLAYER_RUN,     // Player goes first
		PHASE_RIVAL_RUN,      // Rival goes second
		PHASE_RESULT          // Show winner
	};
	
	DuelPhase phase = PHASE_PLAYER_RUN;
	float playerBestTime = 0.0f;
	float rivalBestTime = 0.0f;
	float currentTime = 0.0f;
	
	int playerKOs = 0;        // Times player knocked out rival
	int rivalKOs = 0;         // Times rival knocked out player
	
	virtual void StartPlayerRun();
	virtual void StartRivalRun();
	virtual void UpdateRival(float dt);
	virtual bool CheckKnockout();
};

/// 🏁 Canyon Run Event (Time Attack)
class CanyonRunEvent : public BaseEvent
{
public:
	CanyonRunEvent(GAME* game);
	
	void Update(float dt) override;
	
protected:
	float targetTime = 0.0f;
	float currentTime = 0.0f;
	
	virtual void CheckTargetTime();
};

/// 🚓 Pursuit Event
class PursuitEvent : public BaseEvent
{
public:
	PursuitEvent(GAME* game);
	
	void Start() override;
	void Update(float dt) override;
	EventResults GetResults() const override;
	
protected:
	int targetHeat = 0;           // Heat level to reach
	float timeLimit = 0.0f;       // Time limit
	int copsToDisable = 0;        // Cops to disable to win
	
	bool isEscaped = false;
	bool isBusted = false;
	
	virtual void CheckWinCondition();
	virtual void CheckLossCondition();
};

/// 👑 Boss Race Event
class BossEvent : public BaseEvent
{
public:
	BossEvent(GAME* game);
	
	void Start() override;
	void Update(float dt) override;
	EventResults GetResults() const override;
	
protected:
	enum BossPhase
	{
		PHASE_INTRO,        // Boss intro dialogue
		PHASE_RACE,         // Actual race
		PHASE_OUTRO         // Boss reaction
	};
	
	BossPhase phase = PHASE_INTRO;
	float phaseTime = 0.0f;
	
	std::string bossName;
	std::string bossCar;
	int bossDifficulty = 5;
	
	bool bossDefeated = false;
	int rematchCount = 0;
	
	virtual void StartIntro();
	virtual void StartOutro();
	virtual void UpdateBossAI(float dt);
	virtual bool CheckBossDefeated() const;
};

/// 📋 Event Manager
/// Manages all active and available events
class EventManager
{
public:
	static EventManager& Get();
	
	// Initialization
	bool Initialize(GAME* game);
	void Shutdown();
	
	// Event lifecycle
	BaseEvent* CreateEvent(EventType type);
	bool StartEvent(const EventConfig& config);
	void Update(float dt);
	void EndCurrentEvent();
	
	// Current event access
	BaseEvent* GetCurrentEvent() { return currentEvent; }
	const EventConfig& GetCurrentConfig() const { return currentConfig; }
	bool IsEventActive() const { return currentEvent != nullptr; }
	
	// Event database
	bool LoadEventDatabase(const std::string& path);
	const EventConfig* GetEventConfig(const std::string& eventId) const;
	std::vector<const EventConfig*> GetAvailableEvents() const;
	std::vector<const EventConfig*> GetEventsByType(EventType type) const;
	
	// Progression
	bool IsEventUnlocked(const std::string& eventId) const;
	void UnlockEvent(const std::string& eventId);
	int GetPlayerReputation() const { return playerRep; }
	void AddReputation(int rep);
	
private:
	EventManager() = default;
	
	GAME* pGame = nullptr;
	BaseEvent* currentEvent = nullptr;
	EventConfig currentConfig;
	
	std::map<std::string, EventConfig> eventDatabase;
	std::vector<std::string> unlockedEvents;
	
	int playerRep = 0;
	int playerCash = 0;
};
