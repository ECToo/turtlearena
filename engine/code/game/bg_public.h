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
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define	GAME_VERSION		BASEGAME "-1"

#define	DEFAULT_GRAVITY		800
#ifndef NOTRATEDM // No gibs.
#define	GIB_HEALTH			-40
#endif
#ifndef TMNT // NOARMOR
#define	ARMOR_PROTECTION	0.66
#endif

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	LIGHTNING_RANGE		768

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present
#ifdef SP_NPC
#define	CS_NPCS					28
#endif

#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)
#define CS_PARTICLES			(CS_LOCATIONS+MAX_LOCATIONS) 

#ifdef TMNT // Particles
#define CS_MAX					(CS_PARTICLES+MAX_PARTICLES_AREAS)
#else
#define CS_MAX					(CS_PARTICLES+MAX_LOCATIONS)
#endif

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_FFA,				// free for all
	GT_TOURNAMENT,		// one on one tournament
	GT_SINGLE_PLAYER,	// single player ffa

#if 0 // #ifdef TMNT
	GT_LMS,				// Last Man Standing
	GT_KOTH,			// King Of The Hill
	GT_KEEPAWAY,		// Keep Away
	// race, tag, and chaos?...
#endif

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
#if 0 // #ifdef TMNT
	GT_LTEAMS,			// last team standing
#endif
	GT_CTF,				// capture the flag
	GT_1FCTF,
	GT_OBELISK,
	GT_HARVESTER,
#if 0 // Turtle Man: In the gametype name arrays there is a Team Tournament.
	GT_TEAMTOURNAMENT,
#endif
	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

#ifndef TMNTPLAYERSYS // Moved below bg_playercfg_t
/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size
#ifdef TMNTHOLDSYS // NEXTHOLDABLE
#define PMF_NEXT_ITEM_HELD	32768
#endif

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;
#ifdef TMNTPLAYERSYS
	bg_playercfg_t	*playercfg;
#endif

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================
#endif

#ifdef TMNTPLAYERS
// Time that it takes to put weapon away to hold flag,
// or to get weapon back out
#define FLAG_CHANGE_TIME 250
#endif
// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
#ifndef TMNTHOLDSYS
	STAT_HOLDABLE_ITEM,
#endif
#ifdef MISSIONPACK
	STAT_PERSISTANT_POWERUP,
#endif
#ifdef TMNTWEAPSYS
	STAT_DEFAULTWEAPON, // default weapon
#endif
#ifdef TMNTWEAPSYS2
	// Players can have 3 weapons at once, there is a extra one when switching from pickup to pickup
	STAT_NEWWEAPON, // weapon that the player touched, they will change to it ASAP.
	STAT_OLDWEAPON, // weapon that the player just changed from
	//STAT_CURRENTWEAPON, // ps.weapon is the current weapon!

	// Replacement for the ammo array in playerState_t
	STAT_SAVEDAMMO, // Saved ammo for default weapon
	STAT_OLDAMMO, // ammo for STAT_OLDWEAPON
	STAT_NEWAMMO, // ammo for STAT_NEWWEAPON
	STAT_AMMO, // Ammo for current weapon
#else
	STAT_WEAPONS,					// 16 bit fields
#endif
#ifndef TMNT // NOARMOR
	STAT_ARMOR,				
#endif
#ifdef TMNTPLAYERS
	STAT_FLAGTIME,		// x>0= time that client pickup a ctf flag, x<0= time dropped flag
#endif
	STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH					// health / armor limit, changable by handicap
} statIndex_t;


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
#if !defined TMNT || !defined TMNTWEAPONS || !defined NOTRATEDM
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
#endif
	PERS_ATTACKER,					// clientnum of last damage inflicter
#ifdef TMNT // NOARMOR
	PERS_ATTACKEE_HEALTH,			// health of last person we attacked
#else
	PERS_ATTACKEE_ARMOR,			// health/armor of last person we attacked
#endif
	PERS_KILLED,					// count of the number of times you died
	// player awards tracking
#ifndef TMNTWEAPONS
	PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
