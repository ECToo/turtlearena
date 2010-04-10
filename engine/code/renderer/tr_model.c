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
// tr_models.c -- model loading and caching

#ifdef TMNT_GAME_MODELS
#include "../server/server.h"
#endif

#if defined TMNT_GAME_MODELS && defined DEDICATED
#define RENDERLESS_MODELS // ZTM: Ded server needs the tags for melee attacks
#endif

#ifdef RENDERLESS_MODELS
// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MD4,
#ifdef RAVENMD4
	SF_MDR,
#endif
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MD4,
#ifdef RAVENMD4
	MOD_MDR
#endif
} modtype_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;				// model = tr.models[model->index]

	int			dataSize;			// just for listing purposes
	// ZTM: Not needed by ded server
	//bmodel_t	*bmodel;			// only if type == MOD_BRUSH
	md3Header_t	*md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	void	*md4;				// only if type == (MOD_MD4 | MOD_MDR)

	int			 numLods;
} model_t;

#define	MAX_MOD_KNOWN	1024

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct {
#if 0
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visCount;		// incremented every time a new vis cluster is entered
	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						smpFrame;		// toggles from 0 to 1 every endFrame

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				worldMapLoaded;
	world_t					*world;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xff
	image_t					*identityLightImage;	// full of tr.identityLightByte

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*projectionShadowShader;

	shader_t				*flareShader;
	shader_t				*sunShader;

	int						numLightmaps;
	image_t					*lightmaps[MAX_LIGHTMAPS];

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t					*currentModel;

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			or;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue
#endif

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

#if 0
	int						numImages;
	image_t					*images[MAX_DRAWIMAGES];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];
#endif
} trGlobals_t;

trGlobals_t tr;

void R_ModelInit( void );

/*
===============
R_Init
===============
*/
void R_Init( void ) {
	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );

	R_ModelInit();
}
#else
#include "tr_local.h"
#endif

#define	LL(x) x=LittleLong(x)

static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *name );
static qboolean R_LoadMD4 (model_t *mod, void *buffer, const char *name );
#ifdef RAVENMD4
static qboolean R_LoadMDR (model_t *mod, void *buffer, int filesize, const char *name );
void MC_UnCompress(float mat[3][4],const unsigned char * comp);
#endif

model_t	*loadmodel;

