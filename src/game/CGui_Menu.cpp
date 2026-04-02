#include "pch.h"
#include "Gui_Def.h"
#include "BaseApp.h"
#include "CGame.h"
#include "CHud.h"
#include "CGui.h"
#include "GuiCom.h"
#include "CScene.h"
#include "settings.h"
#include "Career.h"
#include "EventMode.h"

#include <OgreRoot.h>
#include <OgreOverlay.h>
#include "MultiList2.h"
#include "Slider.h"
#include "Gui_Popup.h"

#include <OgreRoot.h>
#include <OgreWindow.h>
#include <OgreOverlay.h>
#include <MyGUI.h>
using namespace MyGUI;
using namespace Ogre;
using namespace std;


//  🌟 ctor
CGui::CGui(App* app1)
{
	app = app1;
	pSet = app1->pSet;
	sc = app1->scn->sc;
	pGame = app1->pGame;
	hud = app1->hud;
	data = app1->scn->data;

	int i;
	for (i=0; i < iCarSt; ++i)
	{	txCarStTxt[i]=0;  txCarStVals[i]=0;  barCarSt[i]=0;  }

	for (i=0; i < ciEdCar; ++i)
		edCar[i] = 0;
}

CGui::~CGui()
{
}


//  🪧 Main menu
//----------------------------------------------------------------------------------------------------------------
void CGui::InitMainMenu()
{
	Btn btn;  int i;
	for (i=0; i < ciMainBtns; ++i)
	{
		auto s = toStr(i);
		app->mMainPanels[i] = fImg("PanMenu"+s);
		Btn("BtnMenu"+s, btnMainMenu);  app->mMainBtns[i] = btn;
	}
	// Career button (BtnMenu4)
	app->mMainPanels[4] = fImg("PanMenu4");
	Btn("BtnMenu4", btnMainMenu);  app->mMainBtns[4] = btn;
	
	for (i=0; i < ciSetupBtns; ++i)
	{
		auto s = toStr(i);
		app->mMainSetupPanels[i] = fImg("PanSetup"+s);
		Btn("BtnSetup"+s, btnMainMenu);  app->mMainSetupBtns[i] = btn;
	}
	for (i=0; i < ciGamesBtns; ++i)
	{
		auto s = toStr(i);
		app->mMainGamesPanels[i] = fImg("PanGames"+s);
		Btn("BtnGames"+s, btnMainMenu);  app->mMainGamesBtns[i] = btn;
	}
	// Career button in games (BtnGames7) - disabled
	// app->mMainGamesPanels[7] = fImg("PanGames7");
	// Btn("BtnGames7", btnMainMenu);  app->mMainGamesBtns[7] = btn;

	//  center
	int wx = app->mWindow->getWidth(), wy = app->mWindow->getHeight();

	Wnd wnd = app->mWMainMenu;  IntSize w = wnd->getSize();
	wnd->setPosition((wx-w.width)*0.5f, (wy-w.height)*0.5f);

	wnd = app->mWMainSetup;  w = wnd->getSize();
	wnd->setPosition((wx-w.width)*0.5f, (wy-w.height)*0.5f);

	wnd = app->mWMainGames;  w = wnd->getSize();
	wnd->setPosition((wx-w.width)*0.5f, (wy-w.height)*0.5f);

	// Center Career window
	wnd = app->mWCareer;  if (wnd) { w = wnd->getSize();
		wnd->setPosition((wx-w.width)*0.5f, (wy-w.height)*0.5f); }

	// Career back button
	Btn btnCareerBack = fBtn("BtnCareerBack");
	if (btnCareerBack)
		btnCareerBack->eventMouseButtonClick += newDelegate(this, &CGui::btnCareerBack);

	Btn btnCareerStart = fBtn("BtnStartCareer");
	if (btnCareerStart)
		btnCareerStart->eventMouseButtonClick += newDelegate(this, &CGui::btnCareerStart);

	for (i=0; i < 10; ++i)
	{
		Btn btnDistrict = fBtn("BtnDistrict" + toStr(i));
		if (btnDistrict)
			btnDistrict->eventMouseButtonClick += newDelegate(this, &CGui::btnCareerDistrict);
	}


	//  Difficulty  ---
	Cmb(diffList, "DiffList", comboDiff);

	auto add = [&](int diff)
	{
		auto clr = gcom->getClrDiff(diff);
		diffList->addItem(clr+ TR("#{Diff"+toStr(diff)+"}"));
	};
	add(1);  add(2);  add(3);  add(4);  add(5);  add(6);  add(7);
	diffList->setIndexSelected(pSet->difficulty);

	//  Simulation  ---
	Cmb(simList, "SimList", comboSim);
	auto sim = [&](int clrDiff, int diff, String s)
	{
		auto clr = gcom->getClrDiff(clrDiff);
		simList->addItem(clr+ TR("#{Diff"+toStr(diff)+"}"));

		StringUtil::toLowerCase(s);
		if (pSet->gui.sim_mode == s)
			simList->setIndexSelected(simList->getItemCount()-1);
	};
	sim(1,2,"Easy");  sim(4,3,"Normal");
	//sim(6,4,"Hard");  // WIP test .. car HI
}


