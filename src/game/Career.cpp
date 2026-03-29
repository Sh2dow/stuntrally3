#include "pch.h"
#include "Career.h"
#include "tinyxml2.h"
#include "Def_Str.h"

using namespace tinyxml2;


//--------------------------------------------------------------------------------------------------------------------------
// CareerProgress
//--------------------------------------------------------------------------------------------------------------------------

bool CareerProgress::LoadFromXml(const std::string& file)
{
    XMLDocument doc;
    XMLError er = doc.LoadFile(file.c_str());
    if (er != XML_SUCCESS) return false;
    
    XMLElement* root = doc.RootElement();
    if (!root) return false;
    
    const char* a;
    
    // Player stats
    a = root->Attribute("reputation");
    if (a) reputation = s2i(a);
    
    a = root->Attribute("cash");
    if (a) cash = s2i(a);
    
    a = root->Attribute("level");
    if (a) level = s2i(a);
    
    // Career progress
    a = root->Attribute("currentDistrict");
    if (a) currentDistrict = s2i(a);
    
    a = root->Attribute("bossesDefeated");
    if (a) bossesDefeated = s2i(a);
    
    // Completed events
    XMLElement* eventsElem = root->FirstChildElement("completedEvents");
    if (eventsElem)
    {
        XMLElement* eventElem = eventsElem->FirstChildElement("event");
        while (eventElem)
        {
            a = eventElem->GetText();
            if (a) completedEvents.push_back(std::string(a));
            eventElem = eventElem->NextSiblingElement("event");
        }
    }
    
    // Unlocked events
    eventsElem = root->FirstChildElement("unlockedEvents");
    if (eventsElem)
    {
        XMLElement* eventElem = eventsElem->FirstChildElement("event");
        while (eventElem)
        {
            a = eventElem->GetText();
            if (a) unlockedEvents.push_back(std::string(a));
            eventElem = eventElem->NextSiblingElement("event");
        }
    }
    
    // Owned cars
    XMLElement* carsElem = root->FirstChildElement("ownedCars");
    if (carsElem)
    {
        XMLElement* carElem = carsElem->FirstChildElement("car");
        while (carElem)
        {
            a = carElem->GetText();
            if (a) ownedCars.push_back(std::string(a));
            carElem = carElem->NextSiblingElement("car");
        }
    }
    
    a = root->Attribute("primaryCar");
    if (a) primaryCar = std::string(a);
    
    // District control
    XMLElement* districtsElem = root->FirstChildElement("districtControl");
    if (districtsElem)
    {
        XMLElement* districtElem = districtsElem->FirstChildElement("district");
        while (districtElem)
        {
            int id = 0;
            float control = 0.0f;
            
            a = districtElem->Attribute("id");
            if (a) id = s2i(a);
            
            a = districtElem->Attribute("control");
            if (a) control = s2r(a);
            
            districtControl[id] = control;
            
            districtElem = districtElem->NextSiblingElement("district");
        }
    }
    
    // Best times
    XMLElement* timesElem = root->FirstChildElement("bestTimes");
    if (timesElem)
    {
        XMLElement* timeElem = timesElem->FirstChildElement("time");
        while (timeElem)
        {
            std::string eventId;
            float time = 0.0f;
            
            a = timeElem->Attribute("event");
            if (a) eventId = std::string(a);
            
            a = timeElem->GetText();
            if (a) time = s2r(a);
            
            if (!eventId.empty())
                bestLapTimes[eventId] = time;
            
            timeElem = timeElem->NextSiblingElement("time");
        }
    }
    
    // Best scores
    XMLElement* scoresElem = root->FirstChildElement("bestScores");
    if (scoresElem)
    {
        XMLElement* scoreElem = scoresElem->FirstChildElement("score");
        while (scoreElem)
        {
            std::string eventId;
            float score = 0.0f;
            
            a = scoreElem->Attribute("event");
            if (a) eventId = std::string(a);
            
            a = scoreElem->GetText();
            if (a) score = s2r(a);
            
            if (!eventId.empty())
                bestDriftScores[eventId] = score;
            
            scoreElem = scoreElem->NextSiblingElement("score");
        }
    }
    
    return true;
}

