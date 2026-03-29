#pragma once
#include "TrackMetadata.h"
#include <OgrePrerequisites.h>

namespace Ogre {
	class TextureGpu;
	class TextureManager;
	class Image2;
	class PixelBox;
}

/// 🗺️ Track Minimap Generator
/// Generates minimap textures from track data
class TrackMinimap
{
public:
	TrackMinimap();
	~TrackMinimap();
	
	// Generate minimap from track metadata
	bool Generate(const TrackMetadata* track, const std::string& outputPath);
	
	// Generate minimap from road spline
	bool GenerateFromSpline(const std::vector<Ogre::Vector3>& points, 
		float width, const std::string& outputPath);
	
	// Load existing minimap
	Ogre::TextureGpu* LoadMinimap(const std::string& path);
	
	// Get minimap for track (cached)
	Ogre::TextureGpu* GetMinimap(const TrackMetadata* track, bool locked = false);
	
	// Settings
	void SetSize(int width, int height) { texWidth = width; texHeight = height; }
	void SetBgColor(const Ogre::Vector3& color) { bgColor = color; }
	void SetRoadColor(const Ogre::Vector3& color) { roadColor = color; }
	void SetLockedOverlay(bool enabled) { showLockedOverlay = enabled; }
	
private:
	int texWidth = 512, texHeight = 512;
	Ogre::Vector3 bgColor{0.1f, 0.1f, 0.15f};    // Dark blue-grey
	Ogre::Vector3 roadColor{0.8f, 0.8f, 0.8f};   // Light grey
	Ogre::Vector3 lockedColor{0.3f, 0.3f, 0.3f}; // Darker grey
	bool showLockedOverlay = true;
	
	// Calculate track bounds
	void CalculateBounds(const std::vector<Ogre::Vector3>& points,
		Ogre::Vector3& min, Ogre::Vector3& max, Ogre::Vector3& center, float& scale);
	
	// Draw road on pixel buffer
	void DrawRoad(Ogre::PixelBox& pixels, const std::vector<Ogre::Vector3>& points,
		float scale, const Ogre::Vector3& offset);
	
	// Draw locked overlay
	void DrawLockedOverlay(Ogre::PixelBox& pixels);
	
	// Create texture from pixel buffer
	Ogre::TextureGpu* CreateTexture(const Ogre::PixelBox& pixels, const std::string& name);
	
	std::map<std::string, Ogre::TextureGpu*> textureCache;
};


/// 🎨 Track Preview Renderer
/// Renders 3D preview of track for FE
class TrackPreviewRenderer
{
public:
	TrackPreviewRenderer();
	~TrackPreviewRenderer();
	
	// Initialize renderer
	bool Initialize(Ogre::SceneManager* sceneMgr);
	void Shutdown();
	
	// Render track preview to texture
	Ogre::TextureGpu* RenderPreview(const TrackMetadata* track);
	
	// Set camera position for preview
	void SetCameraPosition(const Ogre::Vector3& pos, const Ogre::Vector3& target);
	
	// Settings
	void SetPreviewSize(int width, int height) { previewWidth = width; previewHeight = height; }
	
private:
	Ogre::SceneManager* sceneMgr = nullptr;
	Ogre::Camera* previewCamera = nullptr;
	Ogre::SceneNode* cameraNode = nullptr;
	
	int previewWidth = 800, previewHeight = 600;
	
	Ogre::TextureGpu* renderTexture = nullptr;
	
	// Setup preview camera
	void SetupCamera(const TrackMetadata* track);
	
	// Render to texture
	Ogre::TextureGpu* RenderToTexture();
};


/// 📋 Track Info Panel Data
/// Data structure for FE track info display
struct TrackInfoPanel
{
	std::string trackName;
	std::string region;
	std::string author;
	
	float trackLength = 0.0f;     // in meters
	int difficulty = 1;           // 1-5 stars
	int eventTypes = 0;           // bitmask
	
	std::string lengthStr;        // Formatted "2.5 km"
	std::string difficultyStr;    // Formatted "★★★☆☆"
	std::vector<std::string> eventIcons;
	
	// Format helpers
	static std::string FormatLength(float meters);
	static std::string FormatDifficulty(int stars);
	static std::vector<std::string> GetEventIcons(int eventTypes);
	
	// Populate from TrackMetadata
	void Populate(const TrackMetadata* track);
};
