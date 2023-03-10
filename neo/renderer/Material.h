/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2020 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __MATERIAL_H__
#define __MATERIAL_H__


// RB: define this to use the id Tech 4.5 UI interface for ImGui instead of OpenGL or Vulkan
// this allows to have the com_showFPS stats in screenshots

#define IMGUI_BFGUI 1

/*
===============================================================================

	Material

===============================================================================
*/

class idImage;
class idCinematic;
class idUserInterface;

// moved from image.h for default parm
typedef enum
{
	TF_LINEAR,
	TF_NEAREST,
	TF_NEAREST_MIPMAP,		// RB: no linear interpolation but explicit mip-map levels for hierarchical depth buffer
	TF_DEFAULT				// use the user-specified r_textureFilter
} textureFilter_t;

typedef enum
{
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures
	TR_CLAMP_TO_ZERO_ALPHA	// guarantee 0 alpha edge for projected textures
} textureRepeat_t;

typedef struct
{
	int		stayTime;		// msec for no change
	int		fadeTime;		// msec to fade vertex colors over
	float	start[4];		// vertex color at spawn (possibly out of 0.0 - 1.0 range, will clamp after calc)
	float	end[4];			// vertex color at fade-out (possibly out of 0.0 - 1.0 range, will clamp after calc)
} decalInfo_t;

typedef enum
{
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
	DFRM_PARTICLE,
	DFRM_PARTICLE2,
	DFRM_TURB
} deform_t;

typedef enum
{
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
	DI_XRAY_RENDER,
	DI_REMOTE_RENDER,
	DI_GUI_RENDER,
	DI_RENDER_TARGET,
} dynamicidImage_t;

// note: keep opNames[] in sync with changes
typedef enum
{
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_TABLE,
	OP_TYPE_GT,
	OP_TYPE_GE,
	OP_TYPE_LT,
	OP_TYPE_LE,
	OP_TYPE_EQ,
	OP_TYPE_NE,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_SOUND
} expOpType_t;

typedef enum
{
	EXP_REG_TIME,

	EXP_REG_PARM0,
	EXP_REG_PARM1,
	EXP_REG_PARM2,
	EXP_REG_PARM3,
	EXP_REG_PARM4,
	EXP_REG_PARM5,
	EXP_REG_PARM6,
	EXP_REG_PARM7,
	EXP_REG_PARM8,
	EXP_REG_PARM9,
	EXP_REG_PARM10,
	EXP_REG_PARM11,

	EXP_REG_GLOBAL0,
	EXP_REG_GLOBAL1,
	EXP_REG_GLOBAL2,
	EXP_REG_GLOBAL3,
	EXP_REG_GLOBAL4,
	EXP_REG_GLOBAL5,
	EXP_REG_GLOBAL6,
	EXP_REG_GLOBAL7,

	EXP_REG_NUM_PREDEFINED
} expRegister_t;

typedef struct
{
	expOpType_t		opType;
	int				a, b, c;
} expOp_t;

typedef struct
{
	int				registers[4];
} colorStage_t;

typedef enum
{
	TG_EXPLICIT,
	TG_DIFFUSE_CUBE,
	TG_REFLECT_CUBE,
	TG_SKYBOX_CUBE,
	TG_WOBBLESKY_CUBE,
	TG_SCREEN,			// screen aligned, for mirrorRenders and screen space temporaries
	TG_SCREEN2,
	TG_GLASSWARP
} texgen_t;

typedef struct
{
	idCinematic* 		cinematic;
	idImage* 			image;
	texgen_t			texgen;
	bool				hasMatrix;
	int					matrix[2][3];	// we only allow a subset of the full projection matrix

	// dynamic image variables
	dynamicidImage_t	dynamic;
	int					width, height;
	int					dynamicFrameCount;
} textureStage_t;