bool CareerProgress::SaveToXml(const std::string& file) const
{
    XMLDocument xml;
    XMLElement* root = xml.NewElement("career");
    
    // Player stats
    root->SetAttribute("reputation", reputation);
    root->SetAttribute("cash", cash);
    root->SetAttribute("level", level);
    
    // Career progress
    root->SetAttribute("currentDistrict", currentDistrict);
    root->SetAttribute("bossesDefeated", bossesDefeated);
    
    // Completed events
    if (!completedEvents.empty())
    {
        XMLElement* eventsElem = xml.NewElement("completedEvents");
        for (const auto& eventId : completedEvents)
        {
            XMLElement* eventElem = xml.NewElement("event");
            eventElem->SetText(eventId.c_str());
            eventsElem->InsertEndChild(eventElem);
        }
        root->InsertEndChild(eventsElem);
    }
    
    // Unlocked events
    if (!unlockedEvents.empty())
    {
        XMLElement* eventsElem = xml.NewElement("unlockedEvents");
        for (const auto& eventId : unlockedEvents)
        {
            XMLElement* eventElem = xml.NewElement("event");
            eventElem->SetText(eventId.c_str());
            eventsElem->InsertEndChild(eventElem);
        }
        root->InsertEndChild(eventsElem);
    }
    
    // Owned cars
    if (!ownedCars.empty())
    {
        XMLElement* carsElem = xml.NewElement("ownedCars");
        for (const auto& carId : ownedCars)
        {
            XMLElement* carElem = xml.NewElement("car");
            carElem->SetText(carId.c_str());
            carsElem->InsertEndChild(carElem);
        }
        root->InsertEndChild(carsElem);
    }
    
    if (!primaryCar.empty())
        root->SetAttribute("primaryCar", primaryCar.c_str());
    
    // District control
    if (!districtControl.empty())
    {
        XMLElement* districtsElem = xml.NewElement("districtControl");
        for (const auto& pair : districtControl)
        {
            XMLElement* districtElem = xml.NewElement("district");
            districtElem->SetAttribute("id", pair.first);
            districtElem->SetAttribute("control", fToStr(pair.second).c_str());
            districtsElem->InsertEndChild(districtElem);
        }
        root->InsertEndChild(districtsElem);
    }
    
    // Best times
    if (!bestLapTimes.empty())
    {
        XMLElement* timesElem = xml.NewElement("bestTimes");
        for (const auto& pair : bestLapTimes)
        {
            XMLElement* timeElem = xml.NewElement("time");
            timeElem->SetAttribute("event", pair.first.c_str());
            timeElem->SetText(fToStr(pair.second).c_str());
            timesElem->InsertEndChild(timeElem);
        }
        root->InsertEndChild(timesElem);
    }
    
    // Best scores
    if (!bestDriftScores.empty())
    {
        XMLElement* scoresElem = xml.NewElement("bestScores");
        for (const auto& pair : bestDriftScores)
        {
            XMLElement* scoreElem = xml.NewElement("score");
            scoreElem->SetAttribute("event", pair.first.c_str());
            scoreElem->SetText(fToStr(pair.second).c_str());
            scoresElem->InsertEndChild(scoreElem);
        }
        root->InsertEndChild(scoresElem);
    }
    
    xml.InsertEndChild(root);
    return xml.SaveFile(file.c_str());
}


//--------------------------------------------------------------------------------------------------------------------------
// District
//--------------------------------------------------------------------------------------------------------------------------

bool District::IsUnlocked(const CareerProgress& career) const
{
    // Check reputation requirement
    if (career.reputation < requiredRep)
        return false;
    
    // Check boss requirement
    if (career.bossesDefeated < requiredBosses)
        return false;
    
    return true;
}

bool District::IsComplete() const
{
    return playerControl >= 100.0f;
}


//--------------------------------------------------------------------------------------------------------------------------
// CareerManager
//--------------------------------------------------------------------------------------------------------------------------

CareerManager& CareerManager::Get()
{
    static CareerManager instance;
    return instance;
}

bool CareerManager::Initialize()
{
    // Initialize level thresholds
    levelThresholds = {
        0,      // Level 1
        500,    // Level 2
        1500,   // Level 3
        3000,   // Level 4
        5000,   // Level 5
        8000,   // Level 6
        12000,  // Level 7
        17000,  // Level 8
        23000,  // Level 9
        30000,  // Level 10
        50000   // Level 11+
    };
    
    // Load default career data
    LoadCareer("data/career/career.xml");
    
    return true;
}