/*
** R_GetModelByHandle
*/
model_t	*R_GetModelByHandle( qhandle_t index ) {
	model_t		*mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

#ifdef RENDERLESS_MODELS
	mod = Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
#else
	mod = ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
#endif
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel( const char *name ) {
	model_t		*mod;
	union {
		unsigned *u;
		void *v;
	} buf;
	int			lod;
	int			ident;
	qboolean	loaded = qfalse;
	qhandle_t	hModel;
	int			numLoaded;
	char		*fext, defex[] = "md3", filename[MAX_QPATH], namebuf[MAX_QPATH+20];

	if ( !name || !name[0] ) {
#ifdef RENDERLESS_MODELS
		Com_Printf( "RE_RegisterModel: NULL name\n" );
#else
		ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
#endif
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	//
	// search the currently loaded models
	//
	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
		mod = tr.models[hModel];
		if ( !strcmp( mod->name, name ) ) {
			if( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
#ifdef RENDERLESS_MODELS
		Com_Printf( "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
#else
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
#endif
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );


#ifndef RENDERLESS_MODELS
	// make sure the render thread is stopped
	R_SyncRenderThread();
#endif

	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');
	if(!fext)
		fext = defex;
	else
	{
		*fext = '\0';
		fext++;
	}

#ifdef RAVENMD4
#ifdef IOQ3ZTM // Always atemp to load MDR first!
	while (qtrue)
#else
	if(!Q_stricmp(fext, "mdr"))
#endif
	{
		int filesize;
		
#ifdef IOQ3ZTM // Always atempt to load MDR first!
		Com_sprintf(namebuf, sizeof(namebuf), "%s.mdr", filename);
#ifdef RENDERLESS_MODELS
		filesize = FS_ReadFile(namebuf, (void **) &buf.v);
#else
		filesize = ri.FS_ReadFile(namebuf, (void **) &buf.v);
#endif
#else
#ifdef RENDERLESS_MODELS
		filesize = FS_ReadFile(name, (void **) &buf.v);
#else
		filesize = ri.FS_ReadFile(name, (void **) &buf.v);
#endif
#endif
		if(!buf.u)
		{
#ifdef IOQ3ZTM // Always atemp to load MDR first!
			// They didn't say we wanted a mdr model, so try MD3.
			if (Q_stricmp(fext, "mdr") != 0)
			{
				//ri.Printf (PRINT_WARNING,"RE_RegisterModel: couldn't load %s\n", namebuf);
				break;
			}
#endif
#ifdef RENDERLESS_MODELS
			Com_Printf ("RE_RegisterModel: couldn't load %s\n", name);
#else
			ri.Printf (PRINT_WARNING,"RE_RegisterModel: couldn't load %s\n", name);
#endif
			mod->type = MOD_BAD;
			return 0;
		}
		
		ident = LittleLong(*(unsigned *)buf.u);
		if(ident == MDR_IDENT)
#ifdef IOQ3ZTM // Always atemp to load MDR first!
			loaded = R_LoadMDR(mod, buf.u, filesize, namebuf);
#else
			loaded = R_LoadMDR(mod, buf.u, filesize, name);
#endif

#ifdef RENDERLESS_MODELS
		FS_FreeFile (buf.v);
#else
		ri.FS_FreeFile (buf.v);
#endif
		
		if(!loaded)
		{
#ifdef IOQ3ZTM // Always atemp to load MDR first!
#ifdef RENDERLESS_MODELS
			Com_Printf("RE_RegisterModel: couldn't load mdr file %s\n", namebuf);
#else
			ri.Printf(PRINT_WARNING,"RE_RegisterModel: couldn't load mdr file %s\n", namebuf);
#endif
#else
#ifdef RENDERLESS_MODELS
			Com_Printf("RE_RegisterModel: couldn't load mdr file %s\n", name);
#else
			ri.Printf(PRINT_WARNING,"RE_RegisterModel: couldn't load mdr file %s\n", name);
#endif
#endif
			mod->type = MOD_BAD;
			return 0;
		}
		
		return mod->index;
	}
#endif

	fext = defex;

	for ( lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod-- ) {
		if ( lod )
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

#ifdef RENDERLESS_MODELS
		FS_ReadFile( namebuf, &buf.v );
#else
		ri.FS_ReadFile( namebuf, &buf.v );
#endif
		if ( !buf.u ) {
			continue;
		}
		
		loadmodel = mod;
		
		ident = LittleLong(*(unsigned *)buf.u);
		if ( ident == MD4_IDENT ) {
			loaded = R_LoadMD4( mod, buf.u, name );
		} else {
			if ( ident != MD3_IDENT ) {
#ifdef RENDERLESS_MODELS
				Com_Printf ("RE_RegisterModel: unknown fileid for %s\n", name);
#else
				ri.Printf (PRINT_WARNING,"RE_RegisterModel: unknown fileid for %s\n", name);
#endif
				goto fail;
			}

			loaded = R_LoadMD3( mod, lod, buf.u, name );
		}
		
#ifdef RENDERLESS_MODELS
		FS_FreeFile (buf.v);
#else
		ri.FS_FreeFile (buf.v);
#endif

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
//			if ( lod <= r_lodbias->integer ) {
//				break;
//			}
		}
	}

	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod+1];
		}

		return mod->index;
	}
#ifdef _DEBUG
	else {
#ifdef RENDERLESS_MODELS
		Com_Printf ("RE_RegisterModel: couldn't load %s\n", name);
#else
		ri.Printf (PRINT_WARNING,"RE_RegisterModel: couldn't load %s\n", name);
#endif
	}
#endif

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	return 0;
}