#endif
	PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
	PERS_DEFEND_COUNT,				// defend awards
	PERS_ASSIST_COUNT,				// assist awards
#ifndef TMNTWEAPONS
	PERS_GAUNTLET_FRAG_COUNT,		// kills with the guantlet
#endif
	PERS_CAPTURES					// captures
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#if defined MISSIONPACK && !defined TMNTWEAPONS
#define EF_TICKING			0x00000002		// used to make players play the prox mine ticking sound
#endif
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#ifndef TMNTWEAPONS
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#endif
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#ifndef TMNTWEAPONS
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#endif
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
#if defined SINGLEPLAYER || defined SP_NPC // entity
#define	EF_FORCE_END_FRAME	0x00100000
#endif
#ifdef TMNTWEAPONS
// Removed EF_AWARD_GAUNTLET and EF_AWARD_IMPRESSIVE
#define EF_AWARD_BITS ( EF_AWARD_EXCELLENT | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP )
#endif

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,
#ifdef TMNT // POWERS
/* from TMNT FANGAME [tmnt in srb2]
	pw_invulnerability, // White [Gold in TMNT3] crystal: No damage for a limited time.
	pw_infinity, // Green [White in TMNT3] crystal: Unlimited Shuriken for a limited period.

	pw_strength, // Red crystal: Turtles gain increased attack power.
	pw_speed, // Blue crystal: Turtles gain increased speed.
	pw_defense, // Yellow crystal: Turtles gain increased defense.

	// Anti-powers - is there a good reason to add them?
	//  I think they would mostly just bug people.
	pw_weak, // decreased strength.
	pw_slow, // decreased speed.
	pw_nodefense, // decreased defense.
	pw_confused, // revesed controls.
*/

	PW_QUAD, // PW_STRENGTH: g_quadfactor * damge
	PW_BATTLESUIT, // PW_DEFENSE: Half damage
	PW_HASTE, // PW_SPEED: More Speed
	PW_INVIS, // Foot Tech powerup?

	// Turtle Man: I don't like...
	PW_REGEN,		// Give health back. 15 health per second?
	PW_FLIGHT,		// Allow player to fly around the level. Turtle Man: TODO: NiGHTS mode...

	PW_INVUL, // New INVULNERABILITY

	PW_REDFLAG,		// Player has red flag
	PW_BLUEFLAG,	//     ...has blue
	PW_NEUTRALFLAG,	//     ...has neutral

	// MISSIONPACK powers.
	PW_SCOUT,		// Speed?
	PW_GUARD,		// Regen health?
	PW_DOUBLER,		// Doubles attack power
	PW_AMMOREGEN,	// Regen ammo? --What about Melee weapons?
	//PW_INVULNERABILITY, // TMNT Disables MISSIONPACK's PW_INVULNERABILITY
#else
	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	PW_AMMOREGEN,
	PW_INVULNERABILITY,
#endif

	PW_NUM_POWERUPS

} powerup_t;

#ifdef TMNTHOLDSYS
/*
	No item in bg_itemlist can use the same HI_* tag.
	There can be a max of 16 holdable items, see MAX_HOLDABLE.
	Turtle Man Nov 3, 2008
*/
#endif
typedef enum {
	HI_NONE,

#ifndef TMNTHOLDABLE // no q3 teleprter
	HI_TELEPORTER,
#elif !defined TMNTHOLDSYS
	HI_TELEPORTER_REMOVED, // Q3 want them in this order in game
#endif
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
#ifndef TMNT // POWERS
	HI_INVULNERABILITY,
#elif !defined TMNTHOLDSYS
	HI_INVULNERABILITY_REMOVED, // Q3 want them in this order in game
#endif

#ifdef TMNTHOLDABLE
	// Shurikens
	HI_SHURIKEN,
	HI_ELECTRICSHURIKEN,
	HI_FIRESHURIKEN,
	HI_LASERSHURIKEN,

	// Turtle Man: TODO: Make the grapple a holdable item?
	//       So that players can use the grapple and a weapon at the same time?
	// 20090316: Have the grapple in the player's secondary hand like the flag?
#endif

	HI_NUM_HOLDABLE
} holdable_t;

