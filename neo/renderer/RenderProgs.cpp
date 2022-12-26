/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"


#if defined( USE_NVRHI )

	#include <sys/DeviceManager.h>
	#include <nvrhi/utils.h>

	extern DeviceManager* deviceManager;

#elif defined(USE_VULKAN)

	extern idUniformBuffer emptyUBO;

	void CreateVertexDescriptions();

	void CreateDescriptorPools( VkDescriptorPool( &pools )[NUM_FRAME_DATA] );

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
void idRenderProgManager::Init( nvrhi::IDevice* device )
{
	common->Printf( "----- Initializing Render Shaders -----\n" );

	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}

#if defined( USE_NVRHI )
	this->device = device;

	uniforms.SetNum( RENDERPARM_TOTAL, vec4_zero );
	uniformsChanged = false;

	for( int i = 0; i < NUM_BINDING_LAYOUTS; i++ )
	{
		auto constantBufferDesc = nvrhi::utils::CreateVolatileConstantBufferDesc( uniforms.Allocated(), va( "RenderParams_%d", i ), 16384 );
		constantBuffer[i] = device->createBuffer( constantBufferDesc );
	}

	// === Main draw vertex layout ===
	vertexLayoutDescs.SetNum( NUM_VERTEX_LAYOUTS, {} );

	vertexLayoutDescs[LAYOUT_DRAW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "POSITION" )
		.setFormat( nvrhi::Format::RGB32_FLOAT )
		.setOffset( offsetof( idDrawVert, xyz ) )
		.setElementStride( sizeof( idDrawVert ) ) );

	vertexLayoutDescs[LAYOUT_DRAW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "TEXCOORD" )
		.setFormat( nvrhi::Format::RG16_FLOAT )
		.setOffset( offsetof( idDrawVert, st ) )
		.setElementStride( sizeof( idDrawVert ) ) );

	vertexLayoutDescs[LAYOUT_DRAW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "NORMAL" )
		.setFormat( nvrhi::Format::RGBA8_UNORM )
		.setOffset( offsetof( idDrawVert, normal ) )
		.setElementStride( sizeof( idDrawVert ) ) );

	vertexLayoutDescs[LAYOUT_DRAW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "TANGENT" )
		.setFormat( nvrhi::Format::RGBA8_UNORM )
		.setOffset( offsetof( idDrawVert, tangent ) )
		.setElementStride( sizeof( idDrawVert ) ) );

	vertexLayoutDescs[LAYOUT_DRAW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "COLOR" )
		.setArraySize( 2 )
		.setFormat( nvrhi::Format::RGBA8_UNORM )
		.setOffset( offsetof( idDrawVert, color ) )
		.setElementStride( sizeof( idDrawVert ) ) );

	// === Shadow vertex ===

	vertexLayoutDescs[LAYOUT_DRAW_SHADOW_VERT].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "POSITION" )
		.setFormat( nvrhi::Format::RGBA32_FLOAT )
		.setOffset( offsetof( idShadowVert, xyzw ) )
		.setElementStride( sizeof( idShadowVert ) ) );

	// === Shadow vertex skinned ===

	vertexLayoutDescs[LAYOUT_DRAW_SHADOW_VERT_SKINNED].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "POSITION" )
		.setFormat( nvrhi::Format::RGBA32_FLOAT )
		.setOffset( offsetof( idShadowVertSkinned, xyzw ) )
		.setElementStride( sizeof( idShadowVertSkinned ) ) );

	vertexLayoutDescs[LAYOUT_DRAW_SHADOW_VERT_SKINNED].Append(
		nvrhi::VertexAttributeDesc()
		.setName( "COLOR" )
		.setArraySize( 2 )
		.setFormat( nvrhi::Format::RGBA8_UNORM )
		.setOffset( offsetof( idShadowVertSkinned, color ) )
		.setElementStride( sizeof( idShadowVertSkinned ) ) );

	bindingLayouts.SetNum( NUM_BINDING_LAYOUTS );

	auto renderParmLayoutItem = nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 );

	auto uniformsLayoutDesc = nvrhi::BindingLayoutDesc()
							  .setVisibility( nvrhi::ShaderType::All )
							  .addItem( renderParmLayoutItem );

	auto uniformsLayout = device->createBindingLayout( uniformsLayoutDesc );

	auto skinningLayoutDesc = nvrhi::BindingLayoutDesc()
							  .setVisibility( nvrhi::ShaderType::All )
							  .addItem( renderParmLayoutItem )
							  .addItem( nvrhi::BindingLayoutItem::StructuredBuffer_SRV( 11 ) ); // joint buffer;

	auto skinningLayout = device->createBindingLayout( skinningLayoutDesc );

	auto defaultLayoutDesc = nvrhi::BindingLayoutDesc()
							 .setVisibility( nvrhi::ShaderType::Pixel )
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) );

	auto samplerOneLayoutDesc = nvrhi::BindingLayoutDesc()
								.setVisibility( nvrhi::ShaderType::Pixel )
								.addItem( nvrhi::BindingLayoutItem::Sampler( 0 ) );
	auto samplerOneBindingLayout = device->createBindingLayout( samplerOneLayoutDesc );

	auto defaultLayout = device->createBindingLayout( defaultLayoutDesc );

	bindingLayouts[BINDING_LAYOUT_DEFAULT] = { uniformsLayout, defaultLayout, samplerOneBindingLayout };
	bindingLayouts[BINDING_LAYOUT_DEFAULT_SKINNED] = { skinningLayout, defaultLayout, samplerOneBindingLayout };

	bindingLayouts[BINDING_LAYOUT_CONSTANT_BUFFER_ONLY] = { uniformsLayout };
	bindingLayouts[BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED] = { skinningLayout };

	auto defaultMaterialLayoutDesc = nvrhi::BindingLayoutDesc()
									 .setVisibility( nvrhi::ShaderType::Pixel )
									 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )		// normal
									 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) )		// specular
									 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 2 ) );	// base color

	auto defaultMaterialLayout = device->createBindingLayout( defaultMaterialLayoutDesc );

	auto ambientIblLayoutDesc = nvrhi::BindingLayoutDesc()
								.setVisibility( nvrhi::ShaderType::Pixel )
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 3 ) ) // brdf lut
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 4 ) ) // ssao
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 7 ) ) // irradiance cube map
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 8 ) ) // radiance cube map 1
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 9 ) ) // radiance cube map 2
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 10 ) ); // radiance cube map 3

	auto ambientIblLayout = device->createBindingLayout( ambientIblLayoutDesc );

	auto samplerTwoBindingLayoutDesc = nvrhi::BindingLayoutDesc()
									   .setVisibility( nvrhi::ShaderType::Pixel )
									   .addItem( nvrhi::BindingLayoutItem::Sampler( 0 ) )	// (Wrap) Anisotropic sampler: normal sampler & specular sampler
									   .addItem( nvrhi::BindingLayoutItem::Sampler( 1 ) );	// (Clamp) Linear sampler: brdf lut sampler & ssao sampler
	auto samplerTwoBindingLayout = device->createBindingLayout( samplerTwoBindingLayoutDesc );

	bindingLayouts[ BINDING_LAYOUT_AMBIENT_LIGHTING_IBL ] =
	{
		uniformsLayout, defaultMaterialLayout, ambientIblLayout, samplerTwoBindingLayout
	};
	bindingLayouts[ BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED ] =
	{
		skinningLayout, defaultMaterialLayout, ambientIblLayout, samplerTwoBindingLayout
	};

	auto blitLayoutDesc = nvrhi::BindingLayoutDesc()
						  .setVisibility( nvrhi::ShaderType::All )
						  .addItem( nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ) ); // blit constants

	bindingLayouts[BINDING_LAYOUT_BLIT] = { device->createBindingLayout( blitLayoutDesc ) };

	auto aoLayoutDesc = nvrhi::BindingLayoutDesc()
						.setVisibility( nvrhi::ShaderType::All )
						.addItem( renderParmLayoutItem )
						.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )
						.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) )
						.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 2 ) );

	bindingLayouts[BINDING_LAYOUT_DRAW_AO] = { device->createBindingLayout( aoLayoutDesc ), samplerOneBindingLayout };

	auto aoLayoutDesc2 = nvrhi::BindingLayoutDesc()
						 .setVisibility( nvrhi::ShaderType::All )
						 .addItem( renderParmLayoutItem )
						 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) );

	bindingLayouts[BINDING_LAYOUT_DRAW_AO1] = { device->createBindingLayout( aoLayoutDesc2 ), samplerOneBindingLayout };

	auto interactionBindingLayoutDesc = nvrhi::BindingLayoutDesc()
										.setVisibility( nvrhi::ShaderType::Pixel )
										.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 3 ) )	// light falloff
										.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 4 ) );	// light projection

	auto interactionBindingLayout = device->createBindingLayout( interactionBindingLayoutDesc );
	bindingLayouts[BINDING_LAYOUT_DRAW_INTERACTION] =
	{
		uniformsLayout, defaultMaterialLayout, interactionBindingLayout, samplerTwoBindingLayout
	};
	bindingLayouts[BINDING_LAYOUT_DRAW_INTERACTION_SKINNED] =
	{
		skinningLayout, defaultMaterialLayout, interactionBindingLayout, samplerTwoBindingLayout
	};

	auto interactionSmBindingLayoutDesc = nvrhi::BindingLayoutDesc()
										  .setVisibility( nvrhi::ShaderType::Pixel )
										  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 3 ) ) // light falloff
										  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 4 ) ) // light projection
										  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 5 ) ) // shadow map array
										  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 6 ) ); // jitter

	auto interactionSmBindingLayout = device->createBindingLayout( interactionSmBindingLayoutDesc );

	auto samplerFourBindingLayoutDesc = nvrhi::BindingLayoutDesc()
										.setVisibility( nvrhi::ShaderType::Pixel )
										.addItem( nvrhi::BindingLayoutItem::Sampler( 0 ) )	 // material
										.addItem( nvrhi::BindingLayoutItem::Sampler( 1 ) )	 // lighting
										.addItem( nvrhi::BindingLayoutItem::Sampler( 2 ) )	 // shadow compare
										.addItem( nvrhi::BindingLayoutItem::Sampler( 3 ) );	 // blue noise for shadow jitter
	auto samplerFourBindingLayout = device->createBindingLayout( samplerFourBindingLayoutDesc );

	bindingLayouts[BINDING_LAYOUT_DRAW_INTERACTION_SM] =
	{
		uniformsLayout, defaultMaterialLayout, interactionSmBindingLayout, samplerFourBindingLayout
	};
	bindingLayouts[BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED] =
	{
		skinningLayout, defaultMaterialLayout, interactionSmBindingLayout, samplerFourBindingLayout
	};

	auto fogBindingLayoutDesc = nvrhi::BindingLayoutDesc()
								.setVisibility( nvrhi::ShaderType::Pixel )
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )
								.addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) );

	auto fogBindingLayout = device->createBindingLayout( fogBindingLayoutDesc );

	bindingLayouts[BINDING_LAYOUT_FOG] =
	{
		uniformsLayout, fogBindingLayout, samplerTwoBindingLayout
	};
	bindingLayouts[BINDING_LAYOUT_FOG_SKINNED] =
	{
		skinningLayout, fogBindingLayout, samplerTwoBindingLayout
	};

	auto blendLightBindingLayoutDesc = nvrhi::BindingLayoutDesc()
									   .setVisibility( nvrhi::ShaderType::Pixel )
									   .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) ) // light 1
									   .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) ); // light 2

	auto blendLightBindingLayout = device->createBindingLayout( blendLightBindingLayoutDesc );

	bindingLayouts[BINDING_LAYOUT_BLENDLIGHT] =
	{
		uniformsLayout, blendLightBindingLayout, samplerOneBindingLayout
	};
	bindingLayouts[BINDING_LAYOUT_BLENDLIGHT_SKINNED] =
	{
		uniformsLayout, blendLightBindingLayout, samplerOneBindingLayout
	};

	auto pp3DBindingLayout = nvrhi::BindingLayoutDesc()
							 .setVisibility( nvrhi::ShaderType::All )
							 .addItem( renderParmLayoutItem )
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )	// current render
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) )	// normal map
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 2 ) );	// mask

	bindingLayouts[BINDING_LAYOUT_POST_PROCESS_INGAME] = { device->createBindingLayout( pp3DBindingLayout ), samplerOneBindingLayout };

	auto ppFxBindingLayout = nvrhi::BindingLayoutDesc()
							 .setVisibility( nvrhi::ShaderType::All )
							 .addItem( renderParmLayoutItem )
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )
							 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) );

	bindingLayouts[BINDING_LAYOUT_POST_PROCESS_FINAL] = { device->createBindingLayout( ppFxBindingLayout ), samplerTwoBindingLayout };

	auto normalCubeBindingLayoutDesc = nvrhi::BindingLayoutDesc()
									   .setVisibility( nvrhi::ShaderType::Pixel )
									   .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )	// cube map
									   .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) );	// normal map

	auto normalCubeBindingLayout = device->createBindingLayout( normalCubeBindingLayoutDesc );

	bindingLayouts[BINDING_LAYOUT_NORMAL_CUBE] =
	{
		uniformsLayout, normalCubeBindingLayout, samplerOneBindingLayout
	};
	bindingLayouts[BINDING_LAYOUT_NORMAL_CUBE_SKINNED] =
	{
		skinningLayout, normalCubeBindingLayout, samplerOneBindingLayout
	};

	auto binkVideoBindingLayout = nvrhi::BindingLayoutDesc()
								  .setVisibility( nvrhi::ShaderType::All )
								  .addItem( renderParmLayoutItem )
								  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )	// cube map
								  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) )	// cube map
								  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 2 ) );	// normal map

	bindingLayouts[BINDING_LAYOUT_BINK_VIDEO] = { device->createBindingLayout( binkVideoBindingLayout ), samplerOneBindingLayout };

	auto motionVectorsBindingLayout = nvrhi::BindingLayoutDesc()
									  .setVisibility( nvrhi::ShaderType::All )
									  .addItem( renderParmLayoutItem )
									  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )	// cube map
									  .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) );	// normal map

	bindingLayouts[BINDING_LAYOUT_TAA_MOTION_VECTORS] = { device->createBindingLayout( motionVectorsBindingLayout ), samplerOneBindingLayout };

	nvrhi::BindingLayoutDesc tonemapLayout;
	tonemapLayout.visibility = nvrhi::ShaderType::Pixel;
	tonemapLayout.bindings =
	{
		nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
		nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
		nvrhi::BindingLayoutItem::TypedBuffer_SRV( 1 ),
		nvrhi::BindingLayoutItem::Texture_SRV( 2 ),
		nvrhi::BindingLayoutItem::Sampler( 0 )
	};
	bindingLayouts[BINDING_LAYOUT_TONEMAP] = { device->createBindingLayout( tonemapLayout ) };

	nvrhi::BindingLayoutDesc histogramLayout;
	histogramLayout.visibility = nvrhi::ShaderType::Compute;
	histogramLayout.bindings =
	{
		nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
		nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
		nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
	};
	bindingLayouts[BINDING_LAYOUT_HISTOGRAM] = { device->createBindingLayout( histogramLayout ) };

	nvrhi::BindingLayoutDesc exposureLayout;
	exposureLayout.visibility = nvrhi::ShaderType::Compute;
	exposureLayout.bindings =
	{
		nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
		nvrhi::BindingLayoutItem::TypedBuffer_SRV( 0 ),
		nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
	};
	bindingLayouts[BINDING_LAYOUT_EXPOSURE] = { device->createBindingLayout( exposureLayout ) };

	// RB: added checks for GPU skinning
	struct builtinShaders_t
	{
		int						index;
		const char*				name;
		const char*				nameOutSuffix;
		idList<shaderMacro_t>	macros;
		bool					requireGPUSkinningSupport;
		rpStage_t				stages;
		vertexLayoutType_t		layout;
		bindingLayoutType_t		bindingLayout;
		//bindingLayoutType_t		bindingLayout2;
	} builtins[] =
	{
		{ BUILTIN_GUI, "builtin/gui", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_COLOR, "builtin/color", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "builtin/color", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED },
		{ BUILTIN_VERTEX_COLOR, "builtin/vertex_color", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY },

		{ BUILTIN_AMBIENT_LIGHTING_IBL, "builtin/lighting/ambient_lighting_IBL", "", { { "USE_GPU_SKINNING", "0" }, { "USE_PBR", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_SKINNED, "builtin/lighting/ambient_lighting_IBL", "_skinned", { { "USE_GPU_SKINNING", "1" }, { "USE_PBR", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_PBR, "builtin/lighting/ambient_lighting_IBL", "_PBR", { { "USE_GPU_SKINNING", "0" }, { "USE_PBR", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL },
		{ BUILTIN_AMBIENT_LIGHTING_IBL_PBR_SKINNED, "builtin/lighting/ambient_lighting_IBL", "_PBR_skinned", { { "USE_GPU_SKINNING", "1" }, { "USE_PBR", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED },

		{ BUILTIN_AMBIENT_LIGHTGRID_IBL, "builtin/lighting/ambient_lightgrid_IBL", "", { { "USE_GPU_SKINNING", "0" }, { "USE_PBR", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_SKINNED, "builtin/lighting/ambient_lightgrid_IBL", "_skinned", { { "USE_GPU_SKINNING", "1" }, { "USE_PBR", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR, "builtin/lighting/ambient_lightgrid_IBL", "_PBR", { { "USE_GPU_SKINNING", "0" }, { "USE_PBR", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL },
		{ BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR_SKINNED, "builtin/lighting/ambient_lightgrid_IBL", "_PBR_skinned", { { "USE_GPU_SKINNING", "1" }, { "USE_PBR", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED },

		{ BUILTIN_SMALL_GEOMETRY_BUFFER, "builtin/gbuffer", "", { {"USE_GPU_SKINNING", "0" }, { "USE_NORMAL_FMT_RGB8", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED, "builtin/gbuffer", "_skinned", { {"USE_GPU_SKINNING", "1" }, { "USE_NORMAL_FMT_RGB8", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT_SKINNED },
		// RB end
		{ BUILTIN_TEXTURED, "builtin/texture", "", { }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "builtin/texture_color", "", { {"USE_GPU_SKINNING", "0" }, {"USE_SRGB", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SRGB, "builtin/texture_color", "_sRGB", { {"USE_GPU_SKINNING", "0" }, {"USE_SRGB", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "builtin/texture_color", "_skinned", { {"USE_GPU_SKINNING", "1" }, {"USE_SRGB", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT_SKINNED },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "builtin/texture_color_texgen", "",  {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },

		// RB begin
		{ BUILTIN_INTERACTION, "builtin/lighting/interaction", "", { {"USE_GPU_SKINNING", "0" }, { "USE_PBR", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION },
		{ BUILTIN_INTERACTION_SKINNED, "builtin/lighting/interaction", "_skinned", { {"USE_GPU_SKINNING", "1" }, { "USE_PBR", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SKINNED },

		{ BUILTIN_INTERACTION_AMBIENT, "builtin/lighting/interactionAmbient", "", { {"USE_GPU_SKINNING", "0" }, { "USE_PBR", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "builtin/lighting/interactionAmbient", "_skinned", { {"USE_GPU_SKINNING", "1" }, { "USE_PBR", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SKINNED },

		// PBR variants
		{ BUILTIN_PBR_INTERACTION, "builtin/lighting/interaction", "_PBR", { {"USE_GPU_SKINNING", "0" }, { "USE_PBR", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION },
		{ BUILTIN_PBR_INTERACTION_SKINNED, "builtin/lighting/interaction", "_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "USE_PBR", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SKINNED },

		{ BUILTIN_PBR_INTERACTION_AMBIENT, "builtin/lighting/interactionAmbient", "_PBR", { {"USE_GPU_SKINNING", "0" }, { "USE_PBR", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION },
		{ BUILTIN_PBR_INTERACTION_AMBIENT_SKINNED, "builtin/lighting/interactionAmbient", "_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "USE_PBR", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SKINNED },

		// regular shadow mapping
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "builtin/lighting/interactionSM", "_spot", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "builtin/lighting/interactionSM", "_spot_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "builtin/lighting/interactionSM", "_point", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "builtin/lighting/interactionSM", "_point_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "builtin/lighting/interactionSM", "_parallel", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_parallel_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT, "builtin/lighting/interactionSM", "_spot_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "builtin/lighting/interactionSM", "_spot_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT, "builtin/lighting/interactionSM", "_point_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "builtin/lighting/interactionSM", "_point_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL, "builtin/lighting/interactionSM", "_parallel_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_parallel_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "0" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		// shadow mapping using a big atlas
		{ BUILTIN_INTERACTION_SHADOW_ATLAS_SPOT, "builtin/lighting/interactionSM", "_atlas_spot", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_ATLAS_SPOT_SKINNED, "builtin/lighting/interactionSM", "_atlas_spot_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_INTERACTION_SHADOW_ATLAS_POINT, "builtin/lighting/interactionSM", "_atlas_point", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_ATLAS_POINT_SKINNED, "builtin/lighting/interactionSM", "_atlas_point_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_INTERACTION_SHADOW_ATLAS_PARALLEL, "builtin/lighting/interactionSM", "_atlas_parallel", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_INTERACTION_SHADOW_ATLAS_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_atlas_parallel_skinned", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "0" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_SPOT, "builtin/lighting/interactionSM", "_atlas_spot_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_SPOT_SKINNED, "builtin/lighting/interactionSM", "_atlas_spot_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_POINT, "builtin/lighting/interactionSM", "_atlas_point_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_POINT_SKINNED, "builtin/lighting/interactionSM", "_atlas_point_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "1" }, { "LIGHT_PARALLEL", "0" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },

		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_PARALLEL, "builtin/lighting/interactionSM", "_atlas_parallel_PBR", { {"USE_GPU_SKINNING", "0" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM },
		{ BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_atlas_parallel_skinned_PBR", { {"USE_GPU_SKINNING", "1" }, { "LIGHT_POINT", "0" }, { "LIGHT_PARALLEL", "1" }, { "USE_PBR", "1" }, { "USE_NORMAL_FMT_RGB8", "0" }, { "USE_SHADOW_ATLAS", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED },


		// debug stuff
		{ BUILTIN_DEBUG_LIGHTGRID, "builtin/debug/lightgrid", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_DEBUG_LIGHTGRID_SKINNED, "builtin/debug/lightgrid", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT_SKINNED },

		{ BUILTIN_DEBUG_OCTAHEDRON, "builtin/debug/octahedron", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_DEBUG_OCTAHEDRON_SKINNED, "builtin/debug/octahedron", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT_SKINNED },
		// RB end

		{ BUILTIN_ENVIRONMENT, "builtin/legacy/environment", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_ENVIRONMENT_SKINNED, "builtin/legacy/environment", "_skinned",  { {"USE_GPU_SKINNING", "1" } }, true , SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT_SKINNED },
		{ BUILTIN_BUMPY_ENVIRONMENT, "builtin/legacy/bumpyenvironment", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_NORMAL_CUBE },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "builtin/legacy/bumpyenvironment", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_NORMAL_CUBE_SKINNED },

		{ BUILTIN_DEPTH, "builtin/depth", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY },
		{ BUILTIN_DEPTH_SKINNED, "builtin/depth", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED },

		{ BUILTIN_SHADOW, "builtin/lighting/shadow", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY },
		{ BUILTIN_SHADOW_SKINNED, "builtin/lighting/shadow", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT_SKINNED, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED },

		{ BUILTIN_SHADOW_DEBUG, "builtin/debug/shadowDebug", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "builtin/debug/shadowDebug", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED },

		{ BUILTIN_BLENDLIGHT, "builtin/fog/blendlight", "",  { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BLENDLIGHT },
		{ BUILTIN_BLENDLIGHT_SKINNED, "builtin/fog/blendlight", "_skinned",  { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BLENDLIGHT_SKINNED },
		{ BUILTIN_FOG, "builtin/fog/fog", "", { {"USE_GPU_SKINNING", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_FOG },
		{ BUILTIN_FOG_SKINNED, "builtin/fog/fog", "_skinned", { {"USE_GPU_SKINNING", "1" } }, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_FOG_SKINNED },
		{ BUILTIN_SKYBOX, "builtin/legacy/skybox", "",  {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_WOBBLESKY, "builtin/legacy/wobblesky", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_POSTPROCESS, "builtin/post/postprocess", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_POST_PROCESS_FINAL },
		// RB begin
		{ BUILTIN_SCREEN, "builtin/post/screen", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_TONEMAP, "builtin/post/tonemap", "", { { "BRIGHTPASS", "0" }, { "HDR_DEBUG", "0"} }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_BRIGHTPASS, "builtin/post/tonemap", "_brightpass", { { "BRIGHTPASS", "1" }, { "HDR_DEBUG", "0"} }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_HDR_GLARE_CHROMATIC, "builtin/post/hdr_glare_chromatic", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_HDR_DEBUG, "builtin/post/tonemap", "_debug", { { "BRIGHTPASS", "0" }, { "HDR_DEBUG", "1"} }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },

		{ BUILTIN_SMAA_EDGE_DETECTION, "builtin/post/SMAA_edge_detection", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION, "builtin/post/SMAA_blending_weight_calc", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_SMAA_NEIGHBORHOOD_BLENDING, "builtin/post/SMAA_final", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },

		{ BUILTIN_MOTION_BLUR, "builtin/post/motionBlur", "_vectors", { { "VECTORS_ONLY", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_TAA_MOTION_VECTORS },
		{ BUILTIN_TAA_RESOLVE, "builtin/post/taa", "", { { "SAMPLE_COUNT", "1" }, { "USE_CATMULL_ROM_FILTER", "1" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_TAA_RESOLVE },
		{ BUILTIN_TAA_RESOLVE_MSAA_2X, "builtin/post/taa", "_msaa2x", { { "SAMPLE_COUNT", "2" }, { "USE_CATMULL_ROM_FILTER", "1" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_TAA_RESOLVE },
		{ BUILTIN_TAA_RESOLVE_MSAA_4X, "builtin/post/taa", "_msaa4x", { { "SAMPLE_COUNT", "4" }, { "USE_CATMULL_ROM_FILTER", "1" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_TAA_RESOLVE },
		{ BUILTIN_TAA_RESOLVE_MSAA_8X, "builtin/post/taa", "_msaa8x", { { "SAMPLE_COUNT", "8" }, { "USE_CATMULL_ROM_FILTER", "1" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_TAA_RESOLVE },

		{ BUILTIN_AMBIENT_OCCLUSION, "builtin/SSAO/AmbientOcclusion_AO", "", { { "BRIGHTPASS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "builtin/SSAO/AmbientOcclusion_AO", "_write", { { "BRIGHTPASS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "builtin/SSAO/AmbientOcclusion_blur", "", { { "BRIGHTPASS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "builtin/SSAO/AmbientOcclusion_blur", "_write", { { "BRIGHTPASS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "builtin/SSAO/AmbientOcclusion_minify", "", { { "BRIGHTPASS", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO1 },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "builtin/SSAO/AmbientOcclusion_minify", "_mip0", { { "BRIGHTPASS", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DRAW_AO1 },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI, "builtin/SSGI/DeepGBufferRadiosity_radiosity", "", { }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR, "builtin/SSGI/DeepGBufferRadiosity_blur", "", { }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT, "builtin/SSGI/DeepGBufferRadiosity_blur", "_write", { }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		// RB end
		{ BUILTIN_STEREO_DEGHOST, "builtin/VR/stereoDeGhost", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_STEREO_WARP, "builtin/VR/stereoWarp", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_BINK, "builtin/video/bink", "",  { {"USE_SRGB", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BINK_VIDEO },
		{ BUILTIN_BINK_SRGB, "builtin/video/bink", "_srgb", { {"USE_SRGB", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BINK_VIDEO },
		{ BUILTIN_BINK_GUI, "builtin/video/bink_gui", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BINK_VIDEO },
		{ BUILTIN_STEREO_INTERLACE, "builtin/VR/stereoInterlace", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		{ BUILTIN_MOTION_BLUR, "builtin/post/motionBlur", "", { { "VECTORS_ONLY", "1" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },

		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "builtin/debug/debug_shadowmap", "", {}, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT, BINDING_LAYOUT_DEFAULT },
		// RB end

		// SP begin
		{ BUILTIN_BLIT, "builtin/blit", "", { { "TEXTURE_ARRAY", "0" } }, false, SHADER_STAGE_FRAGMENT, LAYOUT_UNKNOWN, BINDING_LAYOUT_BLIT },
		{ BUILTIN_RECT, "builtin/rect", "", { }, false, SHADER_STAGE_VERTEX, LAYOUT_DRAW_VERT, BINDING_LAYOUT_BLIT },
		{ BUILTIN_TONEMAPPING, "builtin/post/tonemapping", "", { { "HISTOGRAM_BINS", "256" }, { "SOURCE_ARRAY", "0" }, { "QUAD_Z", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_UNKNOWN, BINDING_LAYOUT_TONEMAP },
		{ BUILTIN_TONEMAPPING_TEX_ARRAY, "builtin/post/tonemapping", "", { { "HISTOGRAM_BINS", "256" }, { "SOURCE_ARRAY", "1" }, { "QUAD_Z", "0" } }, false, SHADER_STAGE_DEFAULT, LAYOUT_UNKNOWN, BINDING_LAYOUT_TONEMAP },
		{ BUILTIN_HISTOGRAM_CS, "builtin/post/histogram", "", { { "HISTOGRAM_BINS", "256" }, { "SOURCE_ARRAY", "0" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_HISTOGRAM },
		{ BUILTIN_HISTOGRAM_TEX_ARRAY_CS, "builtin/post/histogram", "", { { "HISTOGRAM_BINS", "256" }, { "SOURCE_ARRAY", "1" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_HISTOGRAM },
		{ BUILTIN_EXPOSURE_CS, "builtin/post/exposure", "", { { "HISTOGRAM_BINS", "256" } }, false, SHADER_STAGE_COMPUTE, LAYOUT_UNKNOWN, BINDING_LAYOUT_EXPOSURE },
		// SP end
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );

	renderProgs.SetNum( numBuiltins );

	for( int i = 0; i < numBuiltins; i++ )
	{
		renderProg_t& prog = renderProgs[i];

		prog.name = builtins[i].name;
		prog.builtin = true;
		prog.vertexLayout = builtins[i].layout;
		prog.bindingLayoutType = builtins[i].bindingLayout;

		builtinShaders[builtins[i].index] = i;

		if( builtins[i].requireGPUSkinningSupport && !glConfig.gpuSkinningAvailable )
		{
			// RB: don't try to load shaders that would break the GLSL compiler in the OpenGL driver
			continue;
		}

		int vIndex = -1;
		if( builtins[i].stages & SHADER_STAGE_VERTEX )
		{
			vIndex = FindShader( builtins[i].name, SHADER_STAGE_VERTEX, builtins[i].nameOutSuffix, builtins[i].macros, true, builtins[i].layout );
		}

		int fIndex = -1;
		if( builtins[i].stages & SHADER_STAGE_FRAGMENT )
		{
			fIndex = FindShader( builtins[i].name, SHADER_STAGE_FRAGMENT, builtins[i].nameOutSuffix, builtins[i].macros, true, builtins[i].layout );
		}

		int cIndex = -1;
		if( builtins[i].stages & SHADER_STAGE_COMPUTE )
		{
			cIndex = FindShader( builtins[i].name, SHADER_STAGE_COMPUTE, builtins[i].nameOutSuffix, builtins[i].macros, true, builtins[i].layout );
		}

		idLib::Printf( "Loading shader program %s\n", prog.name.c_str() );

		if( vIndex > -1 && fIndex > -1 )
		{
			LoadProgram( i, vIndex, fIndex );
		}

		if( cIndex > -1 )
		{
			LoadComputeProgram( i, cIndex );
		}
	}
#endif

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

		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_IBL_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTING_IBL_PBR_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTGRID_IBL_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_AMBIENT_LIGHTGRID_IBL_PBR_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_ATLAS_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_ATLAS_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_INTERACTION_SHADOW_ATLAS_PARALLEL_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;

		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_SPOT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_POINT_SKINNED]].usesJoints = true;
		renderProgs[builtinShaders[BUILTIN_PBR_INTERACTION_SHADOW_ATLAS_PARALLEL_SKINNED]].usesJoints = true;
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
		parmBuffers[i] = new idUniformBuffer();
		parmBuffers[i]->AllocBufferObject( NULL, MAX_DESC_SETS * MAX_DESC_SET_UNIFORMS * sizeof( idVec4 ), BU_DYNAMIC );
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

		LoadProgram( i, renderProgs[i].vertexShaderIndex, renderProgs[i].fragmentShaderIndex );
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

#if defined( USE_NVRHI )
	// SRS - Delete renderprogs builtin binding layouts
	for( int i = 0; i < renderProgs.Num(); i++ )
	{
		for( int j = 0; j < renderProgs[i].bindingLayouts.Num(); j++ )
		{
			renderProgs[i].bindingLayouts[j].Reset();
		}
	}

	// SRS - Delete binding layouts
	for( int i = 0; i < bindingLayouts.Num(); i++ )
	{
		for( int j = 0; j < bindingLayouts[i].Num(); j++ )
		{
			bindingLayouts[i][j].Reset();
		}
	}

	// SRS - Unmap buffer memory using overloaded = operator
	for( int i = 0; i < constantBuffer.Num(); i++ )
	{
		constantBuffer[i] = nullptr;
	}
#endif
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/

// TODO REMOVE
int idRenderProgManager::FindShader( const char* name, rpStage_t stage )
{
	idStr shaderName( name );
	shaderName.StripFileExtension();

	for( int i = 0; i < shaders.Num(); i++ )
	{
		shader_t& shader = shaders[i];
		if( shader.name.Icmp( shaderName.c_str() ) == 0 && shader.stage == stage )
		{
			LoadShader( i, stage );
			return i;
		}
	}

	// Load it.
	shader_t shader;
	shader.name = shaderName;
	shader.stage = stage;

#if 0
	for( int i = 0; i < MAX_SHADER_MACRO_NAMES; i++ )
	{
		int feature = ( bool )( 0 & BIT( i ) );
		idStr macroName( GetGLSLMacroName( ( shaderFeature_t )i ) );
		idStr value( feature );
		shader.macros.Append( shaderMacro_t( macroName, value ) );
	}
#endif

	int index = shaders.Append( shader );
	LoadShader( index, stage );

	return index;
}


/*
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
*/


int idRenderProgManager::FindShader( const char* name, rpStage_t stage, const char* nameOutSuffix, const idList<shaderMacro_t>& macros, bool builtin, vertexLayoutType_t vertexLayout )
{
	idStr shaderName( name );
	shaderName.StripFileExtension();

	for( int i = 0; i < shaders.Num(); i++ )
	{
		shader_t& shader = shaders[i];
		if( shader.name.Icmp( shaderName ) == 0 && shader.stage == stage && shader.nameOutSuffix.Icmp( nameOutSuffix ) == 0 )
		{
			LoadShader( i, stage );
			return i;
		}
	}

	// Load it.
	shader_t shader;
	shader.name = shaderName;
	shader.nameOutSuffix = nameOutSuffix;
	shader.shaderFeatures = 0;
	shader.builtin = builtin;
	shader.stage = stage;
	shader.macros = macros;

	int index = shaders.Append( shader );
	LoadShader( index, stage );

	return index;
}

#if defined( USE_NVRHI )
nvrhi::ShaderHandle idRenderProgManager::GetShader( int index )
{
	return shaders[index].handle;
}

programInfo_t idRenderProgManager::GetProgramInfo( int index )
{
	programInfo_t info;

	renderProg_t& prog = renderProgs[index];

	info.bindingLayoutType = prog.bindingLayoutType;

	if( prog.vertexShaderIndex > -1 && prog.vertexShaderIndex < shaders.Num() )
	{
		info.vs = GetShader( prog.vertexShaderIndex );
	}
	if( prog.fragmentShaderIndex > -1 && prog.fragmentShaderIndex < shaders.Num() )
	{
		info.ps = GetShader( prog.fragmentShaderIndex );
	}
	if( prog.computeShaderIndex > -1 && prog.computeShaderIndex < shaders.Num() )
	{
		info.cs = GetShader( prog.computeShaderIndex );
	}
	info.inputLayout = prog.inputLayout;
	info.bindingLayouts = &prog.bindingLayouts;

	return info;
}
#endif

bool idRenderProgManager::IsShaderBound() const
{
	return ( currentIndex != -1 );
}

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