/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name ) {
	int					i, j;
	md3Header_t			*pinmodel;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
	int					version;
	int					size;

	pinmodel = (md3Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD3_VERSION) {
#ifdef RENDERLESS_MODELS
		Com_Printf( "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
#else
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
#endif
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
#ifdef RENDERLESS_MODELS
	mod->md3[lod] = Hunk_Alloc( size, h_low );
#else
	mod->md3[lod] = ri.Hunk_Alloc( size, h_low );
#endif

	Com_Memcpy (mod->md3[lod], buffer, LittleLong(pinmodel->ofsEnd) );

    LL(mod->md3[lod]->ident);
    LL(mod->md3[lod]->version);
    LL(mod->md3[lod]->numFrames);
    LL(mod->md3[lod]->numTags);
    LL(mod->md3[lod]->numSurfaces);
    LL(mod->md3[lod]->ofsFrames);
    LL(mod->md3[lod]->ofsTags);
    LL(mod->md3[lod]->ofsSurfaces);
    LL(mod->md3[lod]->ofsEnd);

	if ( mod->md3[lod]->numFrames < 1 ) {
#ifndef RENDERLESS_MODELS
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
#endif
		return qfalse;
	}
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
#ifndef RENDERLESS_MODELS
#ifdef IOQ3ZTM // ZTM: Show the name of the surface, it is helpful.
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on %s (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->name[0] == '\0' ? "a surface" : surf->name,
				surf->numVerts );
			return qfalse;
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on %s (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->name[0] == '\0' ? "a surface" : surf->name,
				surf->numTriangles );
			return qfalse;
		}
#else
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}
#endif
#endif
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
#ifdef RENDERLESS_MODELS
			shader->shaderIndex = 0;
#else
            shader_t	*sh;

            sh = R_FindShader( shader->name, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
#endif
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}


#ifdef RAVENMD4

/*
=================
R_LoadMDR
=================
*/
static qboolean R_LoadMDR( model_t *mod, void *buffer, int filesize, const char *mod_name ) 
{
	int					i, j, k, l;
	mdrHeader_t			*pinmodel, *mdr;
        mdrFrame_t			*frame;
	mdrLOD_t			*lod, *curlod;
	mdrSurface_t			*surf, *cursurf;
	mdrTriangle_t			*tri, *curtri;
	mdrVertex_t			*v, *curv;
	mdrWeight_t			*weight, *curweight;
	mdrTag_t			*tag, *curtag;
	int					size;
#ifndef RENDERLESS_MODELS
	shader_t			*sh;
#endif

	pinmodel = (mdrHeader_t *)buffer;

	pinmodel->version = LittleLong(pinmodel->version);
	if (pinmodel->version != MDR_VERSION) 
	{
#ifdef RENDERLESS_MODELS
		Com_Printf("R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION);
#else
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION);
#endif
		return qfalse;
	}

	size = LittleLong(pinmodel->ofsEnd);
	
	if(size > filesize)
	{
#ifdef RENDERLESS_MODELS
		Com_Printf("R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name);
#else
		ri.Printf(PRINT_WARNING, "R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name);
#endif
		return qfalse;
	}
	
	mod->type = MOD_MDR;

	LL(pinmodel->numFrames);
	LL(pinmodel->numBones);
	LL(pinmodel->ofsFrames);

	// This is a model that uses some type of compressed Bones. We don't want to uncompress every bone for each rendered frame
	// over and over again, we'll uncompress it in this function already, so we must adjust the size of the target md4.
	if(pinmodel->ofsFrames < 0)
	{
		// mdrFrame_t is larger than mdrCompFrame_t:
		size += pinmodel->numFrames * sizeof(frame->name);
		// now add enough space for the uncompressed bones.
		size += pinmodel->numFrames * pinmodel->numBones * ((sizeof(mdrBone_t) - sizeof(mdrCompBone_t)));
	}
	
	// simple bounds check
	if(pinmodel->numBones < 0 ||
		sizeof(*mdr) + pinmodel->numFrames * (sizeof(*frame) + (pinmodel->numBones - 1) * sizeof(*frame->bones)) > size)
	{
#ifdef RENDERLESS_MODELS
		Com_Printf("R_LoadMDR: %s has broken structure.\n", mod_name);
#else
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
		return qfalse;
	}

	mod->dataSize += size;
#ifdef RENDERLESS_MODELS
	mod->md4 = mdr = Hunk_Alloc( size, h_low );
#else
	mod->md4 = mdr = ri.Hunk_Alloc( size, h_low );
#endif

	// Copy all the values over from the file and fix endian issues in the process, if necessary.
	
	mdr->ident = LittleLong(pinmodel->ident);
	mdr->version = pinmodel->version;	// Don't need to swap byte order on this one, we already did above.
	Q_strncpyz(mdr->name, pinmodel->name, sizeof(mdr->name));
	mdr->numFrames = pinmodel->numFrames;
	mdr->numBones = pinmodel->numBones;
	mdr->numLODs = LittleLong(pinmodel->numLODs);
	mdr->numTags = LittleLong(pinmodel->numTags);
	// We don't care about the other offset values, we'll generate them ourselves while loading.

	mod->numLods = mdr->numLODs;

	if ( mdr->numFrames < 1 ) 
	{
#ifndef RENDERLESS_MODELS
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has no frames\n", mod_name);
#endif
		return qfalse;
	}

	/* The first frame will be put into the first free space after the header */
	frame = (mdrFrame_t *)(mdr + 1);
	mdr->ofsFrames = (int)((byte *) frame - (byte *) mdr);
		
	if (pinmodel->ofsFrames < 0)
	{
		mdrCompFrame_t *cframe;
				
		// compressed model...				
		cframe = (mdrCompFrame_t *)((byte *) pinmodel - pinmodel->ofsFrames);
		
		for(i = 0; i < mdr->numFrames; i++)
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(cframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(cframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j]);
			}

			frame->radius = LittleFloat(cframe->radius);
			frame->name[0] = '\0';	// No name supplied in the compressed version.
			
			for(j = 0; j < mdr->numBones; j++)
			{
				for(k = 0; k < (sizeof(cframe->bones[j].Comp) / 2); k++)
				{
					// Do swapping for the uncompressing functions. They seem to use shorts
					// values only, so I assume this will work. Never tested it on other
					// platforms, though.
					
					((unsigned short *)(cframe->bones[j].Comp))[k] =
						LittleShort( ((unsigned short *)(cframe->bones[j].Comp))[k] );
				}
				
				/* Now do the actual uncompressing */
				MC_UnCompress(frame->bones[j].matrix, cframe->bones[j].Comp);
			}
			
			// Next Frame...
			cframe = (mdrCompFrame_t *) &cframe->bones[j];
			frame = (mdrFrame_t *) &frame->bones[j];
		}
	}
	else
	{
		mdrFrame_t *curframe;
		
		// uncompressed model...
		//
    
		curframe = (mdrFrame_t *)((byte *) pinmodel + pinmodel->ofsFrames);
		
		// swap all the frames
		for ( i = 0 ; i < mdr->numFrames ; i++) 
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
			}
			
			frame->radius = LittleFloat(curframe->radius);
			Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));
			
			for (j = 0; j < (int) (mdr->numBones * sizeof(mdrBone_t) / 4); j++) 
			{
				((float *)frame->bones)[j] = LittleFloat( ((float *)curframe->bones)[j] );
			}
			
			curframe++;
			frame++;
		}
	}
	
	// frame should now point to the first free address after all frames.
	lod = (mdrLOD_t *) frame;
	mdr->ofsLODs = (int) ((byte *) lod - (byte *)mdr);
	
	curlod = (mdrLOD_t *)((byte *) pinmodel + LittleLong(pinmodel->ofsLODs));
		
	// swap all the LOD's
	for ( l = 0 ; l < mdr->numLODs ; l++)
	{
		// simple bounds check
		if((byte *) (lod + 1) > (byte *) mdr + size)
		{
#ifndef RENDERLESS_MODELS
			ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
			return qfalse;
		}

		lod->numSurfaces = LittleLong(curlod->numSurfaces);
		
		// swap all the surfaces
		surf = (mdrSurface_t *) (lod + 1);
		lod->ofsSurfaces = (int)((byte *) surf - (byte *) lod);
		cursurf = (mdrSurface_t *) ((byte *)curlod + LittleLong(curlod->ofsSurfaces));
		
		for ( i = 0 ; i < lod->numSurfaces ; i++)
		{
			// simple bounds check
			if((byte *) (surf + 1) > (byte *) mdr + size)
			{
#ifndef RENDERLESS_MODELS
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
				return qfalse;
			}

			// first do some copying stuff
			
			surf->ident = SF_MDR;
			Q_strncpyz(surf->name, cursurf->name, sizeof(surf->name));
			Q_strncpyz(surf->shader, cursurf->shader, sizeof(surf->shader));
			
			surf->ofsHeader = (byte *) mdr - (byte *) surf;
			
			surf->numVerts = LittleLong(cursurf->numVerts);
			surf->numTriangles = LittleLong(cursurf->numTriangles);
			// numBoneReferences and BoneReferences generally seem to be unused
			
#ifndef RENDERLESS_MODELS
			// now do the checks that may fail.
#ifdef IOQ3ZTM // ZTM: Show the name of the surface, it is helpful.
			if ( surf->numVerts > SHADER_MAX_VERTEXES )
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i verts on %s (%i)\n",
					  mod_name, SHADER_MAX_VERTEXES, surf->name[0] == '\0' ? "a surface" : surf->name,
					  surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES )
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i triangles on %s (%i)\n",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->name[0] == '\0' ? "a surface" : surf->name,
					  surf->numTriangles );
				return qfalse;
			}