//  🏆 Career Window
//----------------------------------------------------------------------------------------------------------------
void CGui::ShowCareerWnd()
{
	auto& career = CareerManager::Get();

	// Update career info
	Ed edInfo = fEd("edCareerInfo");
	if (edInfo)
	{
		std::string info = "Career mode\n\n";
		info += "Select a district, then launch one of its unlocked events.";
		edInfo->setCaption(info);
	}

	// Update player stats
	Ed edStats = fEd("edCareerStats");
	if (edStats)
	{
		std::string stats;
		stats += "Level: " + toStr(career.GetLevel()) + "\n";
		stats += "Reputation: " + toStr(career.GetProgress().reputation) + "\n";
		stats += "Cash: $" + toStr(career.GetProgress().cash) + "\n";
		stats += "\nBosses defeated: " + toStr(career.GetProgress().bossesDefeated) + "/10\n";
		stats += "Districts complete: " + toStr(0) + "/10";  // TODO: count complete districts
		edStats->setCaption(stats);
	}

	// Update district buttons
	for (int i = 0; i < 10; ++i)
	{
		Btn btn = fBtn("BtnDistrict" + toStr(i));
		if (btn)
		{
			const District* district = career.GetDistrict(i + 1);
			if (district)
			{
				bool unlocked = career.CanAccessDistrict(district->id);
				btn->setEnabled(unlocked);
				btn->setCaption(toStr(i + 1) + ". " + district->name);

				// Color based on completion
				if (unlocked)
				{
					bool complete = career.IsDistrictComplete(district->id);
					if (complete)
						btn->setTextColour(Colour(0.6, 1.0, 0.6));  // Green
					else
						btn->setTextColour(Colour(1.0, 1.0, 0.6));  // Yellow
				}
				else
				{
					btn->setTextColour(Colour(0.6, 0.6, 0.6));  // Gray
				}
			}
		}
	}

	if (!career.CanAccessDistrict(selectedCareerDistrict))
	{
		selectedCareerDistrict = 1;
		for (int districtId = 1; districtId <= 10; ++districtId)
		{
			if (career.CanAccessDistrict(districtId))
			{
				selectedCareerDistrict = districtId;
				break;
			}
		}
	}

	UpdateCareerSelection(selectedCareerDistrict);
}

//  🔙 Career Back button
//----------------------------------------------------------------------------------------------------------------
void CGui::btnCareerBack(WP)
{
	pSet->iMenu = MN1_Main;
	app->gui->toggleGui(false);
}

void CGui::btnCareerDistrict(WP wp)
{
	if (!wp)
		return;

	std::string name = wp->getName().c_str();
	auto pos = name.find("BtnDistrict");
	if (pos == std::string::npos)
		return;

	int districtIndex = s2i(name.substr(pos + 11));
	UpdateCareerSelection(districtIndex + 1);
}

void CGui::UpdateCareerSelection(int districtId)
{
	auto& career = CareerManager::Get();
	const District* district = career.GetDistrict(districtId);
	if (!district)
		return;

	selectedCareerDistrict = districtId;
	selectedCareerEvent.clear();

	auto events = career.GetDistrictEvents(districtId);
	if (events.empty())
		events = district->events;
	if (!events.empty())
		selectedCareerEvent = events.front();

	Ed edInfo = fEd("edCareerInfo");
	if (edInfo)
	{
		std::string info = district->name + "\n";
		info += district->description + "\n\n";
		info += "Events:\n";
		for (const auto& eventId : events)
		{
			const EventConfig* cfg = EventManager::Get().GetEventConfig(eventId);
			if (cfg)
			{
				info += " - " + cfg->name + " [" + cfg->trackId + "]";
				if (eventId == selectedCareerEvent)
					info += "  <Selected>";
				info += "\n";
			}
			else
			{
				info += " - " + eventId + "\n";
			}
		}

		if (events.empty())
			info += " - No unlocked events\n";

		edInfo->setCaption(info);
	}

	Btn btnStart = fBtn("BtnStartCareer");
	if (btnStart)
	{
		bool canStart = false;
		std::string caption = "Start Career";
		if (!selectedCareerEvent.empty())
		{
			const EventConfig* cfg = EventManager::Get().GetEventConfig(selectedCareerEvent);
			if (cfg)
			{
				canStart = true;
				caption = "Start: " + cfg->name;
			}
		}
		btnStart->setEnabled(canStart);
		btnStart->setCaption(caption);
	}
}

