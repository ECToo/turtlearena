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
#include "g_local.h"

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	trap_LinkEntity( ent );
}

#ifdef TMNTWEAPONS
/*
================
G_Missile_Die

From XREAL r3036
================
*/
static void G_Missile_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	ent->nextthink = level.time + 1;
	ent->think = G_ExplodeMissile;
}
#endif

#ifdef TMNTHOLDABLE
/*
=================
fire_shuriken
=================
*/
gentity_t *fire_shuriken (gentity_t *self, vec3_t start, vec3_t dir, holdable_t holdable) {
	gentity_t	*bolt;
	vec3_t          mins = { -8, -8, -8 };
	vec3_t			maxs = { 8, 8, 8 };

	VectorNormalize (dir);

	bolt = G_Spawn();

	// So that CG_Shuriken has the right yaw.
	bolt->s.angles[1] = self->client->ps.viewangles[1];

	// Shuriken: 10 damage.
	// Electric Shuriken: Low Damage (5) TODO: Stun player and electricfy water.
	// Fire Shuriken: Higher damage (15), plus explode
	// Laser Shuriken: Faster, bounce (max of 3), low damage (7).

	if (holdable == HI_LASERSHURIKEN)
	{
		bolt->classname = "thrown laser shuriken";
		bolt->s.eFlags = EF_BOUNCE;
		bolt->s.generic1 = 3; // Die on 3rd bounce

		bolt->damage = 7;
		bolt->splashDamage = 0;
		bolt->splashRadius = 0;
		bolt->methodOfDeath = MOD_LASERSHURIKEN;
		bolt->splashMethodOfDeath = MOD_UNKNOWN;

		VectorScale( dir, 400, bolt->s.pos.trDelta );
	}
	else if (holdable == HI_FIRESHURIKEN)
	{
		bolt->classname = "thrown fire shuriken";

		bolt->damage = 15;
		bolt->splashDamage = 100;//15;
		bolt->splashRadius = 120;//60; //120;
		bolt->methodOfDeath = MOD_FIRESHURIKEN;
		bolt->splashMethodOfDeath = MOD_FIRESHURIKEN_EXPLOSION;

		VectorScale( dir, 700, bolt->s.pos.trDelta );
	}
	else if (holdable == HI_ELECTRICSHURIKEN)
	{
		bolt->classname = "thrown electric shuriken";

		bolt->damage = 10;
		bolt->splashDamage = 0;
		bolt->splashRadius = 0;
		bolt->methodOfDeath = MOD_ELECTRICSHURIKEN;
		bolt->splashMethodOfDeath = MOD_UNKNOWN;

		VectorScale( dir, 700, bolt->s.pos.trDelta );
	}
	else
	{
		bolt->classname = "thrown shuriken";

		bolt->damage = 10;
		bolt->splashDamage = 0;
		bolt->splashRadius = 0;
		bolt->methodOfDeath = MOD_SHURIKEN;
		bolt->splashMethodOfDeath = MOD_UNKNOWN;

		VectorScale( dir, 700, bolt->s.pos.trDelta );
	}

	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NUM_WEAPONS+holdable;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;

	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	// make the rocket shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 10;
	bolt->die = G_Missile_Die;

	return bolt;
}
#endif