void CareerManager::Shutdown()
{
    districts.clear();
    districtMap.clear();
    rivals.clear();
    rivalMap.clear();
}

bool CareerManager::LoadCareer(const std::string& file)
{
    XMLDocument doc;
    XMLError er = doc.LoadFile(file.c_str());
    if (er != XML_SUCCESS) return false;
    
    XMLElement* root = doc.RootElement();
    if (!root) return false;
    
    // Load districts
    XMLElement* districtsElem = root->FirstChildElement("districts");
    if (districtsElem)
    {
        XMLElement* districtElem = districtsElem->FirstChildElement("district");
        while (districtElem)
        {
            District district;
            
            const char* a = districtElem->Attribute("id");
            if (a) district.id = s2i(a);
            
            a = districtElem->Attribute("name");
            if (a) district.name = std::string(a);
            
            a = districtElem->Attribute("desc");
            if (a) district.description = std::string(a);
            
            a = districtElem->Attribute("minimap");
            if (a) district.minimapImage = std::string(a);
            
            a = districtElem->Attribute("cameraDist");
            if (a) district.cameraDistance = s2r(a);
            
            a = districtElem->Attribute("requiredRep");
            if (a) district.requiredRep = s2i(a);
            
            a = districtElem->Attribute("requiredBosses");
            if (a) district.requiredBosses = s2i(a);
            
            a = districtElem->Attribute("completionCash");
            if (a) district.completionCash = s2i(a);
            
            a = districtElem->Attribute("completionRep");
            if (a) district.completionRep = s2i(a);
            
            a = districtElem->Attribute("unlockCar");
            if (a) district.unlockCar = std::string(a);
            
            // Events
            XMLElement* eventsElem = districtElem->FirstChildElement("events");
            if (eventsElem)
            {
                XMLElement* eventElem = eventsElem->FirstChildElement("event");
                while (eventElem)
                {
                    a = eventElem->GetText();
                    if (a) district.events.push_back(std::string(a));
                    eventElem = eventElem->NextSiblingElement("event");
                }
            }
            
            a = districtElem->Attribute("bossEvent");
            if (a) district.bossEventId = std::string(a);
            
            districts.push_back(district);
            districtMap[district.id] = (int)(districts.size() - 1);
            
            districtElem = districtElem->NextSiblingElement("district");
        }
    }
    
    // Load rivals
    XMLElement* rivalsElem = root->FirstChildElement("rivals");
    if (rivalsElem)
    {
        XMLElement* rivalElem = rivalsElem->FirstChildElement("rival");
        while (rivalElem)
        {
            Rival rival;
            
            const char* a = rivalElem->Attribute("id");
            if (a) rival.id = std::string(a);
            
            a = rivalElem->Attribute("name");
            if (a) rival.name = std::string(a);
            
            a = rivalElem->Attribute("desc");
            if (a) rival.description = std::string(a);
            
            a = rivalElem->Attribute("district");
            if (a) rival.districtId = s2i(a);
            
            a = rivalElem->Attribute("car");
            if (a) rival.bossCar = std::string(a);
            
            a = rivalElem->Attribute("difficulty");
            if (a) rival.difficulty = s2i(a);
            
            a = rivalElem->Attribute("defeatCash");
            if (a) rival.defeatCash = s2i(a);
            
            a = rivalElem->Attribute("defeatRep");
            if (a) rival.defeatRep = s2i(a);
            
            a = rivalElem->Attribute("unlockEvent");
            if (a) rival.unlockEvent = std::string(a);
            
            XMLElement* dialogueElem = rivalElem->FirstChildElement("dialogue");
            if (dialogueElem)
            {
                a = dialogueElem->Attribute("intro");
                if (a) rival.introDialogue = std::string(a);
                
                a = dialogueElem->Attribute("outro");
                if (a) rival.outroDialogue = std::string(a);
            }
            
            rivals.push_back(rival);
            rivalMap[rival.id] = (int)(rivals.size() - 1);
            
            rivalElem = rivalElem->NextSiblingElement("rival");
        }
    }
    
    // Try to load player progress
    progress.LoadFromXml("data/career/save.xml");
    
    return true;
}

bool CareerManager::SaveCareer(const std::string& file)
{
    return progress.SaveToXml(file);
}

