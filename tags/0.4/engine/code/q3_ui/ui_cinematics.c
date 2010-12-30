/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"


#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define VERTICAL_SPACING	30

#define ID_BACK			10
#define ID_CIN_IDLOGO	11
#define ID_CIN_INTRO	12
#ifndef TA_SP // LESS_VIDEOS
#define ID_CIN_TIER1	13
#define ID_CIN_TIER2	14
#define ID_CIN_TIER3	15
#define ID_CIN_TIER4	16
#define ID_CIN_TIER5	17
#define ID_CIN_TIER6	18
#define ID_CIN_TIER7	19
#define ID_CIN_END		20
#endif


typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		cin_idlogo;
	menutext_s		cin_intro;
#ifndef TA_SP // LESS_VIDEOS
	menutext_s		cin_tier1;
	menutext_s		cin_tier2;
	menutext_s		cin_tier3;
	menutext_s		cin_tier4;
	menutext_s		cin_tier5;
	menutext_s		cin_tier6;
	menutext_s		cin_tier7;
	menutext_s		cin_end;
#endif
	menubitmap_s	back;
} cinematicsMenuInfo_t;

static cinematicsMenuInfo_t	cinematicsMenuInfo;

static char *cinematics[] = {
	"idlogo",
	"intro",
#ifndef TA_SP // LESS_VIDEOS
	"tier1",
	"tier2",
	"tier3",
	"tier4",
	"tier5",
	"tier6",
	"tier7",
	"end"
#endif
};

/*
===============
UI_CinematicsMenu_BackEvent
===============
*/
static void UI_CinematicsMenu_BackEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	UI_PopMenu();
}


/*
===============
UI_CinematicsMenu_Event
===============
*/
static void UI_CinematicsMenu_Event( void *ptr, int event ) {
	int		n;

	if (event != QM_ACTIVATED)
		return;

	n = ((menucommon_s*)ptr)->id - ID_CIN_IDLOGO;
	trap_Cvar_Set( "nextmap", va( "ui_cinematics %i", n ) );
#ifndef TA_SP // LESS_VIDEOS
	if( uis.demoversion && ((menucommon_s*)ptr)->id == ID_CIN_END ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; cinematic demoEnd.RoQ 1\n" );
	}
	else
#endif
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "disconnect; cinematic %s.RoQ\n", cinematics[n] ) );
	}
}

#ifdef IOQ3ZTM
// Also see CIN_TheCheckExtension
qboolean UI_CanShowVideo(const char *video)
{
	enum
	{
		CIN_RoQ,
		CIN_roq,
		CIN_ogm,
		CIN_ogv,
		CIN_MAX
	};
	const char cin_ext[CIN_MAX][4] = { "RoQ\0", "roq\0", "ogm\0", "ogv\0" };
	qboolean skipCin[CIN_MAX] = { qfalse, qfalse , qfalse, qfalse };
	fileHandle_t hnd;
	char filename[MAX_QPATH];
	char fn[MAX_QPATH];
	int stringlen;
	char *extptr;
	int i;
	int len;

	Q_snprintf(filename, MAX_QPATH, "video/%s.RoQ", video);
	stringlen = strlen(filename);

	strncpy(fn, filename, stringlen+1);
	extptr = Q_strrchr(fn, '.');

	len = trap_FS_FOpenFile( fn, &hnd, FS_READ );

	if (len <= 0)
	{
		extptr++;
		for (i = 0; i < CIN_MAX; i++)
		{
			if (!strcmp(extptr, cin_ext[i]))
			{
				skipCin[i] = qtrue;
				break;
			}
		}

		for (i = 0; i < CIN_MAX; i++)
		{
			if (skipCin[i]) {
				continue;
			}

			extptr[0] = cin_ext[i][0];
			extptr[1] = cin_ext[i][1];
			extptr[2] = cin_ext[i][2];
			extptr[3] = '\0';

			len = trap_FS_FOpenFile( fn, &hnd, FS_READ );

			if (len > 0) {
				break;
			}
		}

		if (len <= 0) {
			return qfalse;
		}
	}

	trap_FS_FCloseFile(hnd);
	return qtrue;
}
#endif

