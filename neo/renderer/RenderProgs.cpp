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

#pragma hdrstop
#include "precompiled.h"

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
		{ BUILTIN_GUI, "gui.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_COLOR, "color.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "color", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_VERTEX_COLOR, "vertex_color.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING, "ambient_lighting", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_LIGHTING_SKINNED, "ambient_lighting", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER, "gbuffer", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED, "gbuffer", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_TEXTURED, "texture.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SRGB, "texture_color.vfp", "", BIT( USE_SRGB ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_INTERACTION, "interaction.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SKINNED, "interaction", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "interactionSM", "_spot", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "interactionSM", "_spot_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "interactionSM", "_point", BIT( LIGHT_POINT ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "interactionSM", "_point_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_POINT ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "interactionSM", "_parallel", BIT( LIGHT_PARALLEL ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "interactionSM", "_parallel_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_PARALLEL ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_ENVIRONMENT, "environment.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned.vfp", "",  0, true , SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT},
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyenvironment.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyenvironment_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		{ BUILTIN_DEPTH, "depth.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		{ BUILTIN_SHADOW, "shadow.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT },
		{ BUILTIN_SHADOW_SKINNED, "shadow_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT_SKINNED },
		
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		{ BUILTIN_BLENDLIGHT, "blendlight.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG, "fog.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SKYBOX, "skybox.vfp", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_SCREEN, "screen", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TONEMAP, "tonemap", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BRIGHTPASS, "tonemap", "_brightpass", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_HDR_GLARE_CHROMATIC, "hdr_glare_chromatic", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_HDR_DEBUG, "tonemap", "_debug", BIT( HDR_DEBUG ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		{ BUILTIN_SMAA_EDGE_DETECTION, "SMAA_edge_detection", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION, "SMAA_blending_weight_calc", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SMAA_NEIGHBORHOOD_BLENDING, "SMAA_final", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		{ BUILTIN_AMBIENT_OCCLUSION, "AmbientOcclusion_AO", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "AmbientOcclusion_AO", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "AmbientOcclusion_blur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "AmbientOcclusion_blur", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "AmbientOcclusion_minify", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "AmbientOcclusion_minify", "_mip0", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI, "DeepGBufferRadiosity_radiosity", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR, "DeepGBufferRadiosity_blur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT, "DeepGBufferRadiosity_blur", "_write", BIT( BRIGHTPASS ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK, "bink.vfp", "",  0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		
		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "debug_shadowmap.vfp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
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
		
		int vIndex = -1;
		if( builtins[ i ].stages & SHADER_STAGE_VERTEX )
		{
			vIndex = FindShader( builtins[ i ].name, SHADER_STAGE_VERTEX, builtins[i].nameOutSuffix, builtins[i].shaderFeatures, true );
		}
		
		int fIndex = -1;
		if( builtins[ i ].stages & SHADER_STAGE_FRAGMENT )
		{
			fIndex = FindShader( builtins[ i ].name, SHADER_STAGE_FRAGMENT, builtins[i].nameOutSuffix, builtins[i].shaderFeatures, true );
		}
		
		//idLib::Printf( "Loading GLSL program %i %i %i\n", i, vIndex, fIndex );
		
		LoadGLSLProgram( i, vIndex, fIndex );
	}
	
	r_useHalfLambertLighting.ClearModified();
	r_useHDR.ClearModified();
	
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
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;
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
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();
	
	// destroy shaders
	for( int i = 0; i < shaders.Num(); ++i )
	{
		shader_t& shader = shaders[ i ];
		vkDestroyShaderModule( vkcontext.device, shader.module, NULL );
		shader.module = VK_NULL_HANDLE;
	}
	
	// destroy pipelines
	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		renderProg_t& prog = renderProgs[ i ];
		
		for( int j = 0; j < prog.pipelines.Num(); ++j )
		{
			vkDestroyPipeline( vkcontext.device, prog.pipelines[ j ].pipeline, NULL );
		}
		prog.pipelines.Clear();
		
		vkDestroyDescriptorSetLayout( vkcontext.device, prog.descriptorSetLayout, NULL );
		vkDestroyPipelineLayout( vkcontext.device, prog.pipelineLayout, NULL );
	}
	renderProgs.Clear();
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		parmBuffers[ i ]->FreeBufferObject();
		delete parmBuffers[ i ];
		parmBuffers[ i ] = NULL;
	}
	
	emptyUBO.FreeBufferObject();
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		//vkFreeDescriptorSets( vkcontext.device, descriptorPools[ i ], MAX_DESC_SETS, descriptorSets[ i ] );
		vkResetDescriptorPool( vkcontext.device, descriptorPools[ i ], 0 );
		vkDestroyDescriptorPool( vkcontext.device, descriptorPools[ i ], NULL );
	}
	
	memset( descriptorSets, 0, sizeof( descriptorSets ) );
	memset( descriptorPools, 0, sizeof( descriptorPools ) );
	
	counter = 0;
	currentData = 0;
	currentDescSet = 0;
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
int idRenderProgManager::FindShader( const char* name, rpStage_t stage, const char* nameOutSuffix, uint32 features, bool builtin )
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