void CareerManager::NewCareer()
{
    progress = CareerProgress();
    
    // Unlock first district
    if (!districts.empty())
    {
        progress.currentDistrict = districts[0].id;
    }
    
    // Give starter car
    progress.ownedCars.push_back("HN");  // Honda Civic (example)
    progress.primaryCar = "HN";
    
    // Unlock starter events
    UnlockEvent("tutorial_01");
}

const District* CareerManager::GetDistrict(int id) const
{
    auto it = districtMap.find(id);
    if (it != districtMap.end())
        return &districts[it->second];
    return nullptr;
}

const District* CareerManager::GetCurrentDistrict() const
{
    return GetDistrict(progress.currentDistrict);
}

std::vector<const District*> CareerManager::GetAllDistricts() const
{
    std::vector<const District*> result;
    for (const auto& district : districts)
    {
        result.push_back(&district);
    }
    return result;
}

float CareerManager::GetDistrictControl(int districtId) const
{
    auto it = progress.districtControl.find(districtId);
    if (it != progress.districtControl.end())
        return it->second;
    return 0.0f;
}

void CareerManager::UpdateDistrictControl(int districtId, float delta)
{
    float current = GetDistrictControl(districtId);
    float newValue = std::min(100.0f, current + delta);
    progress.districtControl[districtId] = newValue;
    
    // Check for district completion
    if (newValue >= 100.0f)
    {
        const District* district = GetDistrict(districtId);
        if (district)
        {
            AddCash(district->completionCash);
            AddReputation(district->completionRep);
            
            if (!district->unlockCar.empty())
            {
                AddCar(district->unlockCar);
            }
        }
    }
    
    CheckUnlocks();
}

bool CareerManager::IsDistrictComplete(int districtId) const
{
    return GetDistrictControl(districtId) >= 100.0f;
}

bool CareerManager::IsEventCompleted(const std::string& eventId) const
{
    for (const auto& id : progress.completedEvents)
    {
        if (id == eventId)
            return true;
    }
    return false;
}

bool CareerManager::IsEventUnlocked(const std::string& eventId) const
{
    for (const auto& id : progress.unlockedEvents)
    {
        if (id == eventId)
            return true;
    }
    return false;
}

void CareerManager::MarkEventCompleted(const std::string& eventId)
{
    if (!IsEventCompleted(eventId))
    {
        progress.completedEvents.push_back(eventId);
        
        // Update district control based on event
        for (const auto& district : districts)
        {
            for (const auto& evt : district.events)
            {
                if (evt == eventId)
                {
                    UpdateDistrictControl(district.id, 20.0f);  // 5 events = 100%
                    break;
                }
            }
        }
    }
}

void CareerManager::UnlockEvent(const std::string& eventId)
{
    if (!IsEventUnlocked(eventId))
    {
        progress.unlockedEvents.push_back(eventId);
    }
}

std::vector<std::string> CareerManager::GetAvailableEvents() const
{
    std::vector<std::string> events;
    
    for (const auto& eventId : progress.unlockedEvents)
    {
        if (!IsEventCompleted(eventId))
        {
            events.push_back(eventId);
        }
    }
    
    return events;
}

std::vector<std::string> CareerManager::GetDistrictEvents(int districtId) const
{
    std::vector<std::string> events;
    
    const District* district = GetDistrict(districtId);
    if (district)
    {
        for (const auto& eventId : district->events)
        {
            if (IsEventUnlocked(eventId))
            {
                events.push_back(eventId);
            }
        }
        
        // Add boss event if district is ready
        if (!district->bossEventId.empty() && CanChallengeBoss(districtId))
        {
            events.push_back(district->bossEventId);
        }
    }
    
    return events;
}

const Rival* CareerManager::GetRival(const std::string& id) const
{
    auto it = rivalMap.find(id);
    if (it != rivalMap.end())
        return &rivals[it->second];
    return nullptr;
}

const Rival* CareerManager::GetDistrictBoss(int districtId) const
{
    for (const auto& rival : rivals)
    {
        if (rival.districtId == districtId)
            return &rival;
    }
    return nullptr;
}

bool CareerManager::IsRivalDefeated(const std::string& rivalId) const
{
    const Rival* rival = GetRival(rivalId);
    return rival && rival->isDefeated;
}