#else
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) 
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i verts on a surface (%i)",
					  mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) 
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i triangles on a surface (%i)",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
				return qfalse;
			}
#endif
#endif
			// lowercase the surface name so skin compares are faster
			Q_strlwr( surf->name );

#ifdef RENDERLESS_MODELS
			surf->shaderIndex = 0;
#else
			// register the shaders
			sh = R_FindShader(surf->shader, LIGHTMAP_NONE, qtrue);
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
#endif
			
			// now copy the vertexes.
			v = (mdrVertex_t *) (surf + 1);
			surf->ofsVerts = (int)((byte *) v - (byte *) surf);
			curv = (mdrVertex_t *) ((byte *)cursurf + LittleLong(cursurf->ofsVerts));
			
			for(j = 0; j < surf->numVerts; j++)
			{
				LL(curv->numWeights);
			
				// simple bounds check
				if(curv->numWeights < 0 || (byte *) (v + 1) + (curv->numWeights - 1) * sizeof(*weight) > (byte *) mdr + size)
				{
#ifndef RENDERLESS_MODELS
					ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
					return qfalse;
				}

				v->normal[0] = LittleFloat(curv->normal[0]);
				v->normal[1] = LittleFloat(curv->normal[1]);
				v->normal[2] = LittleFloat(curv->normal[2]);
				
				v->texCoords[0] = LittleFloat(curv->texCoords[0]);
				v->texCoords[1] = LittleFloat(curv->texCoords[1]);
				
				v->numWeights = curv->numWeights;
				weight = &v->weights[0];
				curweight = &curv->weights[0];
				
				// Now copy all the weights
				for(k = 0; k < v->numWeights; k++)
				{
					weight->boneIndex = LittleLong(curweight->boneIndex);
					weight->boneWeight = LittleFloat(curweight->boneWeight);
					
					weight->offset[0] = LittleFloat(curweight->offset[0]);
					weight->offset[1] = LittleFloat(curweight->offset[1]);
					weight->offset[2] = LittleFloat(curweight->offset[2]);
					
					weight++;
					curweight++;
				}
				
				v = (mdrVertex_t *) weight;
				curv = (mdrVertex_t *) curweight;
			}
						
			// we know the offset to the triangles now:
			tri = (mdrTriangle_t *) v;
			surf->ofsTriangles = (int)((byte *) tri - (byte *) surf);
			curtri = (mdrTriangle_t *)((byte *) cursurf + LittleLong(cursurf->ofsTriangles));
			
			// simple bounds check
			if(surf->numTriangles < 0 || (byte *) (tri + surf->numTriangles) > (byte *) mdr + size)
			{
#ifndef RENDERLESS_MODELS
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
				return qfalse;
			}

			for(j = 0; j < surf->numTriangles; j++)
			{
				tri->indexes[0] = LittleLong(curtri->indexes[0]);
				tri->indexes[1] = LittleLong(curtri->indexes[1]);
				tri->indexes[2] = LittleLong(curtri->indexes[2]);
				
				tri++;
				curtri++;
			}
			
			// tri now points to the end of the surface.
			surf->ofsEnd = (byte *) tri - (byte *) surf;
			surf = (mdrSurface_t *) tri;

			// find the next surface.
			cursurf = (mdrSurface_t *) ((byte *) cursurf + LittleLong(cursurf->ofsEnd));
		}

		// surf points to the next lod now.
		lod->ofsEnd = (int)((byte *) surf - (byte *) lod);
		lod = (mdrLOD_t *) surf;

		// find the next LOD.
		curlod = (mdrLOD_t *)((byte *) curlod + LittleLong(curlod->ofsEnd));
	}
	
	// lod points to the first tag now, so update the offset too.
	tag = (mdrTag_t *) lod;
	mdr->ofsTags = (int)((byte *) tag - (byte *) mdr);
	curtag = (mdrTag_t *) ((byte *)pinmodel + LittleLong(pinmodel->ofsTags));

	// simple bounds check
	if(mdr->numTags < 0 || (byte *) (tag + mdr->numTags) > (byte *) mdr + size)
	{
#ifndef RENDERLESS_MODELS
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
#endif
		return qfalse;
	}
	
	for (i = 0 ; i < mdr->numTags ; i++)
	{
		tag->boneIndex = LittleLong(curtag->boneIndex);
		Q_strncpyz(tag->name, curtag->name, sizeof(tag->name));
		
		tag++;
		curtag++;
	}
	
	// And finally we know the real offset to the end.
	mdr->ofsEnd = (int)((byte *) tag - (byte *) mdr);

	// phew! we're done.
	
	return qtrue;
}
#endif

