#pragma once
#include "Gui_Def.h"
#include "HudRenderable.h"
#include "Replay.h"
#include "cardefs.h"
#include "par.h"

// #include "MessageBox.h"
// #include "MessageBoxStyle.h"
#include <OgreVector3.h>
// #include <bullet/LinearMath/btQuickprof.h>  // only for bullet debug text

// #include "networkcallbacks.hpp"
// #include <thread>


namespace Ogre {  class SceneNode;  class SceneManager;  class Viewport;  class ManualObject;  class Item;  }
class HlmsPbsDb2;
class App;  class SETTINGS;  class CGui;
class SplineRoad;  class CarModel;
class GraphView;   class HudRenderable;


class CHud : public BaseGui
{
public:
	App* app =0;
	SETTINGS* pSet =0;
	
	CHud(App* ap1);
		
	
	//  minimap pos triangle, all needed info
	struct SMiniPos
	{
		float x,y;  // pos
		float px[4],py[4];  // points, rotation
	};

	///  ⏱️ HUD  ------------
	class Hud  // for 1 viewport/player
	{
	public:
		//  ⏱️ times bar
		Img bckTimes =0;
		Txt txTimTxt =0, txTimes =0, txCollect =0;
		Ogre::String sTimes, sLap;
		//  🏁 lap results
		Txt txLapTxt =0, txLap =0;  Img bckLap =0;

		//  ❌ wrong check warning, 🥇 win place
		Txt txWarn =0,  txPlace =0;
		Img bckWarn =0, bckPlace =0;
		//  start countdown
		Txt txCountdown =0;

		//  ⏲️ gauges
		HudRenderable* hrGauges =0;
		Ogre::SceneNode *ndGauges =0;
		//  gear, vel
		Txt txVel =0, txGear =0, txAbs =0, txTcs =0;
		Img bckVel =0, bckGear =0;

		//  🔨 damage %, rewind time, boost fuel
		float dmgBlink = 0.f, dmgOld = 0.f;
		Img imgDamage =0;
		Txt txDamage =0,  txRewind =0,  txBFuel =0;
		Img icoDamage =0, icoRewind =0, icoBFuel =0, icoBInf =0;
		
		//  input bars  todo..
		//Img imgSteer =0, barSteer =0,  barThrottle =0, barBrake =0;
		//Txt txtOther =0;  //barHandBrake = 0, barBoost = 0, barRewind =0;

		//  🎥 current camera name
		Txt txCam =0;

		//  🌍 minimap
		HudRenderable *hrMap =0;
		Ogre::SceneNode *ndMap =0;
		//  all info for this car's minimap
		std::vector<SMiniPos> vMiniPos;  // const size: 6
		
		//  center position .
		Ogre::Vector2 vcRpm{0,0}, vcVel{0,0};  // -1..1
		Ogre::Real fScale = 0.19f;

		WP parent =0;
		Hud();
	};
	std::vector<Hud> hud;  // const size: MAX_Players
	// todo: one for all^, in splitscreen


	///  global Hud  ---------

	//  🌍🔺 car pos tris on minimap
	//  one for all cars on all viewports
	HudRenderable* hrPos =0;
	Ogre::SceneNode* ndPos =0;

	//  💬 chat messages
	Txt txMsg =0;  Img bckMsg =0;
	
	//  🎥 camera move info
	Txt txCamInfo =0;


	//  🔧 car debug texts
	Txt txDbgCar[4] ={0,0,0,0}; //, txDbgTxt =0, txDbgExt =0;
	Txt txDbgSurf =0, txDbgProfTim =0, txDbgProfBlt =0;

	//  Carbon HUD styling
	Ogre::ColourValue carbonAccent{0.f, 0.8f, 1.f};  // Cyan accent
	Ogre::ColourValue carbonOrange{1.f, 0.5f, 0.f};  // Orange for warnings
	Ogre::ColourValue carbonBg{0.1f, 0.1f, 0.15f, 0.7f};  // Dark translucent bg
	bool carbonStyle = false;

	constexpr static int MAX_TireVis = 4;
	Ogre::SceneNode *ndTireVis[MAX_TireVis] ={0,0,0,0,};
	HudRenderable   *hrTireVis[MAX_TireVis] ={0,0,0,0,};


	///  🔝 Hud3d  general mesh in 3d but on hud
	//  arrows to next checkpoint, collect gems, win cups etc
	class Hud3d
	{
		int player = 0;
		Ogre::Quaternion qStart, qEnd, qCur;  // smooth animation
		HlmsPbsDb2* pDb =0;

		float dist = 0.f;  // cup anim
		Ogre::Degree yaw{0.f};
	public:
		Ogre::Item* it =0;  // 🟢 ogre
		Ogre::SceneNode* node =0, *nodeRot =0;
		Ogre::Vector3 posTo;  // dir to collect

		void Create(Ogre::SceneManager* mgr, SETTINGS* pSet, int plr,
			float scale = 0.f, Ogre::String mesh="", Ogre::String mtr="");
		void Destroy(Ogre::SceneManager* mgr);

		void UpdateChk(SplineRoad* road, CarModel* carM, const Ogre::Vector3& pos);
		void Update(CarModel* carM, float time);
		void UpdateCol(CarModel* carM, float sc);
		void UpdateCup(CarModel* carM, float time), ShowCup(), Hide();
	}
	arrChk[MAX_Players], arrCol[MAX_Players][MAX_ArrCol],
	cup[3];
		
	float asp =1.f, scX =1.f, scY =1.f,
		minX =0.f, maxX =0.f, minY =0.f, maxY =0.f;  // minimap visible range

	//------------------------------------------

	//  🆕 init
	void Create(), Destroy();

	//  🗜️ show, size
	void Size(), Show(bool hideAll=false); //, ShowVp(bool vp);

	///  💫 update
	void Update(int carId, float time);
	//  update internal
	void UpdPosElems(int cnt, int cntC, int carId),
		 UpdRotElems(int baseCarId, int carId, float vel, float rpm),  // rpm < 0.f to hide
		 GetCarVals(int id, float* vel, float* rpm, float* clutch, int* gear),
		 UpdMiniTer(), //UpdDbgTxtClr(),
		 UpdMultiplayer(CGui* gui, int cnt, float time),
		 //UpdOpponents(Hud& h, int cnt, CarModel* pCarM),
		 //UpdMotBlur(CAR* pCar, float time),
		 UpdCarTexts(int carId, Hud& h, float time, CAR* pCar),
		 UpdTimes(int carId, Hud& h, float time, CAR* pCar, CarModel* pCarM),
		 UpdDebug(CAR* pCar, CarModel* pCarM);


	//  ⛓️ util
	float getHudScale();
	Txt CreateNickText(int carId, Ogre::String text);
	Ogre::Vector3 projectPoint(const Ogre::Camera* cam, const Ogre::Vector3& pos);  // 2d xy, z - out info

	//  string utils
	static Ogre::String StrClr(Ogre::ColourValue c);
	static MyGUI::Colour GetVelClr(float vel);

	//  Carbon HUD methods
	void SetCarbonStyle(bool enable);
	void UpdateCarbonGauges(Hud& h, float vel, float rpm, int gear);

#ifndef BT_NO_PROFILE
	//  bullet debug text
	void bltDumpRecursive(class CProfileIterator* profileIterator, int spacing, std::stringstream& os);
	void bltDumpAll(std::stringstream& os);
#endif // BT_NO_PROFILE
};