// the order BUMP / DIFFUSE / SPECULAR is necessary for interactions to draw correctly on low end cards
typedef enum
{
	SL_AMBIENT,						// execute after lighting
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR,
	SL_COVERAGE,
} stageLighting_t;

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
typedef enum
{
	SVC_IGNORE,
	SVC_MODULATE,
	SVC_INVERSE_MODULATE
} stageVertexColor_t;

// SP Begin
typedef enum
{
	STENCIL_COMP_GREATER,
	STENCIL_COMP_GEQUAL,
	STENCIL_COMP_LESS,
	STENCIL_COMP_LEQUAL,
	STENCIL_COMP_EQUAL,
	STENCIL_COMP_NOTEQUAL,
	STENCIL_COMP_ALWAYS,
	STENCIL_COMP_NEVER
} stencilComp_t;

typedef enum
{
	STENCIL_OP_KEEP,
	STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INCRSAT,
	STENCIL_OP_DECRSAT,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCRWRAP,
	STENCIL_OP_DECRWRAP
} stencilOperation_t;

struct stencilStage_t
{
	// The value to be compared against (if Comp is anything else than always) and/or the value to be written to the buffer
	// (if either Pass, Fail or ZFail is set to replace).
	byte ref = 0;

	// An 8 bit mask as an 0�255 integer, used when comparing the reference value with the contents of the buffer
	// (referenceValue & readMask) comparisonFunction (stencilBufferValue & readMask).
	byte readMask = 255;

	// An 8 bit mask as an 0�255 integer, used when writing to the buffer.Note that, like other write masks,
	// it specifies which bits of stencil buffer will be affected by write
	// (i.e.WriteMask 0 means that no bits are affected and not that 0 will be written).
	byte writeMask = 255;

	// Function used to compare the reference value to the current contents of the buffer.
	stencilComp_t comp = STENCIL_COMP_ALWAYS;

	// What to do with the contents of the buffer if the stencil test(and the depth test) passes.
	stencilOperation_t pass = STENCIL_OP_KEEP;

	// What to do with the contents of the buffer if the stencil test fails.
	stencilOperation_t fail = STENCIL_OP_KEEP;

	// What to do with the contents of the buffer if the stencil test passes, but the depth test fails.
	stencilOperation_t zFail = STENCIL_OP_KEEP;
};
// SP End


static const int	MAX_FRAGMENT_IMAGES = 8;
static const int	MAX_VERTEX_PARMS = 4;

typedef struct
{
	int					vertexProgram;
	int					numVertexParms;
	int					vertexParms[MAX_VERTEX_PARMS][4];	// evaluated register indexes

	int					fragmentProgram;
	int					glslProgram;
	int					numFragmentProgramImages;
	idImage* 			fragmentProgramImages[MAX_FRAGMENT_IMAGES];
} newShaderStage_t;

typedef struct
{
	int					conditionRegister;	// if registers[conditionRegister] == 0, skip stage
	stageLighting_t		lighting;			// determines which passes interact with lights
	uint64				drawStateBits;
	colorStage_t		color;
	bool				hasAlphaTest;
	int					alphaTestRegister;
	textureStage_t		texture;
	stageVertexColor_t	vertexColor;
	bool				ignoreAlphaTest;	// this stage should act as translucent, even
	// if the surface is alpha tested
	float				privatePolygonOffset;	// a per-stage polygon offset

	stencilStage_t*     stencilStage;
	newShaderStage_t*	newStage;			// vertex / fragment program based stage
} shaderStage_t;