/*
=================
R_LoadMD4
=================
*/

static qboolean R_LoadMD4( model_t *mod, void *buffer, const char *mod_name ) {
	int					i, j, k, lodindex;
	md4Header_t			*pinmodel, *md4;
    md4Frame_t			*frame;
	md4LOD_t			*lod;
	md4Surface_t		*surf;
	md4Triangle_t		*tri;
	md4Vertex_t			*v;
	int					version;
	int					size;
#ifndef RENDERLESS_MODELS
	shader_t			*sh;
#endif
	int					frameSize;

	pinmodel = (md4Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD4_VERSION) {
#ifndef RENDERLESS_MODELS
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD4_VERSION);
#endif
		return qfalse;
	}

	mod->type = MOD_MD4;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
#ifdef RENDERLESS_MODELS
	md4 = mod->md4 = Hunk_Alloc( size, h_low );
#else
	md4 = mod->md4 = ri.Hunk_Alloc( size, h_low );
#endif

	Com_Memcpy(md4, buffer, size);

    LL(md4->ident);
    LL(md4->version);
    LL(md4->numFrames);
    LL(md4->numBones);
    LL(md4->numLODs);
    LL(md4->ofsFrames);
    LL(md4->ofsLODs);
    md4->ofsEnd = size;

	if ( md4->numFrames < 1 ) {
#ifndef RENDERLESS_MODELS
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has no frames\n", mod_name );
#endif
		return qfalse;
	}

    // we don't need to swap tags in the renderer, they aren't used
    
	// swap all the frames
	frameSize = (size_t)( &((md4Frame_t *)0)->bones[ md4->numBones ] );
    for ( i = 0 ; i < md4->numFrames ; i++, frame++) {
	    frame = (md4Frame_t *) ( (byte *)md4 + md4->ofsFrames + i * frameSize );
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
		for ( j = 0 ; j < md4->numBones * sizeof( md4Bone_t ) / 4 ; j++ ) {
			((float *)frame->bones)[j] = LittleFloat( ((float *)frame->bones)[j] );
		}
	}

	// swap all the LOD's
	lod = (md4LOD_t *) ( (byte *)md4 + md4->ofsLODs );
	for ( lodindex = 0 ; lodindex < md4->numLODs ; lodindex++ ) {

		// swap all the surfaces
		surf = (md4Surface_t *) ( (byte *)lod + lod->ofsSurfaces );
		for ( i = 0 ; i < lod->numSurfaces ; i++) {
			LL(surf->ident);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			
#ifndef RENDERLESS_MODELS
#ifdef IOQ3ZTM // ZTM: Show the name of the surface, it is helpful.
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				ri.Printf(PRINT_WARNING, "R_LoadMD4: %s has more than %i verts on %s (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->name[0] == '\0' ? "a surface" : surf->name,
					surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				ri.Printf(PRINT_WARNING, "R_LoadMD4: %s has more than %i triangles on %s (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->name[0] == '\0' ? "a surface" : surf->name,
					surf->numTriangles );
				return qfalse;
			}
#else
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			}
#endif
#endif

			// change to surface identifier
			surf->ident = SF_MD4;

			// lowercase the surface name so skin compares are faster
			Q_strlwr( surf->name );
		
			// register the shaders
#ifdef RENDERLESS_MODELS
			surf->shaderIndex = 0;
#else
			sh = R_FindShader( surf->shader, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
#endif

			// swap all the triangles
			tri = (md4Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			// FIXME
			// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
			// in for reference.
			//v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts + 12);
			v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts);
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
				   v->weights[k].offset[0] = LittleFloat( v->weights[k].offset[0] );
				   v->weights[k].offset[1] = LittleFloat( v->weights[k].offset[1] );
				   v->weights[k].offset[2] = LittleFloat( v->weights[k].offset[2] );
				}
				// FIXME
				// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
				// in for reference.
				//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
				v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights]);
			}

			// find the next surface
			surf = (md4Surface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (md4LOD_t *)( (byte *)lod + lod->ofsEnd );
	}

	return qtrue;
}