#if defined MISSIONPACK && !defined TMNTWEAPONS
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->r.mins, -r, -r, -r );
	VectorSet( trigger->r.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

#ifndef TMNT // POWERS
	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else
#endif
	{
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW ) {
		return;
	}

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
#ifndef TMNT // POWERS
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else
#endif
	{
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif

/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
#if defined MISSIONPACK && !defined TMNT // POWERS
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
#endif
	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
#ifdef TMNTHOLDABLE // HI_LASERSHURIKEN
		// If its a laser shuriken
		if (ent->s.weapon == WP_NUM_WEAPONS+HI_LASERSHURIKEN)
		{
			// Die on 3rd bounce
		if (ent->s.generic1) // generic1 is used by Prox Mines, but they don't have EF_BOUNCE
		{
			--ent->s.generic1;
			if (!ent->s.generic1)
			{
				G_AddEvent( ent, EV_LASERSHURIKEN_BOUNCE, 1 );
				// Kill missile
				G_ExplodeMissile( ent );
				return;
			}
		}
		G_AddEvent( ent, EV_LASERSHURIKEN_BOUNCE, 0 );
			G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
		}
#ifndef TMNTWEAPONS
		else
#endif
#endif
#ifndef TMNTWEAPONS
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
#endif
		return;
	}

#if defined MISSIONPACK && !defined TMNT // POWERS
	if ( other->takedamage ) {
		if ( ent->s.weapon != WP_PROX_LAUNCHER )
		{
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
			G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage, 
				0, ent->methodOfDeath);
		}
	}

#if defined MISSIONPACK && !defined TMNTWEAPONS
	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) ) {
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
#if defined MISSIONPACK && !defined TMNTWEAPONS
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
#endif
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
#ifdef TMNTWEAPONS // Turtle Man: TODO: Pull grapple back to player, like LoZ: TP.
#endif
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}
#if defined MISSIONPACK && !defined TMNTWEAPONS
	// if the prox mine wasn't yet outside the player body
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
#endif
	// check think function after bouncing
	G_RunThink( ent );
}

#ifdef TMNTWEAPONS
//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_gun(gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bullet";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 10;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_GUN;
	bolt->splashMethodOfDeath = MOD_UNKNOWN;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
================
G_HomingMissile

From XREAL r3036
================
*/
void G_HomingMissile(gentity_t * ent)
{
	gentity_t	*target = NULL;
	gentity_t	*blip = NULL;
	vec3_t          dir, blipdir;
	vec_t           angle;
	//qboolean        chaff;
	//qboolean        ignorechaff = qfalse;
	const int		HOMING_THINK_TIME = 60;

#if 0 // Turtle Man: FIXME: need to add spawnTime for this to work.
	// explode after 15 seconds without a hit
	if (ent->spawnTime + 15000 <= level.time)
	{
		G_ExplodeMissile(ent);
		return;
	}
#endif

	/*
	if(ent->parent->health <= 0)
	{
		ent->nextthink = level.time + 15000;
		ent->think = G_ExplodeMissile;
		return;
	}
	*/

	/*
	if(ent->parent && ent->parent->client)
	{
		ignorechaff = (ent->parent->client->ps.powerups[PW_ACCURACY] > 0);
	}
	*/

	while((blip = G_FindRadius(blip, ent->r.currentOrigin, 2000)) != NULL)
	{
#if 0
		if(blip->s.weapon == WP_CHAFF)
		{
			if(ignorechaff)
			{
				continue;
			}

			chaff = qtrue;
		}
		else
#endif
		{
			//chaff = qfalse;

			if(blip->client == NULL)
				continue;

			if(blip == ent->parent)
				continue;

			if(blip->health <= 0)
			continue;

			if(blip->client->sess.sessionTeam >= TEAM_SPECTATOR)
			continue;

#if 1 // Turtle Man
			if(g_gametype.integer >= GT_TEAM && OnSameTeam(blip, ent->parent))
#else
			if((g_gametype.integer == GT_TEAM || g_gametype.integer == GT_CTF) && OnSameTeam(blip, ent->parent))
#endif
				continue;
		}

		if(!G_IsVisible(ent, blip->r.currentOrigin))
			continue;

		VectorSubtract(blip->r.currentOrigin, ent->r.currentOrigin, blipdir);

		//if(chaff)
		//{
		//	VectorScale(blipdir, 0.5, blipdir);
		//}

		if((target == NULL) || (VectorLength(blipdir) < VectorLength(dir)))
		{
			//if(chaff)
			//{
			//	VectorScale(blipdir, 2, blipdir);
			//}

			angle = AngleBetweenVectors(ent->r.currentAngles, blipdir);

			if(angle < 120.0f)
			{
				// We add it as our target
				target = blip;
				VectorCopy(blipdir, dir);
			}
		}
	}

	if(target == NULL)
	{
		ent->nextthink = level.time + HOMING_THINK_TIME;	// + 10000;
		ent->think = G_HomingMissile;
	}
	else
	{
		// for exact trajectory calculation, set current point to base.
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

		VectorNormalize(dir);
		// 0.5 is swing rate.
		VectorScale(dir, 0.5, dir);
		VectorAdd(dir, ent->r.currentAngles, dir);

		// turn nozzle to target angle
		VectorNormalize(dir);
		VectorCopy(dir, ent->r.currentAngles);

#if 0 // Turtle Man: XREAL
		// scale direction, put into trDelta
		if(g_rocketAcceleration.integer)
		{
			// use acceleration instead of linear velocity
			ent->s.pos.trType = TR_ACCELERATION;
			ent->s.pos.trAcceleration = g_rocketAcceleration.value;
			VectorScale(dir, g_rocketVelocity.value, ent->s.pos.trDelta);
	}
		else
#endif
		{
			ent->s.pos.trType = TR_LINEAR;
#if 1 // Turtle Man
			VectorScale(dir, 450, ent->s.pos.trDelta);
#else // XREAL
			VectorScale(dir, g_rocketVelocity.value * 0.25, ent->s.pos.trDelta);
#endif
		}

		ent->s.pos.trTime = level.time;

		SnapVector(ent->s.pos.trDelta);	// save net bandwidth
		ent->nextthink = level.time + HOMING_THINK_TIME;	// decrease this value also makes fast swing
		ent->think = G_HomingMissile;

		//G_Printf("targeting %s\n", target->classname);
	}
}