#ifdef TMNTHOLDSYS
// Hold a max of 99 of each shuriken type. (Or any other holdable).
#define MAX_SHURIKENS 99
#endif

#ifdef TMNTWEAPSYS
// Weapon type
// The "Primary" weapon hand is choosen by the modeler, the "normal" is right hand.
// \note if any weapon types are added/removed, update bg_weapontypeinfo
typedef enum
{
    WT_NONE, // Dummy type

	// All of the code is here so I could make a gauntlet weapon type instead of disabling it all?
	WT_GAUNTLET, // Uses primary hand only

    WT_GUN, // One gun, both hands.
    WT_GUN_PRIMARY, // One gun, left hand

#ifdef TMNTWEAPONS
	// Sword1 types.
    WT_SWORD1_BOTH, // One sword, uses both hands
	WT_SWORD1_PRIMARY, // One sword, uses left hand only

    WT_SHORT_SWORD1_BOTH, // One sword, uses both hands
	WT_SHORT_SWORD1_PRIMARY, // One sword, uses left hand only \\Used for one sai

	// Sword2 types.
	// The one handed versions are WT_SWORD1_PRIMARY and WT_SHORT_SWORD1_PRIMARY
	WT_SWORD2, // Two swords, one in each hand.
	WT_SHORT_SWORD2, // Two swords, one in each hand. \\Used for Sais.
	WT_SWORD2_SHORT1_LONG1, // Usagi... Left is wakizashi (short), right is katana (long)
	WT_SWORD2_LONG1_SHORT1, // Swapped version of Usagi...

    WT_BO, // One Bo, uses both hands
    WT_BO_PRIMARY, // One Bo, left hand.

    WT_HAMMER, // One hammer, uses both hands
    WT_HAMMER_PRIMARY, // One hammer, left hand.

	// Nunchuks are going take more work then the other weapons.
	// Have three models, handle, chain, handle2
	//   connect the three models using tags and swing the chain and handle2 using the
	//   flag swing code.
	// OR: Have one model that is animated. Better/easier thing to do?
	//   but it needs more thinking...
	//
	// Ooh! Maybe I can use Q3 "_barrel.md3" the code!
    WT_NUNCHUKS, // Two nunchuk, one in each hand.
    WT_NUNCHUKS1_PRIMARY, // One nunchuk, left hand.
#endif

    WT_MAX

} weapontype_t;

// Default weapon if animation.cfg doesn't set one.
#ifdef TMNTWEAPONS
/*#ifdef TMNT_SUPPORTQ3
// Quake 3 players don't have a "default_weapon" in there animation.cfg
// GUNS_AS_DEFAULT  --Keyword for code
// Turtle Man: FIXME: Should be WP_GAUNTLET for quake3 players
//   Guns as default weapon is disabed.
//   This is due to ammo for default weapons being problatic;
//   Guns will be one of three things, unlimited ammo or cheatable
//    (swap to other player and back to get ammo)
//    or unable to be used...
//   Not sure if this is fix able, and currently modifying any part of
//    the player's info string will reload the player, causing ammo to reset.

#define DEFAULT_DEFAULT_WEAPON WP_GUN
#else // !TMNT_SUPPORTQ3
*/
#define DEFAULT_DEFAULT_WEAPON WP_FISTS
//#endif // TMNT_SUPPORTQ3

#else // !TMNTWEAPONS
#ifdef SONICWEAPONS
#define DEFAULT_DEFAULT_WEAPON WP_RED_RING
#else
#define DEFAULT_DEFAULT_WEAPON WP_GAUNTLET // WP_MACHINEGUN
#endif // SONICWEAPONS
#endif // TMNTWEAPONS
#endif // TMNTWEAPSYS