/*
===============
UI_CinematicsMenu_Init
===============
*/
static void UI_CinematicsMenu_Init( void ) {
	int		y;

	UI_CinematicsMenu_Cache();

	memset( &cinematicsMenuInfo, 0, sizeof(cinematicsMenuInfo) );
	cinematicsMenuInfo.menu.fullscreen = qtrue;

	cinematicsMenuInfo.banner.generic.type		= MTYPE_BTEXT;
	cinematicsMenuInfo.banner.generic.x			= 320;
	cinematicsMenuInfo.banner.generic.y			= 16;
	cinematicsMenuInfo.banner.string			= "CINEMATICS";
	cinematicsMenuInfo.banner.color				= text_banner_color;
	cinematicsMenuInfo.banner.style				= UI_CENTER;

	cinematicsMenuInfo.framel.generic.type		= MTYPE_BITMAP;
	cinematicsMenuInfo.framel.generic.name		= ART_FRAMEL;
	cinematicsMenuInfo.framel.generic.flags		= QMF_INACTIVE;
	cinematicsMenuInfo.framel.generic.x			= 0;  
	cinematicsMenuInfo.framel.generic.y			= 78;
	cinematicsMenuInfo.framel.width  			= 256;
	cinematicsMenuInfo.framel.height  			= 329;

	cinematicsMenuInfo.framer.generic.type		= MTYPE_BITMAP;
	cinematicsMenuInfo.framer.generic.name		= ART_FRAMER;
	cinematicsMenuInfo.framer.generic.flags		= QMF_INACTIVE;
	cinematicsMenuInfo.framer.generic.x			= 376;
	cinematicsMenuInfo.framer.generic.y			= 76;
	cinematicsMenuInfo.framer.width  			= 256;
	cinematicsMenuInfo.framer.height  			= 334;

#ifdef TA_SP // LESS_VIDEOS
	y = 100 + 4 * VERTICAL_SPACING;
#else
	y = 100;
#endif
	cinematicsMenuInfo.cin_idlogo.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_idlogo.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_idlogo.generic.x			= 320;
	cinematicsMenuInfo.cin_idlogo.generic.y			= y;
	cinematicsMenuInfo.cin_idlogo.generic.id		= ID_CIN_IDLOGO;
	cinematicsMenuInfo.cin_idlogo.generic.callback	= UI_CinematicsMenu_Event; 
#ifdef TA_MISC
	cinematicsMenuInfo.cin_idlogo.string			= "LOGO";
#else
	cinematicsMenuInfo.cin_idlogo.string			= "ID LOGO";
#endif
	cinematicsMenuInfo.cin_idlogo.color				= text_big_color;
	cinematicsMenuInfo.cin_idlogo.style				= UI_CENTER;
#ifdef IOQ3ZTM
	// Check if there is a video file
	if (!UI_CanShowVideo(cinematics[0])) {
		cinematicsMenuInfo.cin_idlogo.generic.flags |= QMF_GRAYED;
	}
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_intro.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_intro.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_intro.generic.x			= 320;
	cinematicsMenuInfo.cin_intro.generic.y			= y;
	cinematicsMenuInfo.cin_intro.generic.id			= ID_CIN_INTRO;
	cinematicsMenuInfo.cin_intro.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_intro.string				= "INTRO";
	cinematicsMenuInfo.cin_intro.color				= text_big_color;
	cinematicsMenuInfo.cin_intro.style				= UI_CENTER;
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[1])) {
    cinematicsMenuInfo.cin_intro.generic.flags |= QMF_GRAYED;
    }
#else
	if( uis.demoversion ) {
		cinematicsMenuInfo.cin_intro.generic.flags |= QMF_GRAYED;
	}
#endif