/*
=================
fire_homingrocket

Based on XREAL's fire_homing r2785
=================
*/
gentity_t *fire_homingrocket(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t	*bolt;
	vec3_t          mins = { -8, -8, -8 };
	vec3_t			maxs = { 8, 8, 8 };

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "homingrocket";//"rocket_homing";
	bolt->nextthink = level.time + 600;
	bolt->think = G_HomingMissile;
	bolt->s.eType = ET_MISSILE; //ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_HOMING_LAUNCHER; // WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 25; // 100;
	bolt->splashDamage = 25; // 100;
	bolt->splashRadius = 60; // 120;
	bolt->methodOfDeath = MOD_HOMING; // MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_HOMING_SPLASH; // MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	// make the rocket shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 25; // 50;
	bolt->die = G_Missile_Die;

#if 0 // Turtle Man
	if(g_rocketAcceleration.integer)
	{
		// use acceleration instead of linear velocity
		bolt->s.pos.trType = TR_ACCELERATION;
		bolt->s.pos.trAcceleration = g_rocketAcceleration.value;
		VectorScale(dir, g_rocketVelocity.value, bolt->s.pos.trDelta);
	}
	else
#endif
	{
	bolt->s.pos.trType = TR_LINEAR;
#if 1	// Turtle Man
		VectorScale(dir, 450, bolt->s.pos.trDelta);
#else
		VectorScale(dir, g_rocketVelocity.value * 0.25, bolt->s.pos.trDelta);
#endif
	}

	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}
#endif

//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
#ifdef TMNTWEAPONS
	bolt->s.weapon = WP_ELECTRIC_LAUNCHER;
#else
	bolt->s.weapon = WP_PLASMAGUN;
#endif
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
#ifdef TMNTWEAPONS // MOD
	bolt->methodOfDeath = MOD_ELECTRIC;
	bolt->splashMethodOfDeath = MOD_ELECTRIC_SPLASH;
#else
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
#endif
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================

#ifndef TMNTWEAPONS
/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================
#endif

/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;
#ifdef TMNTWEAPONS
	vec3_t          mins = { -8, -8, -8 };
	vec3_t			maxs = { 8, 8, 8 };