#ifdef SP_NPC
#define WP_FIREBALL WP_ROCKET_LAUNCHER
#ifndef TMNTWEAPONS
#define WP_BAT WP_ROCKET_LAUNCHER // Uses the same name as my weapon.
#endif
#define WP_SEA1 WP_ROCKET_LAUNCHER
#define WP_SEA2 WP_ROCKET_LAUNCHER
#ifndef TMNTWEAPONS
#define WP_GUN WP_ROCKET_LAUNCHER // Uses the same name as my weapon.
#endif
#endif
typedef enum {
#ifdef TMNTWEAPSYS
    WP_DEFAULT = -1, // This weapon will need to be remapped to the default weapon.
#endif

#ifdef TMNTWEAPONS
	// Many of the weapons are based on weapons in TMNT: Mutant Melee

	WP_NONE, // Replace "none" with fists?
			// -- lot of code fixing would be needed "for (i = 1; i < numweapons; i++)", ect

	// For players like Hun who don't have a default "weapon" but just use there fists.
	WP_FISTS, // Invisible weapon, each hand, short range damage.

	// \swords
    WP_KATANAS, // Two swords.
    WP_DAISHO, // 1 katana and 1 wakizashi, for Usagi...

	// \sais --short swords?
    WP_SAIS, // Two sais

	// \nunchuks
    WP_NUNCHUKS, // Two nunchuks
    // WP_CHAINS, // Two metal chains, one in each hand, used like Nunchuks

	// \hammers
	WP_HAMMER,
	WP_AXE,
	//WP_BAMBOOHAMMER, // Bamboo [hammer type], its in Mutant Melee but I don't plan on adding it.

	// \sword1_both
	WP_LONGSWORD,
	WP_BAT,
	//WP_SPIKEDCLUB, // In Mutant Melee...

    // \bos
    WP_BO, // One Bo, uses both hands
	WP_BAMBOOBO, // Bamboo [bo type]

    // \guns
	// Turtle Man: TODO: Make WP_GUN like WP_MACHINEGUN that shoots missial, well sortof...
	//                   It was going to be a chaingun like in Mutant Melee,
	//                   But I think I will make it a tri-blaster instead.
	WP_GUN,
	WP_ELECTRIC_LAUNCHER, // Federation? or Tri? // Sort of like WP_PLASMAGUN.
	WP_ROCKET_LAUNCHER, // Tri rocket launcher // AI Same as in Q3...
	WP_HOMING_LAUNCHER, // EPF homing rocket launcher // TMNT:MM Yellow homing-rocket launcher

    // Leave gappling hook in, because its cool.
	WP_GRAPPLING_HOOK, // Model will be modified tri-blaster

	// MISSIONPACK NOTE: TMNTWEAPONS disables the 3 missionpack weapons.

	WP_NUM_WEAPONS
#elif defined SONICWEAPONS
	// Weapons based on SRB2 1.1 weapons.
	WP_NONE,

	WP_RED_RING,
	WP_AUTOMATIC_RING,
	WP_BOUNCE_RING,
	WP_SCATTER_RING, // WP_SHOTGUN
	WP_GRENADE_RING, // WP_GRENADE_LAUNCHER
	WP_EXPLOSION_RING,
	WP_RAIL_RING, // WP_RAILGUN

	// Removed from SRB2 1.1 (Support yet way?...)
	WP_HOMING_RING,

	WP_NUM_WEAPONS
#else
	WP_NONE,

	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
#ifdef MISSIONPACK
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,
#endif

	WP_NUM_WEAPONS
#endif // TMNTWEAPONS
} weapon_t;

#ifdef TMNTWEAPSYS
#define HAND_PRIMARY 1
#define HAND_SECONDARY 2
#define HAND_BOTH (HAND_PRIMARY) // Weapons that use "both" really only use the primary.
#define HAND_EACH (HAND_PRIMARY|HAND_SECONDARY)
typedef struct
{
	int hands; // bitfield, see HAND_* defines
	weapontype_t oneHanded;	// Use this type when holding a CTF flag.
	int standAnim;
	int attackAnim;
	// Melee weapons should have more than one attack
	//int attackAnim2;
	//int attackAnim3;

} bg_weapontypeinfo_t;