typedef enum
{
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum
{
	SS_SUBVIEW = -3,	// mirrors, viewscreens, etc
	SS_GUI = -2,		// guis
	SS_BAD = -1,
	SS_OPAQUE,			// opaque

	SS_PORTAL_SKY,

	SS_DECAL,			// scorch marks, etc.

	SS_FAR,
	SS_MEDIUM,			// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST,			// screen blood blobs

	SS_POST_PROCESS = 100	// after a screen copy to texture
} materialSort_t;

enum SubViewType : uint16_t
{
	SUBVIEW_NONE,
	SUBVIEW_MIRROR,
	SUBVIEW_DIRECT_PORTAL
};

typedef enum
{
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES			= 256;

const int MAX_TEXGEN_REGISTERS		= 4;

const int MAX_ENTITY_SHADER_PARMS	= 12;
const int MAX_GLOBAL_SHADER_PARMS	= 12;	// ? this looks like it should only be 8

// material flags
typedef enum
{
	MF_DEFAULTED				= BIT( 0 ),
	MF_POLYGONOFFSET			= BIT( 1 ),
	MF_NOSHADOWS				= BIT( 2 ),
	MF_FORCESHADOWS				= BIT( 3 ),
	MF_NOSELFSHADOW				= BIT( 4 ),
	MF_NOPORTALFOG				= BIT( 5 ),	 // this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE			= BIT( 6 ),	 // in use (visible) per editor
	// motorsep 11-23-2014; material LOD keys that define what LOD iteration the surface falls into
	MF_LOD1_SHIFT				= 7,
	MF_LOD1						= BIT( 7 ),	 // motorsep 11-24-2014; material flag for LOD1 iteration
	MF_LOD2						= BIT( 8 ),	 // motorsep 11-24-2014; material flag for LOD2 iteration
	MF_LOD3						= BIT( 9 ),	 // motorsep 11-24-2014; material flag for LOD3 iteration
	MF_LOD4						= BIT( 10 ), // motorsep 11-24-2014; material flag for LOD4 iteration
	MF_LOD_PERSISTENT			= BIT( 11 ), // motorsep 11-24-2014; material flag for persistent LOD iteration
	MF_GUITARGET				= BIT( 12 ), // Admer: this GUI surface is used to compute a GUI render map, but a GUI should NOT be drawn on it
	MF_AUTOGEN_TEMPLATE			= BIT( 13 ), // Admer: this material is a template for auto-generated templates
	MF_ORIGIN					= BIT( 14 ), // Admer: for origin brushes
} materialFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
typedef enum
{
	CONTENTS_SOLID				= BIT( 0 ),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT( 1 ),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT( 2 ),	// used for water
	CONTENTS_PLAYERCLIP			= BIT( 3 ),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT( 4 ),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT( 5 ),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT( 6 ),	// solid to IK
	CONTENTS_BLOOD				= BIT( 7 ),	// used to detect blood decals
	CONTENTS_BODY				= BIT( 8 ),	// used for actors
	CONTENTS_PROJECTILE			= BIT( 9 ),	// used for projectiles
	CONTENTS_CORPSE				= BIT( 10 ),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT( 11 ),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT( 12 ),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT( 13 ),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT( 14 ),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER	= BIT( 15 ),	// used for triggers that are activated by the flashlight

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT( 20 ),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT( 21 ),	// don't cut this brush with CSG operations in the editor
	CONTENTS_ORIGIN				= BIT( 22 ),

	CONTENTS_REMOVE_UTIL		= ~( CONTENTS_AREAPORTAL | CONTENTS_NOCSG )
} contentsFlags_t;

// surface types
const int NUM_SURFACE_BITS		= 4;
const int MAX_SURFACE_TYPES		= 1 << NUM_SURFACE_BITS;

typedef enum
{
	SURFTYPE_NONE,					// default type
	SURFTYPE_METAL,
	SURFTYPE_STONE,
	SURFTYPE_FLESH,
	SURFTYPE_WOOD,
	SURFTYPE_CARDBOARD,
	SURFTYPE_LIQUID,
	SURFTYPE_GLASS,
	SURFTYPE_PLASTIC,
	SURFTYPE_RICOCHET,
	SURFTYPE_10,
	SURFTYPE_11,
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
} surfTypes_t;

// surface flags
typedef enum
{
	SURF_TYPE_BIT0				= BIT( 0 ),	// encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1				= BIT( 1 ),	// "
	SURF_TYPE_BIT2				= BIT( 2 ),	// "
	SURF_TYPE_BIT3				= BIT( 3 ),	// "
	SURF_TYPE_MASK				= ( 1 << NUM_SURFACE_BITS ) - 1,

	SURF_NODAMAGE				= BIT( 4 ),	// never give falling damage
	SURF_SLICK					= BIT( 5 ),	// effects game physics
	SURF_COLLISION				= BIT( 6 ),	// collision surface
	SURF_LADDER					= BIT( 7 ),	// player can climb up this surface
	SURF_NOIMPACT				= BIT( 8 ),	// don't make missile explosions
	SURF_NOSTEPS				= BIT( 9 ),	// no footstep sounds
	SURF_DISCRETE				= BIT( 10 ),	// not clipped or merged by utilities
	SURF_NOFRAGMENT				= BIT( 11 ),	// dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL				= BIT( 12 )	// renderbump will draw this surface as 0x80 0x80 0x80, which
								  // won't collect light from any angle
} surfaceFlags_t;


class idSoundEmitter;

// RB: predefined Quake 1 light styles
static const char* predef_lightstyles[] =
{
	"m",
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"mamamamamama",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomno",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba"
};

static const char* predef_lightstylesinfo[] =
{
	"Normal",
	"Flicker A",
	"Slow Strong Pulse",
	"Candle A",
	"Fast Strobe",
	"Gentle Pulse",
	"Flicker B",
	"Candle B",
	"Candle C",
	"Slow Strobe",
	"Fluorescent Flicker",
	"Slow Pulse (no black)"
};
// RB end

class idMaterial : public idDecl
{
public:
	idMaterial();
	virtual				~idMaterial();

	virtual size_t		Size() const;
	virtual bool		SetDefaultText();
	virtual const char* DefaultDefinition() const;
	virtual bool		Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void		FreeData();
	virtual void		Print() const;

	//BSM Nerve: Added for material editor
	bool				Save( const char* fileName = NULL );

	// returns the internal image name for stage 0, which can be used
	// for the renderer CaptureRenderToImage() call
	// I'm not really sure why this needs to be virtual...
	virtual const char*	ImageName() const;

	void				ReloadImages( bool force, nvrhi::ICommandList* commandList ) const;

	// returns number of stages this material contains
	const int			GetNumStages() const
	{
		return numStages;
	}

	// if the material is simple, all that needs to be known are
	// the images for drawing.
	// These will either all return valid images, or all return NULL
	idImage* 			GetFastPathBumpImage() const
	{
		return fastPathBumpImage;
	};
	idImage* 			GetFastPathDiffuseImage() const
	{
		return fastPathDiffuseImage;
	};
	idImage* 			GetFastPathSpecularImage() const
	{
		return fastPathSpecularImage;
	};

	// get a specific stage
	const shaderStage_t* GetStage( const int index ) const
	{
		assert( index >= 0 && index < numStages );
		return &stages[index];
	}

	// get the first bump map stage, or NULL if not present.
	// used for bumpy-specular
	const shaderStage_t* GetBumpStage() const;

	// returns true if the material will draw anything at all.  Triggers, portals,
	// etc, will not have anything to draw.  A not drawn surface can still castShadow,
	// which can be used to make a simplified shadow hull for a complex object set
	// as noShadow
	bool				IsDrawn() const
	{
		return ( numStages > 0 || entityGui != 0 || gui != NULL );
	}

	// returns true if the material will draw any non light interaction stages
	bool				HasAmbient() const
	{
		return ( numAmbientStages > 0 );
	}

	// returns true if material has a gui
	bool				HasGui() const
	{
		return ( entityGui != 0 || gui != NULL );
	}

	// returns true if the material will generate another view, either as
	// a mirror or dynamic rendered image
	bool				HasSubview() const
	{
		return hasSubview;
	}

	bool                IsPortalSubView() const
	{
		return subViewType == SubViewType::SUBVIEW_DIRECT_PORTAL;
	}

	bool                IsMirrorSubView() const
	{
		return subViewType == SubViewType::SUBVIEW_MIRROR;
	}

	// returns true if the material will generate shadows, not making a
	// distinction between global and no-self shadows
	bool				SurfaceCastsShadow() const
	{
		return TestMaterialFlag( MF_FORCESHADOWS ) || !TestMaterialFlag( MF_NOSHADOWS );
	}

	// returns true if the material will generate interactions with fog/blend lights
	// All non-translucent surfaces receive fog unless they are explicitly noFog
	bool				ReceivesFog() const
	{
		return ( IsDrawn() && !noFog && coverage != MC_TRANSLUCENT );
	}

	// returns true if the material will generate interactions with normal lights
	// Many special effect surfaces don't have any bump/diffuse/specular
	// stages, and don't interact with lights at all
	bool				ReceivesLighting() const
	{
		return numAmbientStages != numStages;
	}

	// returns true if the material should generate interactions on sides facing away
	// from light centers, as with noshadow and noselfshadow options
	bool				ReceivesLightingOnBackSides() const
	{
		return ( materialFlags & ( MF_NOSELFSHADOW | MF_NOSHADOWS ) ) != 0;
	}

	// Standard two-sided triangle rendering won't work with bump map lighting, because
	// the normal and tangent vectors won't be correct for the back sides.  When two
	// sided lighting is desired. typically for alpha tested surfaces, this is
	// addressed by having CleanupModelSurfaces() create duplicates of all the triangles
	// with apropriate order reversal.
	bool				ShouldCreateBackSides() const
	{
		return shouldCreateBackSides;
	}

	// characters and models that are created by a complete renderbump can use a faster
	// method of tangent and normal vector generation than surfaces which have a flat
	// renderbump wrapped over them.
	bool				UseUnsmoothedTangents() const
	{
		return unsmoothedTangents;
	}

	// RB: characters and models that baked in Blender or Substance designer use the newer
	// Mikkelsen tangent space standard.
	// see: https://bgolus.medium.com/generating-perfect-normal-maps-for-unity-f929e673fc57
	bool				UseMikkTSpace() const
	{
		return mikktspace;
	}

	// by default, monsters can have blood overlays placed on them, but this can
	// be overrided on a per-material basis with the "noOverlays" material command.
	// This will always return false for translucent surfaces
	bool				AllowOverlays() const
	{
		return allowOverlays;
	}

	// MC_OPAQUE, MC_PERFORATED, or MC_TRANSLUCENT, for interaction list linking and
	// dmap flood filling
	// The depth buffer will not be filled for MC_TRANSLUCENT surfaces
	// FIXME: what do nodraw surfaces return?
	materialCoverage_t	Coverage() const
	{
		return coverage;
	}

	// returns true if this material takes precedence over other in coplanar cases
	bool				HasHigherDmapPriority( const idMaterial& other ) const
	{
		return ( IsDrawn() && !other.IsDrawn() ) ||
			   ( Coverage() < other.Coverage() );
	}

	// returns a idUserInterface if it has a global gui, or NULL if no gui
	idUserInterface*		GlobalGui() const
	{
		return gui;
	}

	// a discrete surface will never be merged with other surfaces by dmap, which is
	// necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
	// special effects from being combined into a single surface
	// guis, merging sprites or other effects, mirrors and remote views are always discrete
	bool				IsDiscrete() const
	{
		return ( entityGui || gui || deform != DFRM_NONE || sort == SS_SUBVIEW ||
				 ( surfaceFlags & SURF_DISCRETE ) != 0 );
	}

	// Normally, dmap chops each surface by every BSP boundary, then reoptimizes.
	// For gigantic polygons like sky boxes, this can cause a huge number of planar
	// triangles that make the optimizer take forever to turn back into a single
	// triangle.  The "noFragment" option causes dmap to only break the polygons at
	// area boundaries, instead of every BSP boundary.  This has the negative effect
	// of not automatically fixing up interpenetrations, so when this is used, you
	// should manually make the edges of your sky box exactly meet, instead of poking
	// into each other.
	bool				NoFragment() const
	{
		return ( surfaceFlags & SURF_NOFRAGMENT ) != 0;
	}

	//------------------------------------------------------------------
	// light shader specific functions, only called for light entities

	// lightshader option to fill with fog from viewer instead of light from center
	bool				IsFogLight() const
	{
		return fogLight;
	}

	// perform simple blending of the projection, instead of interacting with bumps and textures
	bool				IsBlendLight() const
	{
		return blendLight;
	}

	// an ambient light has non-directional bump mapping and no specular
	bool				IsAmbientLight() const
	{
		return ambientLight;
	}

	// implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
	// but individual light entities can also override this value
	bool				LightCastsShadows() const
	{
		return TestMaterialFlag( MF_FORCESHADOWS ) ||
			   ( !fogLight && !ambientLight && !blendLight && !TestMaterialFlag( MF_NOSHADOWS ) );
	}

	// fog lights, blend lights, ambient lights, etc will all have to have interaction
	// triangles generated for sides facing away from the light as well as those
	// facing towards the light.  It is debatable if noshadow lights should effect back
	// sides, making everything "noSelfShadow", but that would make noshadow lights
	// potentially slower than normal lights, which detracts from their optimization
	// ability, so they currently do not.
	bool				LightEffectsBackSides() const
	{
		return fogLight || ambientLight || blendLight;
	}

	// NULL unless an image is explicitly specified in the shader with "lightFalloffShader <image>"
	idImage*				LightFalloffImage() const
	{
		return lightFalloffImage;
	}

	//------------------------------------------------------------------

	// returns the renderbump command line for this shader, or an empty string if not present
	const char* 		GetRenderBump() const
	{
		return renderBump;
	};

	// set specific material flag(s)
	void				SetMaterialFlag( const int flag ) const
	{
		materialFlags |= flag;
	}

	// clear specific material flag(s)
	void				ClearMaterialFlag( const int flag ) const
	{
		materialFlags &= ~flag;
	}

	// test for existance of specific material flag(s)
	bool				TestMaterialFlag( const int flag ) const
	{
		return ( materialFlags & flag ) != 0;
	}

	// get content flags
	const int			GetContentFlags() const
	{
		return contentFlags;
	}

	// get surface flags
	const int			GetSurfaceFlags() const
	{
		return surfaceFlags;
	}

	// gets name for surface type (stone, metal, flesh, etc.)
	const surfTypes_t	GetSurfaceType() const
	{
		return static_cast<surfTypes_t>( surfaceFlags & SURF_TYPE_MASK );
	}

	// get material description
	const char* 		GetDescription() const
	{
		return desc;
	}

	// get sort order
	const float			GetSort() const
	{
		return sort;
	}

	const int			GetStereoEye() const
	{
		return stereoEye;
	}

	// this is only used by the gui system to force sorting order
	// on images referenced from tga's instead of materials.
	// this is done this way as there are 2000 tgas the guis use
	void				SetSort( float s ) const
	{
		sort = s;
	};

	// DFRM_NONE, DFRM_SPRITE, etc
	deform_t			Deform() const
	{
		return deform;
	}

	// flare size, expansion size, etc
	const int			GetDeformRegister( int index ) const
	{
		return deformRegisters[index];
	}

	// particle system to emit from surface and table for turbulent
	const idDecl*		GetDeformDecl() const
	{
		return deformDecl;
	}

	// currently a surface can only have one unique texgen for all the stages
	texgen_t			Texgen() const;

	// wobble sky parms
	const int* 			GetTexGenRegisters() const
	{
		return texGenRegisters;
	}

	// get cull type
	const cullType_t	GetCullType() const
	{
		return cullType;
	}

	float				GetEditorAlpha() const
	{
		return editorAlpha;
	}

	int					GetEntityGui() const
	{
		return entityGui;
	}

	decalInfo_t			GetDecalInfo() const
	{
		return decalInfo;
	}

	// spectrums are used for "invisible writing" that can only be
	// illuminated by a light of matching spectrum
	int					Spectrum() const
	{
		return spectrum;
	}

	float				GetPolygonOffset() const
	{
		return polygonOffset;
	}

	float				GetSurfaceArea() const
	{
		return surfaceArea;
	}
	void				AddToSurfaceArea( float area )
	{
		surfaceArea += area;
	}

	//------------------------------------------------------------------

	// returns the length, in milliseconds, of the videoMap on this material,
	// or zero if it doesn't have one
	int					CinematicLength() const;

	void				CloseCinematic() const;

	void				ResetCinematicTime( int time ) const;

	int					GetCinematicStartTime() const;

	void				UpdateCinematic( int time ) const;

	// RB: added because we can't rely on the FFmpeg feedback how long a video really is
	bool                CinematicIsPlaying() const;
	// RB end

	//------------------------------------------------------------------

	// gets an image for the editor to use
	idImage* 			GetEditorImage() const;
	idImage* 			GetLightEditorImage() const; // RB
	int					GetImageWidth() const;
	int					GetImageHeight() const;

	void				SetGui( const char* _gui ) const;

	//------------------------------------------------------------------

	// returns number of registers this material contains
	const int			GetNumRegisters() const
	{
		return numRegisters;
	}

	// Regs should point to a float array large enough to hold GetNumRegisters() floats.
	// FloatTime is passed in because different entities, which may be running in parallel,
	// can be in different time groups.
	void				EvaluateRegisters(
		float* 			registers,
		const float		localShaderParms[MAX_ENTITY_SHADER_PARMS],
		const float		globalShaderParms[MAX_GLOBAL_SHADER_PARMS],
		const float		floatTime,
		idSoundEmitter* soundEmitter ) const;

	// if a material only uses constants (no entityParm or globalparm references), this
	// will return a pointer to an internal table, and EvaluateRegisters will not need
	// to be called.  If NULL is returned, EvaluateRegisters must be used.
	const float* 		ConstantRegisters() const
	{
		return constantRegisters;
	};

	bool				SuppressInSubview() const
	{
		return suppressInSubview;
	};
	bool				IsPortalSky() const
	{
		return portalSky;
	};
	void				AddReference();

	// motorsep 11-23-2014; material LOD keys that define what LOD iteration the surface falls into
	// lod1 - lod4 defines several levels of LOD
	// persistentLOD specifies the LOD iteration that still being rendered, even after the camera is beyond the distance at which LOD iteration should not be rendered

	bool				IsLOD() const
	{
		return ( materialFlags & ( MF_LOD1 | MF_LOD2 | MF_LOD3 | MF_LOD4 ) ) != 0;
	}
	// foresthale 2014-11-24: added IsLODVisibleForDistance method
	bool				IsLODVisibleForDistance( float distance, float lodBase ) const
	{
		int bit = ( materialFlags & ( MF_LOD1 | MF_LOD2 | MF_LOD3 | MF_LOD4 ) ) >> MF_LOD1_SHIFT;
		float m1 = lodBase * ( bit >> 1 );
		float m2 = lodBase * bit;
		return distance >= m1 && ( distance < m2 || ( materialFlags & ( MF_LOD_PERSISTENT ) ) );
	}


private:
	// parse the entire material
	void				CommonInit();
	void				ParseMaterial( idLexer& src );
	bool				MatchToken( idLexer& src, const char* match );
	void				ParseSort( idLexer& src );
	void				ParseStereoEye( idLexer& src );
	void				ParseBlend( idLexer& src, shaderStage_t* stage );
	void				ParseVertexParm( idLexer& src, newShaderStage_t* newStage );
	void				ParseVertexParm2( idLexer& src, newShaderStage_t* newStage );
	void				ParseFragmentMap( idLexer& src, newShaderStage_t* newStage );
	void                ParseStencilCompare( const idToken& token, stencilComp_t* stencilComp );
	void                ParseStencilOperation( const idToken& token, stencilOperation_t* stencilOp );
	void                ParseStencil( idLexer& src, stencilStage_t* stencilStage );
	void				ParseStage( idLexer& src, const textureRepeat_t trpDefault = TR_REPEAT );
	void				ParseDeform( idLexer& src );
	void				ParseDecalInfo( idLexer& src );
	bool				CheckSurfaceParm( idToken* token );
	int					GetExpressionConstant( float f );
	int					GetExpressionTemporary();
	expOp_t*			GetExpressionOp();
	int					EmitOp( int a, int b, expOpType_t opType );
	int					ParseEmitOp( idLexer& src, int a, expOpType_t opType, int priority );
	int					ParseTerm( idLexer& src );
	int					ParseExpressionPriority( idLexer& src, int priority );
	int					ParseExpression( idLexer& src );
	void				ClearStage( shaderStage_t* ss );
	int					NameToSrcBlendMode( const idStr& name );
	int					NameToDstBlendMode( const idStr& name );
	void				MultiplyTextureMatrix( textureStage_t* ts, int registers[2][3] );	// FIXME: for some reason the const is bad for gcc and Mac
	void				SortInteractionStages();
	void				AddImplicitStages( const textureRepeat_t trpDefault = TR_REPEAT );
	void				CheckForConstantRegisters();
	void				SetFastPathImages();

private:
	idStr				desc;				// description
	idStr				renderBump;			// renderbump command options, without the "renderbump" at the start

	idImage*			lightFalloffImage;	// only for light shaders

	idImage* 			fastPathBumpImage;	// if any of these are set, they all will be
	idImage* 			fastPathDiffuseImage;
	idImage* 			fastPathSpecularImage;

	int					entityGui;			// draw a gui with the idUserInterface from the renderEntity_t
	// non zero will draw gui, gui2, or gui3 from renderEnitty_t
	mutable idUserInterface*	gui;			// non-custom guis are shared by all users of a material

	bool				noFog;				// surface does not create fog interactions

	int					spectrum;			// for invisible writing, used for both lights and surfaces

	float				polygonOffset;

	int					contentFlags;		// content flags
	int					surfaceFlags;		// surface flags
	mutable int			materialFlags;		// material flags

	decalInfo_t			decalInfo;

	mutable	float		sort;				// lower numbered shaders draw before higher numbered
	int					stereoEye;
	deform_t			deform;
	int					deformRegisters[4];		// numeric parameter for deforms
	const idDecl*		deformDecl;			// for surface emitted particle deforms and tables

	int					texGenRegisters[MAX_TEXGEN_REGISTERS];	// for wobbleSky

	materialCoverage_t	coverage;
	cullType_t			cullType;			// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	SubViewType         subViewType;        // SP added
	bool				shouldCreateBackSides;

	bool				fogLight;
	bool				blendLight;
	bool				ambientLight;
	bool				unsmoothedTangents;
	bool				mikktspace;			// RB: use Mikkelsen tangent space standard for normal mapping
	bool				hasSubview;			// mirror, remote render, etc
	bool				allowOverlays;

	int					numOps;
	expOp_t* 			ops;				// evaluate to make expressionRegisters

	int					numRegisters;																			//
	float* 				expressionRegisters;

	float* 				constantRegisters;	// NULL if ops ever reference globalParms or entityParms

	int					numStages;
	int					numAmbientStages;

	shaderStage_t* 		stages;

	struct mtrParsingData_s*	pd;			// only used during parsing

	float				surfaceArea;		// only for listSurfaceAreas

	// we defer loading of the editor image until it is asked for, so the game doesn't load up
	// all the invisible and uncompressed images.
	// If editorImage is NULL, it will atempt to load editorImageName, and set editorImage to that or defaultImage
	idStr				editorImageName;
	mutable idImage* 	editorImage;		// image used for non-shaded preview
	float				editorAlpha;

	bool				suppressInSubview;
	bool				portalSky;
	int					refCount;
};

typedef idList<const idMaterial*, TAG_MATERIAL> idMatList;

#endif /* !__MATERIAL_H__ */
