#pragma once
#include "configfile.h"
#include "settings_com.h"
#include "PaintsIni.h"
#include "protocol.hpp"


enum eGraphType  {
	Gh_Fps=0,   // 📊
	Gh_CarAccelG,  // 🚗
	Gh_CamBounce,  // 🎥
	Gh_BulletHit,  // ✨
	Gh_Sound,   // 🔊
	Gh_Checks,  // 🔵
	Gh_Suspension, // 🪜
	Gh_TireSlips,  // ⚫
	Gh_TireEdit, Gh_Tires4Edit, // ⚫📉
	Gh_Diffs,
	Gh_TorqueCurve, Gh_Engine,  // 📈
	Gh_Clutch,
	Gh_ALL  };  // total count
const static std::string csGraphNames[Gh_ALL] = {
	"Fps graphics perf.", "Car Accel G's",
	"Camera bounce", "Car Hit chassis",
	"Sound info, sources",
	"Checkpoints",
	"Suspension pos & vel",
	"Tires slip| & slide-",
	"Tire Edit (Pacejka coeffs)*", "All Tires Pacejka vis and edit*",
	"Differentials",
	"Torque Curve, gears", "Engine torque & power",
	"Clutch, Rpm, Gear" };


enum EMenu
{	MN1_Main, MN1_Setup, MN1_Games,  // small main menus
	MN_Single, MN_Tutorial, MN_Champ, MN_Chall,  // game, same window
	MN_Collect, MN_Career,
	MN_HowTo, MN_Replays,  // other windows
	MN_Help, MN_Options, MN_Materials,   // common
	MN_NoCng  // fake for GuiShortcut
};


class SETTINGS : public SETcom
{
public:
//------------------------------------------
	int version =100;  // file version =

	//  🪧 menu
	int iMenu;  // EMenu,
	int yMain =0, ySetup =0, yGames =0;  // kbd up/dn cursors
	int difficulty =0;

	//  ✅ hud show
	bool show_gauges =1, show_digits =1, // ⏲️
		//  🌍 minimap
		trackmap =1, mini_zoomed =1, mini_rotated =1,
		mini_terrain =1, mini_border =1,
		check_beam =1, check_arrow =0,  // 🥛🔝
		show_times =1,  // ⏱️
		ch_all =0,  // show all champs/challs/collect
		// show_opponents =0, opplist_sort =0,

	//  🔧 hud tweak
		car_tirevis =0, // car_dbgbars =0,
		car_dbgtxt =0, car_dbgsurf =0,  // 🗒️
		show_graphs =0;  // 📉
	//  🎚️ sizes
	float size_gauges = 0.19f, size_minimap = 0.2f;
	float size_minipos = 0.1f, size_arrow = 0.2f, zoom_minimap = 4.f;
	int gauges_type = 1; //, gauges_layout = 1;

	//  🎥 camera
	bool show_cam =1, cam_tilt =1;  // info
	bool cam_loop_chng =1;  int cam_in_loop = 1;
	bool cam_bounce =1;  float cam_bnc_mul = 1.f;
	float fov_min = 90.f, fov_boost = 5.f, fov_smooth = 5.f;  // fov
	//  🚦 pacenotes
	bool pace_show =1;  int pace_next = 4;

	//  dbg
	eGraphType graphs_type = Gh_Fps;
	int car_dbgtxtclr = 1, car_dbgtxtcnt = 0;
	bool sounds_info =0;

	//  🎛️ gui, games
	bool cars_sortup =1;  int cars_view = 0, cars_sort = 1;
	int champ_type = 0, chall_type = 0, collect_type = 0;
	bool champ_info =1;
	int car_ed_tab = 0, tweak_tab = 0;  // gui only
	int car_clr = -1;  // paints.ini id
	int paint_filter = 0;  // low

	//  🪟 font, hud
	float font_hud = 1.f;  // scales
	float font_times = 1.f;

	//  📊 graphics
	bool bFog =1;  // always on

	bool particles =1, trails =1;
	float particles_len = 1.5f, trails_len = 1.f;
	bool boost_fov =1;