void CGui::btnCareerStart(WP)
{
	if (selectedCareerEvent.empty())
		return;

	const EventConfig* cfg = EventManager::Get().GetEventConfig(selectedCareerEvent);
	if (!cfg || cfg->trackId.empty())
		return;

	pSet->gui.track = cfg->trackId;
	pSet->gui.track_user = false;
	pSet->gui.track_reversed = cfg->trackReversed;
	gcom->sListTrack = cfg->trackId;
	gcom->bListTrackU = 0;
	btnNewGame(0);
}


//  🔁 btn main
void CGui::btnMainMenu(WP wp)
{
	for (int i=0; i < ciMainBtns; ++i)
	if (wp == app->mMainBtns[i])
	{	switch (i)
		{
		case Menu_Setup:    pSet->iMenu = MN1_Setup;  break;
		case Menu_Replays:  pSet->iMenu = MN_Replays;  break;
		case Menu_Help:     pSet->iMenu = MN_Help;  break;
		case Menu_Options:  pSet->iMenu = MN_Options;  break;
		case Menu_Career:   pSet->iMenu = MN_Career;  ShowCareerWnd();  break;  // NEW Career
		}
		app->gui->toggleGui(false);
		return;
	}
	for (int i=0; i < ciSetupBtns; ++i)
	if (wp == app->mMainSetupBtns[i])
	{	switch (i)
		{
		case Setup_Games:      pSet->iMenu = MN1_Games;  break;
		case Setup_HowToPlay:  pSet->iMenu = MN_HowTo;  break;
		case Setup_Back:       pSet->iMenu = MN1_Main;  break;
		}
		app->gui->toggleGui(false);
		return;
	}
	for (int i=0; i < ciGamesBtns; ++i)
	if (wp == app->mMainGamesBtns[i])
	{	switch (i)
		{
		case Games_Single:       GuiShortcut(MN_Single, TAB_Track, -1, i);  return;
		case Games_SplitScreen:  GuiShortcut(MN_Single, TAB_Split, -1, i);  return;  // 👥
		case Games_Multiplayer:  GuiShortcut(MN_Single, TAB_Multi, -1, i);  return;  // 📡

		case Games_Tutorial:   GuiShortcut(MN_Tutorial, TAB_Champs, -1, i);  return;
		case Games_Champ:      GuiShortcut(MN_Champ,    TAB_Champs, -1, i);  return;
		case Games_Challenge:  GuiShortcut(MN_Chall,    TAB_Champs, -1, i);  return;

		case Games_Collection: GuiShortcut(MN_Collect, TAB_Champs, -1, i);  break;
		// Games_Career is disabled - Career is only accessible from main menu

		case Games_Stats:      app->mWndStats->setVisible(true);  break;
		case Games_Back:       pSet->iMenu = MN1_Setup;  break;
		}
		app->gui->toggleGui(false);
		return;
	}
}

void CGui::tabMainMenu(Tab tab, size_t id)
{
	//_  game tab change
	if (tab == app->mTabsGame)
	{
		if (id == TAB_Car)
			app->gui->CarListUpd();  //  off filtering by chall
	
		app->mWndTrkFilt->setVisible(false);  //

		/*if (id == TAB_Multi)
		{	//  back to mplr tab, upload game info
									//_ only for host..
			if (app->mMasterClient && app->gui->valNetPassword->getVisible())
			{	app->gui->uploadGameInfo();
				app->gui->updateGameInfoGUI();
			}
			//- app->gui->evBtnNetRefresh(0);  // upd games list (don't, breaks game start)
		}*/
	}
	
	if (id != 0)  return;  // <back
	tab->setIndexSelected(1);  // dont switch to 0

	// Career is a separate window, go back to main menu
	if (pSet->iMenu == MN_Career)
		pSet->iMenu = MN1_Main;
	else if (pSet->iMenu >= MN_Single && pSet->iMenu <= MN_Career)
		pSet->iMenu = MN1_Games;
	else
		pSet->iMenu = MN1_Main;
	app->gui->toggleGui(false);  // back to main
}