//=============================================================================

#ifndef RENDERLESS_MODELS
/*
** RE_BeginRegistration
*/
void RE_BeginRegistration( glconfig_t *glconfigOut ) {

	R_Init();

	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;		// force markleafs to regenerate
	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
//	RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
}
#endif

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void ) {
	model_t		*mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;

#ifdef TMNT_GAME_MODELS
	// Models in game are no longer valid
	if (Cvar_VariableValue("sv_running") && gvm)
	{
		int i;

		for (i = 0; i < sv_maxclients->integer; i++)
		{
			if (svs.clients[i].state != CS_ACTIVE) {
				continue;
			}

			// call prog code to reload player model
			VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, i );
		}
	}
#endif
}


#ifndef RENDERLESS_MODELS
/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
			if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] ) {
				lods++;
			}
		}
		ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );

#if	0		// not working right with new hunk
	if ( tr.world ) {
		ri.Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}
#endif


//=============================================================================


/*
================
R_GetTag
================
*/
static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

#ifdef RAVENMD4
void R_GetAnimTag( mdrHeader_t *mod, int framenum, const char *tagName, md3Tag_t * dest) 
{
	int				i, j, k;
	int				frameSize;
	mdrFrame_t		*frame;
	mdrTag_t		*tag;

	if ( framenum >= mod->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (mdrTag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ )
	{
		if ( !strcmp( tag->name, tagName ) )
		{
			Q_strncpyz(dest->name, tag->name, sizeof(dest->name));

			// uncompressed model...
			//
			frameSize = (long)( &((mdrFrame_t *)0)->bones[ mod->numBones ] );
			frame = (mdrFrame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );

			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
			}

			dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
			dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
			dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];				

			return;
		}
	}

	AxisClear( dest->axis );
	VectorClear( dest->origin );
	strcpy(dest->name,"");
}
#endif