	//---------------  car setup
	bool abs = 0, tcs = 0,  // meh
		autoshift = 1, autorear = 1, rear_inv = 1, show_mph = 0;
	float sss_effect[3] = {0.574f, 0.65f, 0.3f},
		sss_velfactor[3] = {0.626f, 0.734f, 0.3f};
	//  steering range multipliers
	float steer_range[3] = {1.0, 0.76, 0.9},  //  0 gravel  1 asphalt  2 hovers
			steer_sim[2] = {0.65, 0.90};  // simulation modes  0 easy  1 normal
	std::vector<int> cam_view;  // [MAX_Players]

	//---------------  game config
	class GameSet
	{
	public:
		std::string track{"Test1-Flat"};  bool track_user =0;  // 🏞️
		bool track_reversed =0;
		float trees = 1.5f;  // 🌳🪨 veget common
		float bushes = 1.f;

		std::vector<std::string> car;  // [MAX_Players]   local players
		std::vector<CarPaint> clr;     // [MAX_Vehicles]  also for ghosts 🎨  own paint.cfg

		bool vr_mode =0;  // not used, copy in game from pSet->
		int local_players = 1, local_bots = 2, num_laps = 2;  // 👥 split + 🤖 bots
		//  🔨 game setup
		std::string sim_mode{"normal"};
		bool collis_veget =1, collis_cars =1, collis_roadw =0, dyn_objects =1, drive_horiz =0;
		
		int boost_type = 3, flip_type = 2, damage_type = 1, rewind_type = 1;
		float damage_dec = 0.4f;
		float boost_power =1.f, boost_max =6.f, boost_min =2.f, boost_per_km =1.f, boost_add_sec =0.1f;
		void BoostDefault();  // 💨

		bool rpl_rec =1;  // 📽️
		//  🏆 champ
		int champ_num = -1, chall_num = -1;  // -1 none
		bool champ_rev =0;
		int collect_num = -1, collect_all = 0;

		float pre_time = 2.f;  int start_order = 0;

		//  ctor
		GameSet();
		bool hasLaps()
		{	return local_players > 1 || /*|| app->mClient*/
				champ_num >= 0 || chall_num >= 0;
		}
	}  game,  // current game, changed only on new game start
		gui;  // gui only config
	//---------------


	//  ⚙️ startup, other
	bool dev_keys = 0, dev_no_prvs = 0;  // dev
	bool split_vertically = 1;

	bool bltDebug = 0, bltLines = 0, bltProfilerTxt = 0, profilerTxt = 0;
	bool loadingbackground = 1, show_welcome = 1;
	bool paintAdj = 0;


	//  💫 sim freq  (1/interval timestep)
	float game_fq = 160.f, blt_fq = 160.f;
	int blt_iter = 24, dyn_iter = 60;
	// guicom::g.sim_quality, sets above from presets
	
	float perf_speed = 100000.f;  // other
	int thread_sleep = 5; //, gui_sleep;

	//  ⚫📉 tire graphs vis
	float tc_r = 6000.f, tc_xr = 1.f;  // tire circles max
	float te_yf = 8000.f, te_xfx = 12.f, te_xfy = 160.f, te_xf_pow = 1.f;  // tire edit max
	bool te_reference = 0, te_common = 1;

	//  🔧 tweak
	std::map<char, std::string> dev_tracks;  // alt-shift-

	//  📽️ replay
	bool rpl_rec = 1, rpl_ghost = 1, rpl_bestonly = 1;
	bool rpl_ghostother = 1, rpl_trackghost = 1;
	bool rpl_ghostpar = 0, rpl_ghostrewind = 1, rpl_listghosts = 0;
	bool rpl_hideHudAids = 0;
	int rpl_listview = 0, rpl_numViews = 4;
	float ghoHideDist = 5.f, ghoHideDistTrk = 5.f;  // ghost hide dist, when close

	//  📡 network
	std::string nickname{"Player"}, netGameName{"Default Game"};
	std::string master_server_address{""}, connect_address{"localhost"};
	int master_server_port = protocol::DEFAULT_PORT,
		local_port = protocol::DEFAULT_PORT, connect_port = protocol::DEFAULT_PORT;

	int net_local_plr = -1;  // not in gui


//------------------------------------------
	SETTINGS();

	template <typename T>
	bool Param(CONFIGFILE & conf, bool write, std::string pname, T & value)
	{
		if (write)
		{	conf.SetParam(pname, value);
			return true;
		}else
			return conf.GetParam(pname, value);
	}
	void Serialize(bool write, CONFIGFILE & config);
	void SerPaints(bool write, CONFIGFILE & config);
	void Load(std::string sfile), Save(std::string sfile);
};