/*
	Turtle Man: TODO: Load bg_weapontypeinfo and bg_weaponinfo from a text file,
		and allow new weapons to be added too.
	Main file: "scripts/weaponinfo.txt"
	Addon file example "scripts/my_sword.weap"

weapontype "wt_katana"
{
	hands HAND_EACH
	oneHanded "wt_sword1_primary"
	standAnim TORSO_STAND2
	attackAnim TORSO_ATTACK2
}

weapon "wp_katana"
{
	name "Katanas"
	item weapon_katanas
	weapontype "wt_katana"
	mod MOD_KATANA
	mod2 MOD_KATANA
	// Other data...
}

*/
typedef struct
{
	//char name[MAX_QPATH];
	//gitem_t *item;

	weapontype_t weapontype;
	int mod; // Means of death, MOD_* enum - Primary weapon
	// Turtle Man: TODO: If WT_GUN, have mod2 be splash damage MOD?
	// -or- support guns in both hands?...
	int mod2; // Means of death, MOD_* enum - Secondary weapon

	// Damage amounts
	//int damage; // Damage given by Primary weapon
	//int damage2; // Damage given by Secondary weapon

	// Melee weapons
	// Primary weapon
	vec3_t mins;
	vec3_t maxs;
	int start_range;			// can be neg to have the weapon do damge below tag
	int end_range;				// Dist to attack

	// Secondary weapon
	//vec3_t mins2;
	//vec3_t maxs2;
	//int start_range2;
	//int end_range2;

} bg_weaponinfo_t;

extern bg_weapontypeinfo_t bg_weapontypeinfo[WT_MAX];
extern bg_weaponinfo_t bg_weaponinfo[WP_NUM_WEAPONS];

weapontype_t BG_WeaponTypeForPlayerState(playerState_t *ps);
weapontype_t BG_WeaponTypeForNum(weapon_t weaponnum);
qboolean BG_WeapTypeIsMelee(weapontype_t wt);
qboolean BG_WeapUseAmmo(weapon_t w);
#endif
#ifdef TMNTHOLDSYS
int BG_ItemNumForHoldableNum(holdable_t holdablenum);
#endif

// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#ifndef TMNT
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#endif
#ifndef TMNTWEAPONS
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#endif
#ifndef NOTRATEDM // Disable strong lang.
#define PLAYEREVENT_HOLYSHIT			0x0004
#endif

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP_PAD,			// boing sound at origin, jump sound on player

	EV_JUMP,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves

	EV_ITEM_PICKUP,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

#ifdef TMNTWEAPSYS2
	EV_DROP_WEAPON,
#else
	EV_NOAMMO,
#endif
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

#ifdef TMNTHOLDABLE
	EV_LASERSHURIKEN_BOUNCE,
#else
	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex
#endif

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,

#ifdef TMNTENTITIES
	EV_FX_CHUNKS,
#endif
#ifndef NOTRATEDM // No gibs.
	EV_GIB_PLAYER,			// gib a previously living player
#endif
	EV_SCOREPLUM,			// score plum

//#ifdef MISSIONPACK
#ifndef TMNTWEAPONS
	EV_PROXIMITY_MINE_STICK,
	EV_PROXIMITY_MINE_TRIGGER,
#endif
	EV_KAMIKAZE,			// kamikaze explodes
	EV_OBELISKEXPLODE,		// obelisk explodes
	EV_OBELISKPAIN,			// obelisk is in pain
#ifndef TMNT // POWERS
	EV_INVUL_IMPACT,		// invulnerability sphere impact
	EV_JUICED,				// invulnerability juiced effect
	EV_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere
#endif
//#endif

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL

} entity_event_t;


typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

// animations
#ifdef TMNTPLAYERS
// NOTE: In animation.cfg I call some animations by other names;
// * TORSO_ATTACK_GUN is TORSO_ATTACK
// * TORSO_ATTACK_GUNTLET is TORSO_ATTACK2
// * TORSO_STAND_GUN is TORSO_STAND
// * TORSO_STAND_GAUNTLET is TORSO_STAND2
#endif
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

