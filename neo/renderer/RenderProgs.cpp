/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans

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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"


#if defined(USE_VULKAN)

	extern idUniformBuffer emptyUBO;

	void CreateVertexDescriptions();

	void CreateDescriptorPools( VkDescriptorPool( &pools )[ NUM_FRAME_DATA ] );

#endif


idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager()
{
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager()
{
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs& args )
{
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "----- Initializing Render Shaders -----\n" );


	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}

	// RB: added checks for GPU skinning
	struct builtinShaders_t
	{
		int					index;
		const char*			name;
		const char*			nameOutSuffix;
		uint32				shaderFeatures;
		bool				requireGPUSkinningSupport;
		rpStage_t			stages;
		vertexLayoutType_t	layout;
	} builtins[] =
	{
		{ BUILTIN_GUI, "builtin/gui", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_COLOR, "builtin/color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "builtin/color", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_VERTEX_COLOR, "builtin/vertex_color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING, "builtin/lighting/ambient_lighting", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING_SKINNED, "builtin/lighting/ambient_lighting", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_AMBIENT_LIGHTING_IBL, "builtin/lighting/ambient_lighting_IBL", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_SKINNED, "builtin/lighting/ambient_lighting_IBL", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_PBR, "builtin/lighting/ambient_lighting_IBL", "_PBR", BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_PBR_SKINNED, "builtin/lighting/ambient_lighting_IBL", "_PBR_skinned", BIT( USE_GPU_SKINNING | USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_AMBIENT_LIGHTGRID_IBL, "builtin/lighting/ambient_lightgrid_IBL", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_SKINNED, "builtin/lighting/ambient_lightgrid_IBL", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR, "builtin/lighting/ambient_lightgrid_IBL", "_PBR", BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR_SKINNED, "builtin/lighting/ambient_lightgrid_IBL", "_PBR_skinned", BIT( USE_GPU_SKINNING | USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_SMALL_GEOMETRY_BUFFER, "builtin/gbuffer", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED, "builtin/gbuffer", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_TEXTURED, "builtin/texture", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "builtin/texture_color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SRGB, "builtin/texture_color", "_sRGB", BIT( USE_SRGB ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "builtin/texture_color_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "builtin/texture_color_texgen", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// RB begin
		{ BUILTIN_INTERACTION, "builtin/lighting/interaction", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SKINNED, "builtin/lighting/interaction", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_AMBIENT, "builtin/lighting/interactionAmbient", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "builtin/lighting/interactionAmbient_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "builtin/lighting/interactionSM", "_spot", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "builtin/lighting/interactionSM", "_spot_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "builtin/lighting/interactionSM", "_point", BIT( LIGHT_POINT ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "builtin/lighting/interactionSM", "_point_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_POINT ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "builtin/lighting/interactionSM", "_parallel", BIT( LIGHT_PARALLEL ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_parallel_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_PARALLEL ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// PBR variants
		{ BUILTIN_PBR_INTERACTION, "builtin/lighting/interaction", "_PBR", BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_PBR_INTERACTION_SKINNED, "builtin/lighting/interaction", "_skinned_PBR", BIT( USE_GPU_SKINNING ) | BIT( USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_PBR_INTERACTION_AMBIENT, "builtin/lighting/interactionAmbient", "_PBR", BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_PBR_INTERACTION_AMBIENT_SKINNED, "builtin/lighting/interactionAmbient_skinned", "_PBR", BIT( USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT, "builtin/lighting/interactionSM", "_spot_PBR", BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "builtin/lighting/interactionSM", "_spot_skinned_PBR", BIT( USE_GPU_SKINNING ) | BIT( USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT, "builtin/lighting/interactionSM", "_point_PBR", BIT( LIGHT_POINT ) | BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "builtin/lighting/interactionSM", "_point_skinned_PBR", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_POINT ) | BIT( USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL, "builtin/lighting/interactionSM", "_parallel_PBR", BIT( LIGHT_PARALLEL ) | BIT( USE_PBR ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_parallel_skinned_PBR", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_PARALLEL ) | BIT( USE_PBR ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// debug stuff
		{ BUILTIN_DEBUG_LIGHTGRID, "builtin/debug/lightgrid", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEBUG_LIGHTGRID_SKINNED, "builtin/debug/lightgrid", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_DEBUG_OCTAHEDRON, "builtin/debug/octahedron", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEBUG_OCTAHEDRON_SKINNED, "builtin/debug/octahedron", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end

		{ BUILTIN_ENVIRONMENT, "builtin/legacy/environment", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_ENVIRONMENT_SKINNED, "builtin/legacy/environment_skinned", "",  0, true , SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT},
		{ BUILTIN_BUMPY_ENVIRONMENT, "builtin/legacy/bumpyenvironment", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "builtin/legacy/bumpyenvironment_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_DEPTH, "builtin/depth", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEPTH_SKINNED, "builtin/depth_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_SHADOW, "builtin/lighting/shadow", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT },
		{ BUILTIN_SHADOW_SKINNED, "builtin/lighting/shadow_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT_SKINNED },

		{ BUILTIN_SHADOW_DEBUG, "builtin/debug/shadowDebug", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "builtin/debug/shadowDebug_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_BLENDLIGHT, "builtin/fog/blendlight", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG, "builtin/fog/fog", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG_SKINNED, "builtin/fog/fog_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SKYBOX, "builtin/legacy/skybox", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_WOBBLESKY, "builtin/legacy/wobblesky", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_POSTPROCESS, "builtin/post/postprocess", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_SCREEN, "builtin/post/screen", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TONEMAP, "builtin/post/tonemap", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BRIGHTPASS, "builtin/post/tonemap", "_brightpass", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_HDR_GLARE_CHROMATIC, "builtin/post/hdr_glare_chromatic", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_HDR_DEBUG, "builtin/post/tonemap", "_debug", BIT( HDR_DEBUG ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_SMAA_EDGE_DETECTION, "builtin/post/SMAA_edge_detection", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION, "builtin/post/SMAA_blending_weight_calc", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMAA_NEIGHBORHOOD_BLENDING, "builtin/post/SMAA_final", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_AMBIENT_OCCLUSION, "builtin/SSAO/AmbientOcclusion_AO", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "builtin/SSAO/AmbientOcclusion_AO", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "builtin/SSAO/AmbientOcclusion_blur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "builtin/SSAO/AmbientOcclusion_blur", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "builtin/SSAO/AmbientOcclusion_minify", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "builtin/SSAO/AmbientOcclusion_minify", "_mip0", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI, "builtin/SSGI/DeepGBufferRadiosity_radiosity", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR, "builtin/SSGI/DeepGBufferRadiosity_blur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT, "builtin/SSGI/DeepGBufferRadiosity_blur", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_STEREO_DEGHOST, "builtin/VR/stereoDeGhost", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_WARP, "builtin/VR/stereoWarp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK, "builtin/video/bink", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// SRS - Added Bink shader without sRGB to linear conversion for testVideo cmd
		{ BUILTIN_BINK_SRGB, "builtin/video/bink", "_sRGB", BIT( USE_SRGB ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// SRS end
		{ BUILTIN_BINK_GUI, "builtin/video/bink_gui", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_INTERLACE, "builtin/VR/stereoInterlace", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_MOTION_BLUR, "builtin/post/motionBlur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "builtin/debug/debug_shadowmap", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );

	renderProgs.SetNum( numBuiltins );

	for( int i = 0; i < numBuiltins; i++ )
	{
		renderProg_t& prog = renderProgs[ i ];

		prog.name = builtins[i].name;
		prog.builtin = true;
		prog.vertexLayout = builtins[i].layout;

		builtinShaders[builtins[i].index] = i;

		if( builtins[i].requireGPUSkinningSupport && !glConfig.gpuSkinningAvailable )
		{
			// RB: don't try to load shaders that would break the GLSL compiler in the OpenGL driver
			continue;
		}

		uint32 shaderFeatures = builtins[i].shaderFeatures;
		if( builtins[i].requireGPUSkinningSupport )
		{
			shaderFeatures |= BIT( USE_GPU_SKINNING );
		}

		int vIndex = -1;
		if( builtins[ i ].stages & SHADER_STAGE_VERTEX )
		{
			vIndex = FindShader( builtins[ i ].name, SHADER_STAGE_VERTEX, builtins[i].nameOutSuffix, shaderFeatures, true, builtins[i].layout );
		}

		int fIndex = -1;
		if( builtins[ i ].stages & SHADER_STAGE_FRAGMENT )
		{
			fIndex = FindShader( builtins[ i ].name, SHADER_STAGE_FRAGMENT, builtins[i].nameOutSuffix, shaderFeatures, true, builtins[i].layout );
		}

		//idLib::Printf( "Loading GLSL program %i %i %i\n", i, vIndex, fIndex );

		LoadGLSLProgram( i, vIndex, fIndex );
	}

	r_useHalfLambertLighting.ClearModified();
	r_useHDR.ClearModified();
	r_usePBR.ClearModified();
	r_pbrDebug.ClearModified();

	uniforms.SetNum( RENDERPARM_TOTAL, vec4_zero );

	if( glConfig.gpuSkinningAvailable )
	{
		renderProgs[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;
		// RB begin
		renderProgs[builtinShaders[BUILTIN_DEBUG_LIGHTGRID_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_DEBUG_OCTAHEDRON_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_IBL_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_IBL_PBR_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTGRID_IBL_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;

		// PBR variants
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;
		// RB end
	}

	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );

#if defined(USE_VULKAN)
	counter = 0;

	// Create Vertex Descriptions
	CreateVertexDescriptions();

	// Create Descriptor Pools
	CreateDescriptorPools( descriptorPools );

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		parmBuffers[ i ] = new idUniformBuffer();
		parmBuffers[ i ]->AllocBufferObject( NULL, MAX_DESC_SETS * MAX_DESC_SET_UNIFORMS * sizeof( idVec4 ), BU_DYNAMIC );
	}

	// Placeholder: mainly for optionalSkinning
	emptyUBO.AllocBufferObject( NULL, sizeof( idVec4 ), BU_DYNAMIC );
#endif
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders()
{
	for( int i = 0; i < shaders.Num(); i++ )
	{
		LoadShader( i, shaders[i].stage );
	}

	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		if( renderProgs[i].vertexShaderIndex == -1 || renderProgs[i].fragmentShaderIndex == -1 )
		{
			// RB: skip reloading because we didn't load it initially
			continue;
		}

		LoadGLSLProgram( i, renderProgs[i].vertexShaderIndex, renderProgs[i].fragmentShaderIndex );
	}
}



/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	KillAllShaders();
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/
int idRenderProgManager::FindShader( const char* name, rpStage_t stage, const char* nameOutSuffix, uint32 features, bool builtin, vertexLayoutType_t vertexLayout )
{
	idStr shaderName( name );
	shaderName.StripFileExtension();
	//shaderName += nameOutSuffix;

	for( int i = 0; i < shaders.Num(); i++ )
	{
		shader_t& shader = shaders[ i ];
		if( shader.name.Icmp( shaderName.c_str() ) == 0 && shader.stage == stage && shader.nameOutSuffix.Icmp( nameOutSuffix ) == 0 )
		{
			LoadShader( i, stage );
			return i;
		}
	}

	shader_t shader;
	shader.name = shaderName;
	shader.nameOutSuffix = nameOutSuffix;
	shader.shaderFeatures = features;
	shader.builtin = builtin;
	shader.stage = stage;
	shader.vertexLayout = vertexLayout;

	int index = shaders.Append( shader );
	LoadShader( index, stage );

	return index;
}


// RB begin
bool idRenderProgManager::IsShaderBound() const
{
	return ( current != -1 );
}
// RB end

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		SetRenderParm( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* value )
{
	SetUniformValue( rp, value );
}


/*
========================
RpPrintState
========================
*/
void RpPrintState( uint64 stateBits )
{

	// culling
	idLib::Printf( "Culling: " );
	switch( stateBits & GLS_CULL_BITS )
	{
		case GLS_CULL_FRONTSIDED:
			idLib::Printf( "FRONTSIDED -> BACK" );
			break;
		case GLS_CULL_BACKSIDED:
			idLib::Printf( "BACKSIDED -> FRONT" );
			break;
		case GLS_CULL_TWOSIDED:
			idLib::Printf( "TWOSIDED" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( "\n" );

	// polygon mode
	idLib::Printf( "PolygonMode: %s\n", ( stateBits & GLS_POLYMODE_LINE ) ? "LINE" : "FILL" );

	// color mask
	idLib::Printf( "ColorMask: " );
	idLib::Printf( ( stateBits & GLS_REDMASK ) ? "_" : "R" );
	idLib::Printf( ( stateBits & GLS_GREENMASK ) ? "_" : "G" );
	idLib::Printf( ( stateBits & GLS_BLUEMASK ) ? "_" : "B" );
	idLib::Printf( ( stateBits & GLS_ALPHAMASK ) ? "_" : "A" );
	idLib::Printf( "\n" );

	// blend
	idLib::Printf( "Blend: src=" );
	switch( stateBits & GLS_SRCBLEND_BITS )
	{
		case GLS_SRCBLEND_ZERO:
			idLib::Printf( "ZERO" );
			break;
		case GLS_SRCBLEND_ONE:
			idLib::Printf( "ONE" );
			break;
		case GLS_SRCBLEND_DST_COLOR:
			idLib::Printf( "DST_COLOR" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			idLib::Printf( "ONE_MINUS_DST_COLOR" );
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			idLib::Printf( "SRC_ALPHA" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			idLib::Printf( "ONE_MINUS_SRC_ALPHA" );
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			idLib::Printf( "DST_ALPHA" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			idLib::Printf( "ONE_MINUS_DST_ALPHA" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( ", dst=" );
	switch( stateBits & GLS_DSTBLEND_BITS )
	{
		case GLS_DSTBLEND_ZERO:
			idLib::Printf( "ZERO" );
			break;
		case GLS_DSTBLEND_ONE:
			idLib::Printf( "ONE" );
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			idLib::Printf( "SRC_COLOR" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			idLib::Printf( "ONE_MINUS_SRC_COLOR" );
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			idLib::Printf( "SRC_ALPHA" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			idLib::Printf( "ONE_MINUS_SRC_ALPHA" );
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			idLib::Printf( "DST_ALPHA" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			idLib::Printf( "ONE_MINUS_DST_ALPHA" );
			break;
		default:
			idLib::Printf( "NA" );
	}
	idLib::Printf( "\n" );

	// depth func
	idLib::Printf( "DepthFunc: " );
	switch( stateBits & GLS_DEPTHFUNC_BITS )
	{
		case GLS_DEPTHFUNC_EQUAL:
			idLib::Printf( "EQUAL" );
			break;
		case GLS_DEPTHFUNC_ALWAYS:
			idLib::Printf( "ALWAYS" );
			break;
		case GLS_DEPTHFUNC_LESS:
			idLib::Printf( "LEQUAL" );
			break;
		case GLS_DEPTHFUNC_GREATER:
			idLib::Printf( "GEQUAL" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( "\n" );

	// depth mask
	idLib::Printf( "DepthWrite: %s\n", ( stateBits & GLS_DEPTHMASK ) ? "FALSE" : "TRUE" );

	// depth bounds
	idLib::Printf( "DepthBounds: %s\n", ( stateBits & GLS_DEPTH_TEST_MASK ) ? "TRUE" : "FALSE" );

	// depth bias
	idLib::Printf( "DepthBias: %s\n", ( stateBits & GLS_POLYGON_OFFSET ) ? "TRUE" : "FALSE" );

	// stencil
	auto printStencil = [&]( stencilFace_t face, uint64 bits, uint64 mask, uint64 ref )
	{
		idLib::Printf( "Stencil: %s, ", ( bits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) ? "ON" : "OFF" );
		idLib::Printf( "Face=" );
		switch( face )
		{
			case STENCIL_FACE_FRONT:
				idLib::Printf( "FRONT" );
				break;
			case STENCIL_FACE_BACK:
				idLib::Printf( "BACK" );
				break;
			default:
				idLib::Printf( "BOTH" );
				break;
		}
		idLib::Printf( ", Func=" );
		switch( bits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				idLib::Printf( "NEVER" );
				break;
			case GLS_STENCIL_FUNC_LESS:
				idLib::Printf( "LESS" );
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				idLib::Printf( "EQUAL" );
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				idLib::Printf( "LEQUAL" );
				break;
			case GLS_STENCIL_FUNC_GREATER:
				idLib::Printf( "GREATER" );
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				idLib::Printf( "NOTEQUAL" );
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				idLib::Printf( "GEQUAL" );
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				idLib::Printf( "ALWAYS" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", OpFail=" );
		switch( bits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_FAIL_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_FAIL_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_FAIL_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_FAIL_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_FAIL_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", ZFail=" );
		switch( bits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", OpPass=" );
		switch( bits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_PASS_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_PASS_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_PASS_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_PASS_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_PASS_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", mask=%llu, ref=%llu\n", mask, ref );
	};

	uint32 mask = uint32( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
	uint32 ref = uint32( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
	if( stateBits & GLS_SEPARATE_STENCIL )
	{
		printStencil( STENCIL_FACE_FRONT, ( stateBits & GLS_STENCIL_FRONT_OPS ), mask, ref );
		printStencil( STENCIL_FACE_BACK, ( ( stateBits & GLS_STENCIL_BACK_OPS ) >> 12 ), mask, ref );
	}
	else
	{
		printStencil( STENCIL_FACE_NUM, stateBits, mask, ref );
	}
}