//  🔁⚫ Game Simulation change  Race menu
//---------------------------------------------------------
void CGui::comboSim(Cmb cmb, size_t val)
{
	int damage = 0;
	switch (val)
	{
	case 0:  pSet->gui.sim_mode = "easy";    damage = 1;  break;
	case 1:  pSet->gui.sim_mode = "normal";  damage = 2;  break;
	case 2:  pSet->gui.sim_mode = "hard";    damage = 2;  break;
	}
	//  game
	pSet->gui.damage_type = damage;  cmbDamage->setIndexSelected(damage);

	bReloadSim = true;
	tabTireSet(0,iTireSet);  listCarChng(carList,0);
}


//  🔁🚦 Game Difficulty change  Race menu
//----------------------------------------------------------------------------------------------------------------
void CGui::comboDiff(Cmb cmb, size_t val)
{
	LogO("+ comboDiff" + toStr(val));
	pSet->difficulty = val;

	auto resetFilter = [&]()
	{
		for (int i=0; i < COL_FIL; ++i)
		{	pSet->col_fil[0][i] = pSet->colFilDef[0][i];
			pSet->col_fil[1][i] = pSet->colFilDef[1][i];
	}	};

	auto SetDiff = [&](bool sortUp, int sortCol,  bool filter, int diffMax,
		int pipes, int jumps, int len,
		bool beam, bool arrow, bool trail, bool minimap,
		string car, string track)
	{
		//  tracks
		gcom->trkList->mSortColumnIndex = pSet->tracks_sort = sortCol;
		gcom->trkList->mSortUp = pSet->tracks_sortup = sortUp;  gcom->trkList->mSortUpOld = !sortUp;
		pSet->tracks_filter = filter;  resetFilter();  gcom->ckTrkFilter.SetValue(filter);

		pSet->col_fil[1][1] = diffMax;  gcom->svTrkFilMax[1].SetValueI(diffMax);  // upd filt wnd gui
		pSet->col_fil[1][7] = jumps;  gcom->svTrkFilMax[7].SetValueI(jumps);
		pSet->col_fil[1][9] = pipes;  gcom->svTrkFilMax[9].SetValueI(pipes);
		pSet->col_fil[1][13] = len;  gcom->svTrkFilMax[13].SetValueI(len);
		//  trk, car
		pSet->gui.track = track;
		pSet->gui.car[0] = car;
		for (int i=0; i < carList->getItemCount(); ++i)
			if (carList->getItemNameAt(i).substr(7) == car)
				carList->setIndexSelected(i);
		//  hud
		ckBeam.SetValue(beam);  ckArrow.SetValue(arrow);
		ckTrailShow.SetValue(trail);  ckMinimap.SetValue(minimap);
	};

	const char D = SETcom::colFilDef[1][1], L = SETcom::colFilDef[1][13];  // max diff, len
	switch (val)
	{// up,col, filt,diff, pipes,jmp,len  bm,ar,tr,mi  car,trk
	case 0:  SetDiff(1,6,  1,2, 0,0, 3,  1,1,1,1, "V2", "Isl2-Sandy");  break;
	case 1:  SetDiff(1,6,  1,3, 1,1, 6,  0,1,1,1, "ES", "Isl12-Beach");  break;  //Isl5-Shore
	case 2:  SetDiff(1,6,  1,4, 2,2, 9,  0,0,1,1, "HI", "Jng6-Fun");  break;  // Isl6-Flooded
	case 3:  SetDiff(0,3,  1,5, 4,4,14,  0,0,1,1, "HI", "Isl14-Ocean");  break;
	case 4:  SetDiff(0,3,  0,6, 4,4,21,  0,0,0,1, "SX", "Grc9-Oasis");  break;
	case 5:  SetDiff(0,17, 0,6, 4,4, L,  0,0,0,0, "SX", "Mos5-Factory");  break;  // Isl17-AdapterIslands
	case 6:  SetDiff(0,17, 0,D, 4,4, L,  0,0,0,0, "U6", "Uni7-GlassStairs");  break;
	}
	app->mMainGamesBtns[Games_Tutorial]->setVisible(val < 4);  // tutorials hide
	gcom->TrackListUpd(true);  gcom->listTrackChng(gcom->trkList,0);
	listCarChng(carList,0);

	ChampsListUpdate();
	ChallsListUpdate();
}