/*
================
R_LerpTag
================
*/
int R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName ) {
	md3Tag_t	*start, *end;
#ifdef RAVENMD4
	md3Tag_t	start_space, end_space;
#endif
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->md3[0] )
	{
#ifdef RAVENMD4
		if(model->md4)
		{
			start = &start_space;
			end = &end_space;
			R_GetAnimTag((mdrHeader_t *) model->md4, startFrame, tagName, start);
			R_GetAnimTag((mdrHeader_t *) model->md4, endFrame, tagName, end);
		}
		else
#endif
		{

			AxisClear( tag->axis );
			VectorClear( tag->origin );
			return qfalse;

		}
	}
	else
	{
		start = R_GetTag( model->md3[0], startFrame, tagName );
		end = R_GetTag( model->md3[0], endFrame, tagName );
		if ( !start || !end ) {
			AxisClear( tag->axis );
			VectorClear( tag->origin );
			return qfalse;
		}
	}
	
	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}


/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t		*model;
	md3Header_t	*header;
	md3Frame_t	*frame;

	model = R_GetModelByHandle( handle );

#ifndef RENDERLESS_MODELS
	if ( model->bmodel ) {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}
#endif

	if ( !model->md3[0] ) {
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}

	header = model->md3[0];

	frame = (md3Frame_t *)( (byte *)header + header->ofsFrames );

	VectorCopy( frame->bounds[0], mins );
	VectorCopy( frame->bounds[1], maxs );
}

#ifdef RAVENMD4
//
#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

//
#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;

	val=(int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;
}
#endif