#ifdef TMNTPLAYERS // New TMNT player animations
	// Place default weapons somewhere on there person while there not used.
	// TORSO_***DEFAULT_SECONDARY for Don should be
	//  switching to/from two handed Bo to using one hand.
	TORSO_PUTDEFAULT_BOTH,
	TORSO_PUTDEFAULT_PRIMARY,
	TORSO_PUTDEFAULT_SECONDARY,
	TORSO_GETDEFAULT_BOTH,
	TORSO_GETDEFAULT_PRIMARY,
	TORSO_GETDEFAULT_SECONDARY,

	// Pickup weapons should support CTF flags too?
	// TORSO_PUT_SECONDARY,
	// TORSO_GET_SECONDARY,

	// Gun-type standing animations
	//TORSO_STAND_GUNTLET, // Would be the same as TORSO_STAND2...
    //TORSO_STAND_GUN, // Would be the same as TORSO_STAND...
    TORSO_STAND_GUN_PRIMARY, // I could reuse TORSO_STAND2 even though its for the gauntlet...

	// Melee weapon standing animations
    TORSO_STAND_SWORD1_BOTH,
    TORSO_STAND_SWORD1_PRIMARY,

    TORSO_STAND_SHORT_SWORD1_BOTH,
    TORSO_STAND_SHORT_SWORD1_PRIMARY, // One Sai...

    TORSO_STAND_SWORD2,
    TORSO_STAND_SHORT_SWORD2, // Two Sais...
    TORSO_STAND_SWORD2_SHORT1_LONG1,
    TORSO_STAND_SWORD2_LONG1_SHORT1,

    TORSO_STAND_BO,
    TORSO_STAND_BO_PRIMARY,

    TORSO_STAND_HAMMER,
    TORSO_STAND_HAMMER_PRIMARY,

    TORSO_STAND_NUNCHUKS,
    TORSO_STAND_NUNCHUKS1_PRIMARY,

	// Gun attacks
    //TORSO_ATTACK_GUNTLET, // Would be the same as TORSO_ATTACK2...
    //TORSO_ATTACK_GUN, // Would be the same as TORSO_ATTACK...
    TORSO_ATTACK_GUN_PRIMARY, // Can't reuse TORSO_ATTACK2 needs a new animation.

    // Turtle Man: TODO: Attack animations, how many anims per type?...
    //       Only one for WT_HAMMER and WT_GUN
    TORSO_ATTACK_SWORD1_BOTH_1,
    TORSO_ATTACK_SWORD1_BOTH_2,
    TORSO_ATTACK_SWORD1_BOTH_3,

    TORSO_ATTACK_SWORD1_PRIMARY_1,
    TORSO_ATTACK_SWORD1_PRIMARY_2,
    TORSO_ATTACK_SWORD1_PRIMARY_3,

    ///TORSO_ATTACK_SHORT_SWORD1_BOTH,
    ///TORSO_ATTACK_SHORT_SWORD1_PRIMARY,

    ///TORSO_ATTACK_SWORD2,
    ///TORSO_ATTACK_SHORT_SWORD2,
    ///TORSO_ATTACK_SWORD2_SHORT1_LONG1,
    ///TORSO_ATTACK_SWORD2_LONG1_SHORT1,

    ///TORSO_ATTACK_BO,
    ///TORSO_ATTACK_BO_PRIMARY,

    TORSO_ATTACK_HAMMER,
    TORSO_ATTACK_HAMMER_PRIMARY,

    ///TORSO_ATTACK_NUNCHUKS,
    ///TORSO_ATTACK_NUNCHUKS1_PRIMARY,
#endif

	MAX_ANIMATIONS,

#ifdef TMNTPLAYERS
	// Turtle Man: TODO:
	// * LEGS_BACKCR/BACKWALK should be with the others (Were they added after public release?)
	// * FLAG_RUN (ect) are hacks for flag animations, not player animations!
#endif
	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;

#ifdef TMNTWEAPSYS
animNumber_t BG_TorsoStandForPlayerState(playerState_t *ps);
animNumber_t BG_TorsoAttackForPlayerState(playerState_t *ps);
animNumber_t BG_TorsoStandForWeapon(weapon_t weaponnum);
animNumber_t BG_TorsoAttackForWeapon(weapon_t weaponnum);
#endif