#endif

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
#ifdef TMNTWEAPONS
	bolt->damage = 50;
	bolt->splashDamage = 50;
#else
	bolt->damage = 100;
	bolt->splashDamage = 100;
#endif
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

#ifdef TMNTWEAPONS // XREAL r2785
	// make the rocket shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 25;
	bolt->die = G_Missile_Die;
#endif

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;
#ifdef IOQ3ZTM // IOQ3BUGFIX: Fix Grapple-Attack player animation.
	self->client->ps.pm_flags |= PMF_GRAPPLE_SHOT;
#endif

	return hook;
}


#ifndef TMNTWEAPONS // missionpack
#ifdef MISSIONPACK
/*
=================
fire_nail
=================
*/
#define NAILGUN_SPREAD	500

gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA( start, 8192 * 16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );

	scale = 555 + random() * 1800;
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	return bolt;
}	


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}
#endif
#endif

#ifdef SP_NPC
#define MOUSER_SHOT_SPEED 960
#define FLYBOT_SHOT_SPEED 1200

//
// ank homing missile, bat & sealord fire
//
void ank_shot_think( gentity_t *ent ) {
	vec3_t		targetdir, forward, midbody;
	trace_t		tr;
	gentity_t	*enemy;
	int			length;
	enemy=ent->ns.enemy;
	if (enemy->health<=0) return;
		VectorCopy(ent->s.pos.trDelta, forward);
	VectorNormalize(forward);
	midbody[0] = enemy->r.currentOrigin[0] +
		(enemy->r.mins[0] + enemy->r.maxs[0]) * 0.5;
	midbody[1] = enemy->r.currentOrigin[1] +
		(enemy->r.mins[1] + enemy->r.maxs[1]) * 0.5;
	midbody[2] = enemy->r.currentOrigin[2] +
		(enemy->r.mins[2] + enemy->r.maxs[2]) * 0.5;
	VectorSubtract(midbody, ent->r.currentOrigin, targetdir);
	length = VectorLength(targetdir);
	targetdir[0] /= length;
	targetdir[1] /= length;
	targetdir[2] /= length;
	if ( DotProduct(forward, targetdir) < 0.93 ) return;
	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL,
		enemy->r.currentOrigin, ENTITYNUM_NONE, MASK_SHOT );
	if ( enemy != &g_entities[tr.entityNum] ) return;
	ent->nextthink = level.time+1;
	VectorMA(forward, 0.08, targetdir, targetdir);		// 0.05 es mas suave!
	VectorNormalize(targetdir);
	VectorScale(targetdir, MOUSER_SHOT_SPEED, ent->s.pos.trDelta);
}

/*
=================
fire_ank_shot
=================
*/
gentity_t *fire_mouser_shot (gentity_t *self, vec3_t start, vec3_t dir, gentity_t *enemy) {
#if 1 // #ifdef TMNTWEAPONS // MOD
	return NULL;
#else
	gentity_t	*bolt;
	VectorNormalize (dir);
	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 1;
	bolt->think = ank_shot_think;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FIREBALL;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->ns.enemy=enemy;
	bolt->damage = 30*(1+0.25*npc_skill);
	bolt->splashDamage = 15*(1+0.25*npc_skill);
	bolt->splashRadius = 40;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, MOUSER_SHOT_SPEED, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);
	return bolt;
#endif
}

/*
=================
fire_bat
=================
*/
gentity_t *fire_flybot_shot (gentity_t *self, vec3_t start, vec3_t dir) {
#if 1 // #ifdef TMNTWEAPONS // MOD
	return NULL;
#else
	gentity_t	*bolt;
	VectorNormalize (dir);
	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BAT;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 30*(1+0.25*npc_skill);;
	bolt->splashDamage = 15*(1+0.25*npc_skill);;
	bolt->splashRadius = 40;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, FLYBOT_SHOT_SPEED, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);
	return bolt;
#endif
}
#endif
