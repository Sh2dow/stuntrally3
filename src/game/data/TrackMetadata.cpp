#include "pch.h"
#include "TrackMetadata.h"
#include "tinyxml2.h"
#include "Def_Str.h"
#include <OgreStringConverter.h>
#include <algorithm>

using namespace tinyxml2;
using namespace Ogre;
using namespace std;


// Helper: convert Ogre::Vector3 to MATHVECTOR<float,3>
static MATHVECTOR<float,3> ToMathVec(const Ogre::Vector3& v)
{
	return MATHVECTOR<float,3>(v.x, v.y, v.z);
}

// Helper: convert MATHVECTOR<float,3> to Ogre::Vector3
static Ogre::Vector3 ToOgreVec(const MATHVECTOR<float,3>& v)
{
	return Ogre::Vector3(v[0], v[1], v[2]);
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackBarrier
//--------------------------------------------------------------------------------------------------------------------------

bool TrackBarrier::LoadXml(XMLElement* elem)
{
	const char* a;
	
	a = elem->Attribute("start");
	if (a)  start = ToMathVec(s2v(a));
	
	a = elem->Attribute("end");
	if (a)  end = ToMathVec(s2v(a));
	
	a = elem->Attribute("height");
	if (a)  height = s2r(a);
	
	a = elem->Attribute("group");
	if (a)  groupKey = s2i(a);
	
	a = elem->Attribute("playerOnly");
	if (a)  playerOnly = StringConverter::parseBool(a);
	
	a = elem->Attribute("handedness");
	if (a)  handedness = s2i(a);
	
	a = elem->Attribute("enabled");
	if (a)  enabled = StringConverter::parseBool(a);
	
	return true;
}

void TrackBarrier::SaveXml(XMLElement* elem, XMLDocument& doc) const
{
	elem->SetAttribute("start", toStr(ToOgreVec(start)).c_str());
	elem->SetAttribute("end", toStr(ToOgreVec(end)).c_str());
	elem->SetAttribute("height", fToStr(height).c_str());
	elem->SetAttribute("group", groupKey);
	elem->SetAttribute("playerOnly", playerOnly);
	elem->SetAttribute("handedness", handedness);
	elem->SetAttribute("enabled", enabled);
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackZone
//--------------------------------------------------------------------------------------------------------------------------

bool TrackZone::LoadXml(XMLElement* elem)
{
	const char* a;
	
	// Type
	a = elem->Attribute("type");
	if (a)
	{
		std::string typeName = std::string(a);
		type = ZONE_RESET;  // default
		for (int i = 0; i < ZONE_COUNT; ++i)
		{
			if (TrackZoneTypeNames[i] == typeName)
			{
				type = (TrackZoneType)i;
				break;
			}
		}
	}
	
	// Position
	a = elem->Attribute("pos");
	if (a)  position = ToMathVec(s2v(a));
	
	// Radius
	a = elem->Attribute("radius");
	if (a)  radius = s2r(a);
	
	// Length
	a = elem->Attribute("length");
	if (a)  length = s2r(a);
	
	// Group
	a = elem->Attribute("group");
	if (a)  groupKey = s2i(a);
	
	// Enabled
	a = elem->Attribute("enabled");
	if (a)  enabled = StringConverter::parseBool(a);
	
	// Custom parameters
	XMLElement* param = elem->FirstChildElement("param");
	while (param)
	{
		const char* key = param->Attribute("key");
		const char* val = param->Attribute("value");
		if (key && val)
			params[key] = val;
		param = param->NextSiblingElement("param");
	}
	
	return true;
}

void TrackZone::SaveXml(XMLElement* elem, XMLDocument& doc) const
{
	elem->SetAttribute("type", TrackZoneTypeNames[type].c_str());
	elem->SetAttribute("pos", toStr(ToOgreVec(position)).c_str());
	elem->SetAttribute("radius", fToStr(radius).c_str());
	elem->SetAttribute("length", fToStr(length).c_str());
	elem->SetAttribute("group", groupKey);
	elem->SetAttribute("enabled", enabled);
	
	// Custom parameters
	for (const auto& p : params)
	{
		XMLElement* param = doc.NewElement("param");
		param->SetAttribute("key", p.first.c_str());
		param->SetAttribute("value", p.second.c_str());
		elem->InsertEndChild(param);
	}
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackMetadata
//--------------------------------------------------------------------------------------------------------------------------

TrackMetadata::TrackMetadata()
{
	engagePos.Set(0, 0, 0);
}

bool TrackMetadata::LoadXml(const std::string& file)
{
	XMLDocument doc;
	XMLError er = doc.LoadFile(file.c_str());
	if (er != XML_SUCCESS)  return false;
	
	XMLElement* root = doc.RootElement();
	if (!root)  return false;
	
	const char* a;
	
	// Core identity
	a = root->Attribute("id");
	if (a)  id = std::string(a);
	
	a = root->Attribute("displayName");
	if (a)  displayName = std::string(a);
	
	a = root->Attribute("artName");
	if (a)  artName = std::string(a);
	
	a = root->Attribute("region");
	if (a)  region = std::string(a);
	
	// Engage position
	XMLElement* engage = root->FirstChildElement("engagePos");
	if (engage)
	{
		a = engage->Attribute("pos");
		if (a)  engagePos = ToMathVec(s2v(a));
		
		a = engage->Attribute("yaw");
		if (a)  engageYaw = s2r(a);
	}
	
	// Minimap assets
	XMLElement* minimap = root->FirstChildElement("minimap");
	if (minimap)
	{
		a = minimap->Attribute("locked");
		if (a)  minimapLocked = std::string(a);
		
		a = minimap->Attribute("unlocked");
		if (a)  minimapUnlocked = std::string(a);
	}
	
	a = root->Attribute("trackMap");
	if (a)  trackMap = std::string(a);
	
	// Stats
	XMLElement* stats = root->FirstChildElement("stats");
	if (stats)
	{
		a = stats->Attribute("length");
		if (a)  length = s2r(a);
		
		a = stats->Attribute("difficulty");
		if (a)  difficulty = s2i(a);
		
		a = stats->Attribute("eventTypes");
		if (a)  eventTypes = s2i(a);
	}
	
	// Progression
	XMLElement* prog = root->FirstChildElement("progression");
	if (prog)
	{
		a = prog->Attribute("isUnlocked");
		if (a)  isUnlocked = StringConverter::parseBool(a);
		
		a = prog->Attribute("districtId");
		if (a)  districtId = s2i(a);
	}
	
	// Zones
	XMLElement* zonesElem = root->FirstChildElement("zones");
	if (zonesElem)
	{
		XMLElement* zoneElem = zonesElem->FirstChildElement("zone");
		while (zoneElem)
		{
			TrackZone zone;
			zone.LoadXml(zoneElem);
			zones.push_back(zone);
			zoneElem = zoneElem->NextSiblingElement("zone");
		}
	}
	
	// Barriers
	XMLElement* barriersElem = root->FirstChildElement("barriers");
	if (barriersElem)
	{
		XMLElement* barrierElem = barriersElem->FirstChildElement("barrier");
		while (barrierElem)
		{
			TrackBarrier barrier;
			barrier.LoadXml(barrierElem);
			barriers.push_back(barrier);
			barrierElem = barrierElem->NextSiblingElement("barrier");
		}
	}
	
	return true;
}

bool TrackMetadata::SaveXml(const std::string& file) const
{
	XMLDocument xml;
	XMLElement* root = xml.NewElement("track");
	
	// Core identity
	root->SetAttribute("id", id.c_str());
	root->SetAttribute("displayName", displayName.c_str());
	root->SetAttribute("artName", artName.c_str());
	root->SetAttribute("region", region.c_str());
	
	// Engage position
	XMLElement* engage = xml.NewElement("engagePos");
	engage->SetAttribute("pos", toStr(ToOgreVec(engagePos)).c_str());
	engage->SetAttribute("yaw", fToStr(engageYaw).c_str());
	root->InsertEndChild(engage);
	
	// Minimap assets
	XMLElement* minimap = xml.NewElement("minimap");
	minimap->SetAttribute("locked", minimapLocked.c_str());
	minimap->SetAttribute("unlocked", minimapUnlocked.c_str());
	root->InsertEndChild(minimap);
	
	if (!trackMap.empty())
		root->SetAttribute("trackMap", trackMap.c_str());
	
	// Stats
	XMLElement* stats = xml.NewElement("stats");
	stats->SetAttribute("length", fToStr(length).c_str());
	stats->SetAttribute("difficulty", difficulty);
	stats->SetAttribute("eventTypes", eventTypes);
	root->InsertEndChild(stats);
	
	// Progression
	XMLElement* prog = xml.NewElement("progression");
	prog->SetAttribute("isUnlocked", isUnlocked);
	prog->SetAttribute("districtId", districtId);
	root->InsertEndChild(prog);
	
	// Zones
	if (!zones.empty())
	{
		XMLElement* zonesElem = xml.NewElement("zones");
		for (const auto& zone : zones)
		{
			XMLElement* zoneElem = xml.NewElement("zone");
			zone.SaveXml(zoneElem, xml);
			zonesElem->InsertEndChild(zoneElem);
		}
		root->InsertEndChild(zonesElem);
	}
	
	// Barriers
	if (!barriers.empty())
	{
		XMLElement* barriersElem = xml.NewElement("barriers");
		for (const auto& barrier : barriers)
		{
			XMLElement* barrierElem = xml.NewElement("barrier");
			barrier.SaveXml(barrierElem, xml);
			barriersElem->InsertEndChild(barrierElem);
		}
		root->InsertEndChild(barriersElem);
	}
	
	xml.InsertEndChild(root);
	return xml.SaveFile(file.c_str());
}

std::string TrackMetadata::GetZoneTypeName(TrackZoneType type) const
{
	if (type >= 0 && type < ZONE_COUNT)
		return TrackZoneTypeNames[type];
	return "unknown";
}

TrackZoneType TrackMetadata::GetZoneType(const std::string& name) const
{
	for (int i = 0; i < ZONE_COUNT; ++i)
	{
		if (TrackZoneTypeNames[i] == name)
			return (TrackZoneType)i;
	}
	return ZONE_RESET;
}

std::vector<TrackZone*> TrackMetadata::GetZonesByType(TrackZoneType type)
{
	std::vector<TrackZone*> result;
	for (auto& zone : zones)
	{
		if (zone.type == type)
			result.push_back(&zone);
	}
	return result;
}

std::vector<const TrackZone*> TrackMetadata::GetZonesByType(TrackZoneType type) const
{
	std::vector<const TrackZone*> result;
	for (const auto& zone : zones)
	{
		if (zone.type == type)
			result.push_back(&zone);
	}
	return result;
}

std::vector<TrackBarrier*> TrackMetadata::GetBarriersByGroup(int groupKey)
{
	std::vector<TrackBarrier*> result;
	for (auto& barrier : barriers)
	{
		if (barrier.groupKey == groupKey)
			result.push_back(&barrier);
	}
	return result;
}

std::vector<const TrackBarrier*> TrackMetadata::GetBarriersByGroup(int groupKey) const
{
	std::vector<const TrackBarrier*> result;
	for (const auto& barrier : barriers)
	{
		if (barrier.groupKey == groupKey)
			result.push_back(&barrier);
	}
	return result;
}


//--------------------------------------------------------------------------------------------------------------------------
// TrackDatabase
//--------------------------------------------------------------------------------------------------------------------------

TrackDatabase& TrackDatabase::Get()
{
	static TrackDatabase instance;
	return instance;
}

bool TrackDatabase::Load(const std::string& basePath)
{
	tracks.clear();
	trackIndex.clear();
	
	// Try to load track database file
	std::string dbFile = basePath + "TrackDatabase.xml";
	XMLDocument doc;
	XMLError er = doc.LoadFile(dbFile.c_str());
	
	if (er != XML_SUCCESS)
	{
		// No database file, create empty one
		return true;
	}
	
	XMLElement* root = doc.RootElement();
	if (!root)
		return false;
	
	XMLElement* trackElem = root->FirstChildElement("track");
	while (trackElem)
	{
		const char* id = trackElem->Attribute("id");
		if (id)
		{
			TrackMetadata info;
			info.id = std::string(id);
			
			const char* file = trackElem->Attribute("file");
			if (file)
			{
				std::string trackFile = basePath + std::string(file);
				if (info.LoadXml(trackFile))
				{
					tracks.push_back(info);
					trackIndex[info.id] = tracks.size() - 1;
				}
			}
		}
		trackElem = trackElem->NextSiblingElement("track");
	}
	
	return true;
}

bool TrackDatabase::Save(const std::string& basePath) const
{
	XMLDocument xml;
	XMLElement* root = xml.NewElement("trackDatabase");
	
	for (const auto& track : tracks)
	{
		XMLElement* trackElem = xml.NewElement("track");
		trackElem->SetAttribute("id", track.id.c_str());
		
		std::string fileName = track.id + ".xml";
		trackElem->SetAttribute("file", fileName.c_str());
		
		// Save individual track file
		std::string trackFile = basePath + fileName;
		track.SaveXml(trackFile);
		
		root->InsertEndChild(trackElem);
	}
	
	xml.InsertEndChild(root);
	
	std::string dbFile = basePath + "TrackDatabase.xml";
	return xml.SaveFile(dbFile.c_str());
}

const TrackMetadata* TrackDatabase::GetTrack(const std::string& trackId) const
{
	auto it = trackIndex.find(trackId);
	if (it != trackIndex.end())
	{
		return &tracks[it->second];
	}
	return nullptr;
}

TrackMetadata* TrackDatabase::GetTrack(const std::string& trackId)
{
	auto it = trackIndex.find(trackId);
	if (it != trackIndex.end())
	{
		return &tracks[it->second];
	}
	return nullptr;
}

std::vector<const TrackMetadata*> TrackDatabase::GetTracksByRegion(const std::string& region) const
{
	std::vector<const TrackMetadata*> result;
	for (const auto& track : tracks)
	{
		if (track.region == region)
			result.push_back(&track);
	}
	return result;
}

std::vector<const TrackMetadata*> TrackDatabase::GetTracksByDistrict(int districtId) const
{
	std::vector<const TrackMetadata*> result;
	for (const auto& track : tracks)
	{
		if (track.districtId == districtId)
			result.push_back(&track);
	}
	return result;
}

void TrackDatabase::UnlockTrack(const std::string& trackId)
{
	TrackMetadata* track = GetTrack(trackId);
	if (track)
		track->isUnlocked = true;
}

void TrackDatabase::LockTrack(const std::string& trackId)
{
	TrackMetadata* track = GetTrack(trackId);
	if (track)
		track->isUnlocked = false;
}

bool TrackDatabase::IsTrackUnlocked(const std::string& trackId) const
{
	const TrackMetadata* track = GetTrack(trackId);
	if (track)
		return track->isUnlocked;
	return false;
}

void TrackDatabase::EnableBarrierGroup(const std::string& trackId, int groupKey)
{
	TrackMetadata* track = GetTrack(trackId);
	if (track)
	{
		auto barriers = track->GetBarriersByGroup(groupKey);
		for (auto* barrier : barriers)
			barrier->enabled = true;
	}
}

void TrackDatabase::DisableBarrierGroup(const std::string& trackId, int groupKey)
{
	TrackMetadata* track = GetTrack(trackId);
	if (track)
	{
		auto barriers = track->GetBarriersByGroup(groupKey);
		for (auto* barrier : barriers)
			barrier->enabled = false;
	}
}
