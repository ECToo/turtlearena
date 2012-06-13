/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//
/*
=======================================================================

SETUP MENU

=======================================================================
*/


#include "ui_local.h"


#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_CUSTOMIZEPLAYER		10
#define ID_CUSTOMIZECONTROLS	11
#define ID_SYSTEMCONFIG			12
#define ID_GAME					13
#define ID_LOAD					14
#define ID_SAVE					15
#define ID_DEFAULTS				16
#define ID_BACK					17
#ifdef TURTLEARENA // LONG_CREDITS
#define ID_CREDITS				18
#endif
#ifdef TA_SP
#define ID_REPLAYS				19
#define ID_CINEMATICS			20
#define ID_MODS					21
#endif


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
#ifndef TA_MISC
	menutext_s		setupplayers;
#endif
	menutext_s		setupcontrols;
	menutext_s		setupsystem;
	menutext_s		game;
#ifdef TURTLEARENA // LONG_CREDITS
	menutext_s		credits;
#endif
#ifdef TA_SP
	menutext_s		replays;
//	menutext_s		cinematics;
	menutext_s		mods;
#endif
//	menutext_s		load;
//	menutext_s		save;
	menutext_s		defaults;
	menubitmap_s	back;
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;


/*
=================
Setup_ResetDefaults_Action
=================
*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
Setup_ResetDefaults_Draw
=================
*/
static void Setup_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset *ALL*", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "options to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
}