#ifdef SP_NPC
// NPC animations
typedef enum {
	ANPC_DEATH1,
	ANPC_DEATH2,
	ANPC_DEATH3,
	ANPC_TAUNT,
	ANPC_ATTACK_FAR,
	ANPC_ATTACK_MELEE,
	ANPC_STANDING,
	ANPC_STANDING_ACTIVE,
	ANPC_WALK,
	ANPC_RUN,
	ANPC_BACKPEDAL,
	ANPC_JUMP,
	ANPC_LAND,
	ANPC_PAIN,
	MAX_ANIMATIONS_NPC
} animNumberNPC_t;
#endif


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128

#ifdef TMNTPLAYERSYS
// Moved footstep_t to both game from client game.
typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef struct bg_playercfg_s
{
    char filename[MAX_QPATH];

	// Q3 animation data.
	animation_t		animations[MAX_TOTALANIMATIONS];

	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model

	// New Info for TMNT, allows player models to have data that changes
	//  what happens in game.
#ifdef TMNTWEAPSYS
    weapon_t default_weapon;
#endif

	// Player's boundingbox
	vec3_t bbmins;
	vec3_t bbmaxs;

	// Speed control, some characters are faster then others.
	int   max_speed;
	float accelerate_speed; // Replaces pm_accelerate; default 10.0f

#if 0
	// model info
	// These will be used to find the location of a "tag_*" on the model.
	//  For Real melee weapon attack checking.
	//  But this will require moving (or allowing) model loading in Both Game.
	//  Currently models are only loaded in CGame/UI (Which is fine if you don't
	//   care where a point on the model is each frame, but I want to know.)
	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	lerpFrame_t		legs;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	lerpFrame_t		torso;

	qhandle_t		headModel;
	qhandle_t		headSkin;
#endif

} bg_playercfg_t;

//qboolean BG_ParsePlayerCFGFile(const char *filename, bg_playercfg_t *playercfg);
qboolean BG_LoadPlayerCFGFile(const char *model, bg_playercfg_t *playercfg);
#endif

#ifdef TMNTPLAYERSYS // Moved below bg_playercfg_t
/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size
#ifdef TMNTHOLDSYS // NEXTHOLDABLE
#define PMF_NEXT_ITEM_HELD	32768
#endif

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;
#ifdef TMNTPLAYERSYS
	bg_playercfg_t	*playercfg;
#endif

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================
#endif

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
#if 0 // #ifdef TMNT // TMNTTEAM4 // Turtle Man: TODO: team orange and team purple
	TEAM_PURPLE,
	TEAM_ORANGE,
#endif
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE, 
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	MOD_UNKNOWN,
#ifdef TMNTWEAPONS // MOD
	MOD_FIST, // _FISTS
	MOD_KATANA, // _KATANAS
	MOD_WAKIZASHI, // Usagi's shorter sword
	MOD_SAI,	// _SAIS
	MOD_NUNCHUK, // _NUNCHUKS
	MOD_HAMMER,
	MOD_AXE,
	//MOD_BAMBOOHAMMER,
	MOD_SWORD, // _LONGSWORD
	MOD_BAT,
	//MOD_SPIKEDCLUB,
	MOD_BO,
	MOD_BAMBOOBO,
	MOD_GUN,
	MOD_ELECTRIC,
	MOD_ELECTRIC_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_HOMING,
	MOD_HOMING_SPLASH,
#else
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_PLASMA,
	MOD_PLASMA_SPLASH,
	MOD_RAILGUN,
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
#endif
#ifdef TMNTHOLDABLE
	MOD_SHURIKEN,
	MOD_FIRESHURIKEN,
	MOD_FIRESHURIKEN_EXPLOSION,
	MOD_ELECTRICSHURIKEN,
	MOD_LASERSHURIKEN,
#endif
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
#ifdef TMNTENTITIES
	MOD_EXPLOSION, // Use for func_breakable explosions.
#endif
#ifdef MISSIONPACK
#ifndef TMNTWEAPONS // MOD
	MOD_NAIL,
	MOD_CHAINGUN,
	MOD_PROXIMITY_MINE,