#ifndef TA_SP // LESS_VIDEOS
	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier1.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier1.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier1.generic.x			= 320;
	cinematicsMenuInfo.cin_tier1.generic.y			= y;
	cinematicsMenuInfo.cin_tier1.generic.id			= ID_CIN_TIER1;
	cinematicsMenuInfo.cin_tier1.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier1.string				= "Tier 1";
	cinematicsMenuInfo.cin_tier1.color				= text_big_color;
	cinematicsMenuInfo.cin_tier1.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 1 ) ) {
		cinematicsMenuInfo.cin_tier1.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[2])) {
		cinematicsMenuInfo.cin_tier1.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier2.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier2.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier2.generic.x			= 320;
	cinematicsMenuInfo.cin_tier2.generic.y			= y;
	cinematicsMenuInfo.cin_tier2.generic.id			= ID_CIN_TIER2;
	cinematicsMenuInfo.cin_tier2.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier2.string				= "Tier 2";
	cinematicsMenuInfo.cin_tier2.color				= text_big_color;
	cinematicsMenuInfo.cin_tier2.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 2 ) ) {
		cinematicsMenuInfo.cin_tier2.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[3])) {
		cinematicsMenuInfo.cin_tier2.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier3.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier3.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier3.generic.x			= 320;
	cinematicsMenuInfo.cin_tier3.generic.y			= y;
	cinematicsMenuInfo.cin_tier3.generic.id			= ID_CIN_TIER3;
	cinematicsMenuInfo.cin_tier3.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier3.string				= "Tier 3";
	cinematicsMenuInfo.cin_tier3.color				= text_big_color;
	cinematicsMenuInfo.cin_tier3.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 3 ) ) {
		cinematicsMenuInfo.cin_tier3.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[4])) {
		cinematicsMenuInfo.cin_tier3.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier4.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier4.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier4.generic.x			= 320;
	cinematicsMenuInfo.cin_tier4.generic.y			= y;
	cinematicsMenuInfo.cin_tier4.generic.id			= ID_CIN_TIER4;
	cinematicsMenuInfo.cin_tier4.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier4.string				= "Tier 4";
	cinematicsMenuInfo.cin_tier4.color				= text_big_color;
	cinematicsMenuInfo.cin_tier4.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 4 ) ) {
		cinematicsMenuInfo.cin_tier4.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[5])) {
		cinematicsMenuInfo.cin_tier4.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier5.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier5.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier5.generic.x			= 320;
	cinematicsMenuInfo.cin_tier5.generic.y			= y;
	cinematicsMenuInfo.cin_tier5.generic.id			= ID_CIN_TIER5;
	cinematicsMenuInfo.cin_tier5.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier5.string				= "Tier 5";
	cinematicsMenuInfo.cin_tier5.color				= text_big_color;
	cinematicsMenuInfo.cin_tier5.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 5 ) ) {
		cinematicsMenuInfo.cin_tier5.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[6])) {
		cinematicsMenuInfo.cin_tier5.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier6.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier6.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier6.generic.x			= 320;
	cinematicsMenuInfo.cin_tier6.generic.y			= y;
	cinematicsMenuInfo.cin_tier6.generic.id			= ID_CIN_TIER6;
	cinematicsMenuInfo.cin_tier6.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier6.string				= "Tier 6";
	cinematicsMenuInfo.cin_tier6.color				= text_big_color;
	cinematicsMenuInfo.cin_tier6.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 6 ) ) {
		cinematicsMenuInfo.cin_tier6.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[7])) {
		cinematicsMenuInfo.cin_tier6.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_tier7.generic.type		= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_tier7.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_tier7.generic.x			= 320;
	cinematicsMenuInfo.cin_tier7.generic.y			= y;
	cinematicsMenuInfo.cin_tier7.generic.id			= ID_CIN_TIER7;
	cinematicsMenuInfo.cin_tier7.generic.callback	= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_tier7.string				= "Tier 7";
	cinematicsMenuInfo.cin_tier7.color				= text_big_color;
	cinematicsMenuInfo.cin_tier7.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 7 ) ) {
		cinematicsMenuInfo.cin_tier7.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[8])) {
		cinematicsMenuInfo.cin_tier7.generic.flags |= QMF_GRAYED;
    }
#endif

	y += VERTICAL_SPACING;
	cinematicsMenuInfo.cin_end.generic.type			= MTYPE_PTEXT;
	cinematicsMenuInfo.cin_end.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.cin_end.generic.x			= 320;
	cinematicsMenuInfo.cin_end.generic.y			= y;
	cinematicsMenuInfo.cin_end.generic.id			= ID_CIN_END;
	cinematicsMenuInfo.cin_end.generic.callback		= UI_CinematicsMenu_Event; 
	cinematicsMenuInfo.cin_end.string				= "END";
	cinematicsMenuInfo.cin_end.color				= text_big_color;
	cinematicsMenuInfo.cin_end.style				= UI_CENTER;
	if( !UI_CanShowTierVideo( 8 ) ) {
		cinematicsMenuInfo.cin_end.generic.flags |= QMF_GRAYED;
	}
#ifdef IOQ3ZTM
	// Check if there is a video file
    if (!UI_CanShowVideo(cinematics[9])) {
		cinematicsMenuInfo.cin_end.generic.flags |= QMF_GRAYED;
    }
#endif
#endif

	cinematicsMenuInfo.back.generic.type		= MTYPE_BITMAP;
	cinematicsMenuInfo.back.generic.name		= ART_BACK0;
	cinematicsMenuInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	cinematicsMenuInfo.back.generic.id			= ID_BACK;
	cinematicsMenuInfo.back.generic.callback	= UI_CinematicsMenu_BackEvent;
	cinematicsMenuInfo.back.generic.x			= 0;
	cinematicsMenuInfo.back.generic.y			= 480-64;
	cinematicsMenuInfo.back.width				= 128;
	cinematicsMenuInfo.back.height				= 64;
	cinematicsMenuInfo.back.focuspic			= ART_BACK1;

	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.banner );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.framel );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.framer );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_idlogo );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_intro );
#ifndef TA_SP // LESS_VIDEOS
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier1 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier2 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier3 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier4 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier5 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier6 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_tier7 );
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.cin_end );
#endif
	Menu_AddItem( &cinematicsMenuInfo.menu, &cinematicsMenuInfo.back );
}


/*
=================
UI_CinematicsMenu_Cache
=================
*/
void UI_CinematicsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_CinematicsMenu
===============
*/
void UI_CinematicsMenu( void ) {
	UI_CinematicsMenu_Init();
	UI_PushMenu( &cinematicsMenuInfo.menu );
}


/*
===============
UI_CinematicsMenu_f
===============
*/
void UI_CinematicsMenu_f( void ) {
	int		n;

	n = atoi( UI_Argv( 1 ) );
	UI_CinematicsMenu();
	Menu_SetCursorToItem( &cinematicsMenuInfo.menu, cinematicsMenuInfo.menu.items[n + 3] );
}