/*
===============
UI_SetupMenu_Event
===============
*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
#ifndef TA_MISC
	case ID_CUSTOMIZEPLAYER:
		UI_SelectPlayerMenu(UI_PlayerSettingsMenu, "PLAYER SETTINGS");
		break;
#endif

	case ID_CUSTOMIZECONTROLS:
		UI_SelectPlayerMenu(UI_ControlsMenu, "CONTROLS");
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;

#ifdef TURTLEARENA // LONG_CREDITS
	case ID_CREDITS:
		UI_LongCreditMenu();
		break;
#endif

#ifdef TA_SP
	case ID_REPLAYS:
		UI_DemosMenu();
		break;

//	case ID_CINEMATICS:
//		UI_CinematicsMenu();
//		break;

	case ID_MODS:
		UI_ModsMenu();
		break;
#endif

//	case ID_LOAD:
//		UI_LoadConfigMenu();
//		break;

//	case ID_SAVE:
//		UI_SaveConfigMenu();
//		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SetupMenu_Init
===============
*/
static void UI_SetupMenu_Init( void ) {
	int				y;

	UI_SetupMenu_Cache();

	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
#ifdef TA_SP // New menus
	setupMenuInfo.banner.string						= "OPTIONS";
#else
	setupMenuInfo.banner.string						= "SETUP";
#endif
	setupMenuInfo.banner.color						= text_banner_color;
	setupMenuInfo.banner.style						= UI_CENTER;

	setupMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framel.generic.name				= ART_FRAMEL;
	setupMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framel.generic.x					= 0;  
	setupMenuInfo.framel.generic.y					= 78;
	setupMenuInfo.framel.width  					= 256;
	setupMenuInfo.framel.height  					= 329;

	setupMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framer.generic.name				= ART_FRAMER;
	setupMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framer.generic.x					= 376;
	setupMenuInfo.framer.generic.y					= 76;
	setupMenuInfo.framer.width  					= 256;
	setupMenuInfo.framer.height  					= 334;

#ifdef TA_SP
	if( trap_Cvar_VariableValue( "cl_paused" ) )
		y = (480 - SETUP_MENU_VERTICAL_SPACING * 3) / 2;
	else
		y = (480 - SETUP_MENU_VERTICAL_SPACING * 7) / 2;
#else
	y = 134;
#endif
#ifndef TA_MISC
	setupMenuInfo.setupplayers.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupplayers.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupplayers.generic.x				= 320;
	setupMenuInfo.setupplayers.generic.y				= y;
	setupMenuInfo.setupplayers.generic.id			= ID_CUSTOMIZEPLAYER;
	setupMenuInfo.setupplayers.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupplayers.string				= "PLAYERS";
	setupMenuInfo.setupplayers.color					= text_big_color;
	setupMenuInfo.setupplayers.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
#endif
	setupMenuInfo.setupcontrols.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupcontrols.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupcontrols.generic.x			= 320;
	setupMenuInfo.setupcontrols.generic.y			= y;
	setupMenuInfo.setupcontrols.generic.id			= ID_CUSTOMIZECONTROLS;
	setupMenuInfo.setupcontrols.generic.callback	= UI_SetupMenu_Event; 
#ifdef TA_MISC
	setupMenuInfo.setupcontrols.string				= "Controls";
#else
	setupMenuInfo.setupcontrols.string				= "CONTROLS";
#endif
	setupMenuInfo.setupcontrols.color				= text_big_color;
	setupMenuInfo.setupcontrols.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 320;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
#ifdef TA_MISC
	setupMenuInfo.setupsystem.string				= "System";
#else
	setupMenuInfo.setupsystem.string				= "SYSTEM";
#endif
	setupMenuInfo.setupsystem.color					= text_big_color;
	setupMenuInfo.setupsystem.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.game.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.game.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.game.generic.x					= 320;
	setupMenuInfo.game.generic.y					= y;
	setupMenuInfo.game.generic.id					= ID_GAME;
	setupMenuInfo.game.generic.callback				= UI_SetupMenu_Event; 
#ifdef TA_MISC
	setupMenuInfo.game.string						= "Game Options";
#else
	setupMenuInfo.game.string						= "GAME OPTIONS";
#endif
	setupMenuInfo.game.color						= text_big_color;
	setupMenuInfo.game.style						= UI_CENTER;

	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
#ifdef TA_SP
		// Moved here from q3 main menu.
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.replays.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.replays.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.replays.generic.x					= 320;
		setupMenuInfo.replays.generic.y					= y;
		setupMenuInfo.replays.generic.id				= ID_REPLAYS;
		setupMenuInfo.replays.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.replays.string					= "Replays";
		setupMenuInfo.replays.color						= text_big_color;
		setupMenuInfo.replays.style						= UI_CENTER;

/*
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.cinematics.generic.type			= MTYPE_PTEXT;
		setupMenuInfo.cinematics.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.cinematics.generic.x				= 320;
		setupMenuInfo.cinematics.generic.y				= y;
		setupMenuInfo.cinematics.generic.id				= ID_CINEMATICS;
		setupMenuInfo.cinematics.generic.callback		= UI_SetupMenu_Event; 
		setupMenuInfo.cinematics.string					= "Cinematics";
		setupMenuInfo.cinematics.color					= text_big_color;
		setupMenuInfo.cinematics.style					= UI_CENTER;
*/

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.mods.generic.type		= MTYPE_PTEXT;
		setupMenuInfo.mods.generic.flags	= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.mods.generic.x		= 320;
		setupMenuInfo.mods.generic.y		= y;
		setupMenuInfo.mods.generic.id		= ID_MODS;
		setupMenuInfo.mods.generic.callback	= UI_SetupMenu_Event; 
		setupMenuInfo.mods.string			= "Mods";
		setupMenuInfo.mods.color			= text_big_color;
		setupMenuInfo.mods.style			= UI_CENTER;
#endif

#ifdef TURTLEARENA // LONG_CREDITS
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.credits.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.credits.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.credits.generic.x					= 320;
		setupMenuInfo.credits.generic.y					= y;
		setupMenuInfo.credits.generic.id				= ID_CREDITS;
		setupMenuInfo.credits.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.credits.string					= "Credits";
		setupMenuInfo.credits.color						= text_big_color;
		setupMenuInfo.credits.style						= UI_CENTER;
#endif

#if 0
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.load.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.load.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.load.generic.x					= 320;
		setupMenuInfo.load.generic.y					= y;
		setupMenuInfo.load.generic.id					= ID_LOAD;
		setupMenuInfo.load.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.load.string						= "LOAD";
		setupMenuInfo.load.color						= text_big_color;
		setupMenuInfo.load.style						= UI_CENTER;

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.save.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.save.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.save.generic.x					= 320;
		setupMenuInfo.save.generic.y					= y;
		setupMenuInfo.save.generic.id					= ID_SAVE;
		setupMenuInfo.save.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.save.string						= "SAVE";
		setupMenuInfo.save.color						= text_big_color;
		setupMenuInfo.save.style						= UI_CENTER;
#endif

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.defaults.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.defaults.generic.x				= 320;
		setupMenuInfo.defaults.generic.y				= y;
		setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
		setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
#ifdef TA_MISC
		setupMenuInfo.defaults.string					= "Defaults";
#else
		setupMenuInfo.defaults.string					= "DEFAULTS";
#endif
		setupMenuInfo.defaults.color					= text_big_color;
		setupMenuInfo.defaults.style					= UI_CENTER;
	}

	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.banner );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framer );
#ifndef TA_MISC
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupplayers );
#endif
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupcontrols );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsystem );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.game );
	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
#ifdef TA_SP
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.replays );
//		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.cinematics );
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.mods );
#endif
#ifdef TURTLEARENA // LONG_CREDITS
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.credits );
#endif
//		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.load );
//		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.save );
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
	}
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );
}


/*
=================
UI_SetupMenu_Cache
=================
*/
void UI_SetupMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_SetupMenu
===============
*/
void UI_SetupMenu( void ) {
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}