#endif

	MOD_KAMIKAZE,
#ifndef TMNTWEAPONS // MOD
	MOD_JUICED,
#endif
#endif
	MOD_GRAPPLE
} meansOfDeath_t;

#ifdef SP_NPC
typedef enum {
	NPC_NONE,

	// Badies
	NPC_FLYBOT,
	NPC_MOUSER,

	// Earth Protection Force Soldiers, they work for Agent Bishop
	NPC_EPFSOLDIER1,
	NPC_EPFSOLDIER2,
	NPC_EPFSOLDIER3,

/*
	NPC_PURPLEDRAGON1,
	NPC_PURPLEDRAGON2,
	NPC_PURPLEDRAGON3,
	NPC_PURPLEDRAGON4,
	NPC_PURPLEDRAGON5,
	NPC_PURPLEDRAGON6,
	NPC_PURPLEDRAGON7,
	NPC_PURPLEDRAGON8,
	NPC_PURPLEDRAGON9,
*/

	// TDC Badies, to be removed.
	NPC_ANK,
	NPC_BAT,
	NPC_HULK,
	NPC_METLAR,
	NPC_PILOT,
	NPC_SEALORD,
	NPC_SOLDIER1,
	NPC_SOLDIER2,

	NPC_NUMNPCS,

	NPC_MAX = NPC_NUMNPCS

	// 25 free NPC slots.
	//NPC_FREESLOT1 = NPC_NUMNPCS,
	//NPC_FREESLOT25 = NPC_FREESLOT1 + 25,

	//NPC_MAX
} npcType_t;

#define HULK_QUAKE_LEN	1700 // msec

// Turtle Man: Flags for general NPC effects.
typedef enum
{
	NPCF_NODROPWEAPON	= 1, // Don't drop weapon when killed.
	NPCF_WALKANDFLY		= 2, // Can't run, but can fly
	NPCF_FLYONLY		= 4, // Can't walk or run, only fly -fast and slow
	NPCF_ALLY			= 8, // Not a baddy, NPC on the same side don't attack each other.

	NPCF_LAST // dummy flag

} npcflag_e;

// Turtle Man: General death types.
typedef enum
{
	NPCD_NONE, // Stays there doing nothing. Forever.
	NPCD_SINK, // Sink into the ground and removes the ent.
	NPCD_EXPLODE, // Starts explotion and removes the ent. For robots.
	NPCD_SMOKE, // Starts smoke and removes the ent.

	NPCD_MAX
} npcDeath_e;

#define MAX_NPCNAME 32
typedef struct gnpc_s {
	char *classname; // MAX_NPCNAME
	npcType_t npcType;
	weapon_t weapon; // Turtle Man: weapon to hold/use [tag_weapon]
	int flags; // see npcflag_e
	int deathType; // see npcDeath_e
	int health;
	float painFreq;
	int	walkingSpeed;
	int runningSpeed;
	int fov;
	int jumpHeight;
	int walkingRotSpd;
	int runningRotSpd;
	int melee_dist;
	int melee_damage;
	int far_damage;
	int animTimes[MAX_ANIMATIONS_NPC];
	vec3_t mins, maxs, eye;
} bgnpc_t;

extern bgnpc_t bg_npclist[];
#endif

//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
#ifndef TMNT // NOARMOR
	IT_ARMOR,				// EFX: rotate + minlight
#endif
#ifdef TMNT // CRATE
	IT_CRATE,				// EFX: Turtle Man: TODO: solid + minlight
#endif
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	char		*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model[MAX_ITEM_MODELS];

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

#ifdef IOQ3ZTM
	char		*unused;		// This field is unused.
#else
	char		*precaches;		// string of all models and images this item will use
#endif
	char		*sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
extern	int		bg_numItems;

gitem_t	*BG_FindItem( const char *pickupName );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps );


// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,
#ifdef SP_NPC
	ET_NPC,
#endif
#ifdef SINGLEPLAYER // entity
	ET_MODELANIM,
#endif

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;



void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704