void CareerManager::MarkRivalDefeated(const std::string& rivalId)
{
    auto it = rivalMap.find(rivalId);
    if (it != rivalMap.end())
    {
        rivals[it->second].isDefeated = true;
        progress.bossesDefeated++;
        
        // Award rewards
        const Rival* rival = &rivals[it->second];
        AddCash(rival->defeatCash);
        AddReputation(rival->defeatRep);
        
        // Unlock next content
        if (!rival->unlockEvent.empty())
        {
            UnlockEvent(rival->unlockEvent);
        }
    }
}

void CareerManager::AddReputation(int rep)
{
    progress.reputation += rep;
    CalculateLevel();
    CheckUnlocks();
}

void CareerManager::AddCash(int cash)
{
    progress.cash += cash;
}

void CareerManager::SpendCash(int cash)
{
    progress.cash = std::max(0, progress.cash - cash);
}

bool CareerManager::CanAccessDistrict(int districtId) const
{
    const District* district = GetDistrict(districtId);
    if (!district) return false;
    
    return district->IsUnlocked(progress);
}

bool CareerManager::CanChallengeBoss(int districtId) const
{
    const District* district = GetDistrict(districtId);
    if (!district) return false;
    
    // Must have high district control
    if (GetDistrictControl(districtId) < 75.0f)
        return false;
    
    // Must be unlocked
    return district->IsUnlocked(progress);
}

bool CareerManager::OwnsCar(const std::string& carId) const
{
    for (const auto& id : progress.ownedCars)
    {
        if (id == carId)
            return true;
    }
    return false;
}

void CareerManager::AddCar(const std::string& carId)
{
    if (!OwnsCar(carId))
    {
        progress.ownedCars.push_back(carId);
    }
}

void CareerManager::SetPrimaryCar(const std::string& carId)
{
    if (OwnsCar(carId))
    {
        progress.primaryCar = carId;
    }
}

float CareerManager::GetBestLapTime(const std::string& eventId) const
{
    auto it = progress.bestLapTimes.find(eventId);
    if (it != progress.bestLapTimes.end())
        return it->second;
    return 0.0f;
}

float CareerManager::GetBestDriftScore(const std::string& eventId) const
{
    auto it = progress.bestDriftScores.find(eventId);
    if (it != progress.bestDriftScores.end())
        return it->second;
    return 0.0f;
}

void CareerManager::SetBestLapTime(const std::string& eventId, float time)
{
    float current = GetBestLapTime(eventId);
    if (current == 0.0f || time < current)
    {
        progress.bestLapTimes[eventId] = time;
    }
}

void CareerManager::SetBestDriftScore(const std::string& eventId, float score)
{
    float current = GetBestDriftScore(eventId);
    if (score > current)
    {
        progress.bestDriftScores[eventId] = score;
    }
}

int CareerManager::GetRepForNextLevel() const
{
    int level = progress.level;
    if (level < (int)levelThresholds.size())
        return levelThresholds[level];
    return levelThresholds.back() + (level - (int)levelThresholds.size()) * 10000;
}

float CareerManager::GetLevelProgress() const
{
    int currentRep = progress.reputation;
    int prevThreshold = (progress.level > 1) ? levelThresholds[progress.level - 2] : 0;
    int nextThreshold = GetRepForNextLevel();
    
    if (nextThreshold <= prevThreshold)
        return 1.0f;
    
    float progress = (float)(currentRep - prevThreshold) / (float)(nextThreshold - prevThreshold);
    return std::max(0.0f, std::min(1.0f, progress));
}

void CareerManager::CalculateLevel()
{
    int rep = progress.reputation;
    int newLevel = 1;
    
    for (size_t i = 0; i < levelThresholds.size(); ++i)
    {
        if (rep >= levelThresholds[i])
            newLevel = (int)i + 1;
        else
            break;
    }
    
    progress.level = newLevel;
}

void CareerManager::CheckUnlocks()
{
    // Check district unlocks
    for (const auto& district : districts)
    {
        if (district.IsUnlocked(progress))
        {
            // Unlock district events
            for (const auto& eventId : district.events)
            {
                UnlockEvent(eventId);
            }
        }
    }
    
    // Check rival unlocks
    for (const auto& rival : rivals)
    {
        if (CanChallengeBoss(rival.districtId))
        {
            UnlockEvent(rival.id);  // Use rival ID as event ID
        }
    }
}
