/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2019 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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

// SRS - Include SDL headers to enable vsync changes without restart for UNIX-like OSs
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	// SRS - Don't seem to need these #undefs (at least on macOS), are they needed for Linux, etc?
	// DG: SDL.h somehow needs the following functions, so #undef those silly
	//     "don't use" #defines from Str.h
	//#undef strncmp
	//#undef strcasecmp
	//#undef vsnprintf
	// DG end
	#include <SDL.h>
#endif
// SRS end

#include "../RenderCommon.h"
#include "../RenderBackend.h"
#include "../../framework/Common_local.h"
#include "renderer/RenderPass.h"

#include "../../imgui/imgui.h"

#include <sys/DeviceManager.h>

#include "nvrhi/utils.h"

idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
idCVar stereoRender_warp( "stereoRender_warp", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use the optical warping renderprog instead of stereoDeGhost" );
idCVar stereoRender_warpStrength( "stereoRender_warpStrength", "1.45", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "amount of pre-distortion" );

idCVar stereoRender_warpCenterX( "stereoRender_warpCenterX", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for left eye, right eye will be 1.0 - this" );
idCVar stereoRender_warpCenterY( "stereoRender_warpCenterY", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for both eyes" );
idCVar stereoRender_warpParmZ( "stereoRender_warpParmZ", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpParmW( "stereoRender_warpParmW", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpTargetFraction( "stereoRender_warpTargetFraction", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "fraction of half-width the through-lens view covers" );

idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

void GLimp_SwapBuffers();
void RB_SetMVP( const idRenderMatrix& mvp );

glContext_t glcontext;

class NvrhiContext
{
public:

	int										currentImageParm = 0;
	idArray< idImage*, MAX_IMAGE_PARMS >	imageParms;
	nvrhi::GraphicsPipelineHandle			pipeline;
	bool									fullscreen = false;
};

static NvrhiContext context;

/*
==================
GL_CheckErrors
==================
*/
// RB: added filename, line parms
bool GL_CheckErrors_( const char* filename, int line )
{
	return false;
}


/*
========================
DebugCallback

For ARB_debug_output
========================
*/
// RB: added const to userParam
static void CALLBACK DebugCallback( unsigned int source, unsigned int type,
									unsigned int id, unsigned int severity, int length, const char* msg, const void* userParam )
{
	char	s[1024];

	// it probably isn't safe to do an idLib::Printf at this point

	const char* severityStr = "Severity: Unkown";
	switch( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH:
			severityStr = "Severity: High";
			break;

		case GL_DEBUG_SEVERITY_MEDIUM:
			severityStr = "Severity: Medium";
			break;

		case GL_DEBUG_SEVERITY_LOW:
			severityStr = "Severity: High";
			break;
	}

	idStr::snPrintf( s, sizeof( s ), "[OpenGL] Debug: [ %s ] Code %d, %d : '%s'\n", severityStr, source, type, msg );

	// RB: printf should be thread safe on Linux
#if defined(_WIN32)
	OutputDebugString( s );
	OutputDebugString( "\n" );
#else
	printf( "%s\n", s );
#endif
	// RB end
}


/*
==================
R_CheckPortableExtensions
==================
*/
// RB: replaced QGL with GLEW
static void R_CheckPortableExtensions()
{
	glConfig.glVersion = atof( glConfig.version_string );
	const char* badVideoCard = idLocalization::GetString( "#str_06780" );
	if( glConfig.glVersion < 2.0f )
	{
		idLib::FatalError( "%s", badVideoCard );
	}

	if( idStr::Icmpn( glConfig.renderer_string, "ATI ", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "AMD ", 4 ) == 0 )
	{
		glConfig.vendor = VENDOR_AMD;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "NVIDIA", 6 ) == 0 )
	{
		glConfig.vendor = VENDOR_NVIDIA;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "Intel", 5 ) == 0 )
	{
		glConfig.vendor = VENDOR_INTEL;
	}

	// RB: Mesa support
	if( idStr::Icmpn( glConfig.renderer_string, "Mesa", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "X.org", 5 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "Gallium", 7 ) == 0 ||
			strcmp( glConfig.vendor_string, "X.Org" ) == 0 ||
			idStr::Icmpn( glConfig.renderer_string, "llvmpipe", 8 ) == 0 )
	{
		if( glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE )
		{
			glConfig.driverType = GLDRV_OPENGL_MESA_CORE_PROFILE;
		}
		else
		{
			glConfig.driverType = GLDRV_OPENGL_MESA;
		}
	}
	// RB end

	// GL_ARB_multitexture
	if( glConfig.driverType != GLDRV_OPENGL3X )
	{
		glConfig.multitextureAvailable = true;
	}
	else
	{
		glConfig.multitextureAvailable = GLEW_ARB_multitexture != 0;
	}

	// GL_EXT_direct_state_access
	glConfig.directStateAccess = GLEW_EXT_direct_state_access != 0;


	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.textureCompressionAvailable = true;
	}
	else
	{
		glConfig.textureCompressionAvailable = GLEW_ARB_texture_compression != 0 && GLEW_EXT_texture_compression_s3tc != 0;
	}
	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicFilterAvailable = GLEW_EXT_texture_filter_anisotropic != 0;
	if( glConfig.anisotropicFilterAvailable )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "   maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	}
	else
	{
		glConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	// The actual extension is broken as specificed, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	glConfig.textureLODBiasAvailable = ( glConfig.glVersion >= 1.4 || GLEW_EXT_texture_lod_bias != 0 );
	if( glConfig.textureLODBiasAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_texture_lod_bias" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_texture_lod_bias" );
	}

	// GL_ARB_seamless_cube_map
	glConfig.seamlessCubeMapAvailable = GLEW_ARB_seamless_cube_map != 0;
	r_useSeamlessCubeMap.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_vertex_buffer_object
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.vertexBufferObjectAvailable = true;
	}
	else
	{
		glConfig.vertexBufferObjectAvailable = GLEW_ARB_vertex_buffer_object != 0;
	}

	// GL_ARB_map_buffer_range, map a section of a buffer object's data store
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.mapBufferRangeAvailable = true;
	//}
	//else
	{
		glConfig.mapBufferRangeAvailable = GLEW_ARB_map_buffer_range != 0;
	}

	// GL_ARB_vertex_array_object
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.vertexArrayObjectAvailable = true;
	//}
	//else
	{
		glConfig.vertexArrayObjectAvailable = GLEW_ARB_vertex_array_object != 0;
	}

	// GL_ARB_draw_elements_base_vertex
	glConfig.drawElementsBaseVertexAvailable = GLEW_ARB_draw_elements_base_vertex != 0;

	// GL_ARB_vertex_program / GL_ARB_fragment_program
	glConfig.fragmentProgramAvailable = GLEW_ARB_fragment_program != 0;
	//if( glConfig.fragmentProgramAvailable )
	{
		glGetIntegerv( GL_MAX_TEXTURE_COORDS, ( GLint* )&glConfig.maxTextureCoords );
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint* )&glConfig.maxTextureImageUnits );
	}

	// GLSL, core in OpenGL > 2.0
	glConfig.glslAvailable = ( glConfig.glVersion >= 2.0f );

	// GL_ARB_uniform_buffer_object
	glConfig.uniformBufferAvailable = GLEW_ARB_uniform_buffer_object != 0;
	if( glConfig.uniformBufferAvailable )
	{
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, ( GLint* )&glConfig.uniformBufferOffsetAlignment );
		if( glConfig.uniformBufferOffsetAlignment < 256 )
		{
			glConfig.uniformBufferOffsetAlignment = 256;
		}
	}
	// RB: make GPU skinning optional for weak OpenGL drivers
	glConfig.gpuSkinningAvailable = glConfig.uniformBufferAvailable && ( glConfig.driverType == GLDRV_OPENGL3X || glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE || glConfig.driverType == GLDRV_OPENGL32_COMPATIBILITY_PROFILE );

	// ATI_separate_stencil / OpenGL 2.0 separate stencil
	glConfig.twoSidedStencilAvailable = ( glConfig.glVersion >= 2.0f ) || GLEW_ATI_separate_stencil != 0;

	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = GLEW_EXT_depth_bounds_test != 0;

	// GL_ARB_sync
	glConfig.syncAvailable = GLEW_ARB_sync &&
							 // as of 5/24/2012 (driver version 15.26.12.64.2761) sync objects
							 // do not appear to work for the Intel HD 4000 graphics
							 ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() );

	// GL_ARB_occlusion_query
	glConfig.occlusionQueryAvailable = GLEW_ARB_occlusion_query != 0;

#if defined(__APPLE__)
	// SRS - DSA not available in Apple OpenGL 4.1, but enable for OSX anyways since elapsed time query will be used to get timing info instead
	glConfig.timerQueryAvailable = ( GLEW_ARB_timer_query != 0 || GLEW_EXT_timer_query != 0 );
#else
	// GL_ARB_timer_query using the DSA interface
	glConfig.timerQueryAvailable = ( GLEW_ARB_direct_state_access != 0 && GLEW_ARB_timer_query != 0 );
#endif

	// GREMEDY_string_marker
	glConfig.gremedyStringMarkerAvailable = GLEW_GREMEDY_string_marker != 0;
	if( glConfig.gremedyStringMarkerAvailable )
	{
		common->Printf( "...using %s\n", "GL_GREMEDY_string_marker" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_GREMEDY_string_marker" );
	}

	// KHR_debug
	glConfig.khronosDebugAvailable = GLEW_KHR_debug != 0;
	if( glConfig.khronosDebugAvailable )
	{
		common->Printf( "...using %s\n", "GLEW_KHR_debug" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GLEW_KHR_debug" );
	}

	// GL_ARB_framebuffer_object
	glConfig.framebufferObjectAvailable = GLEW_ARB_framebuffer_object != 0;
	if( glConfig.framebufferObjectAvailable )
	{
		glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig.maxRenderbufferSize );
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments );

		common->Printf( "...using %s\n", "GL_ARB_framebuffer_object" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_ARB_framebuffer_object" );
	}

	// GL_EXT_framebuffer_blit
	glConfig.framebufferBlitAvailable = GLEW_EXT_framebuffer_blit != 0;
	if( glConfig.framebufferBlitAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_framebuffer_blit" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_framebuffer_blit" );
	}

	// GL_ARB_debug_output
	glConfig.debugOutputAvailable = GLEW_ARB_debug_output != 0;
	if( glConfig.debugOutputAvailable )
	{
		if( r_debugContext.GetInteger() >= 1 )
		{
			glDebugMessageCallbackARB( ( GLDEBUGPROCARB ) DebugCallback, NULL );
		}
		if( r_debugContext.GetInteger() >= 2 )
		{
			// force everything to happen in the main thread instead of in a separate driver thread
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}
		if( r_debugContext.GetInteger() >= 3 )
		{
			// enable all the low priority messages
			glDebugMessageControlARB( GL_DONT_CARE,
									  GL_DONT_CARE,
									  GL_DEBUG_SEVERITY_LOW_ARB,
									  0, NULL, true );
		}
	}

	// GL_ARB_multitexture
	if( !glConfig.multitextureAvailable )
	{
		idLib::Error( "GL_ARB_multitexture not available" );
	}
	// GL_ARB_texture_compression + GL_EXT_texture_compression_s3tc
	if( !glConfig.textureCompressionAvailable )
	{
		idLib::Error( "GL_ARB_texture_compression or GL_EXT_texture_compression_s3tc not available" );
	}
	// GL_ARB_vertex_buffer_object
	if( !glConfig.vertexBufferObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_buffer_object not available" );
	}
	// GL_ARB_map_buffer_range
	if( !glConfig.mapBufferRangeAvailable )
	{
		idLib::Error( "GL_ARB_map_buffer_range not available" );
	}
	// GL_ARB_vertex_array_object
	if( !glConfig.vertexArrayObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_array_object not available" );
	}
	// GL_ARB_draw_elements_base_vertex
	if( !glConfig.drawElementsBaseVertexAvailable )
	{
		idLib::Error( "GL_ARB_draw_elements_base_vertex not available" );
	}
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	//if( !glConfig.fragmentProgramAvailable )
	//{
	//	idLib::Warning( "GL_ARB_fragment_program not available" );
	//}
	// GLSL
	if( !glConfig.glslAvailable )
	{
		idLib::Error( "GLSL not available" );
	}
	// GL_ARB_uniform_buffer_object
	if( !glConfig.uniformBufferAvailable )
	{
		idLib::Error( "GL_ARB_uniform_buffer_object not available" );
	}
	// GL_EXT_stencil_two_side
	if( !glConfig.twoSidedStencilAvailable )
	{
		idLib::Error( "GL_ATI_separate_stencil not available" );
	}
}
// RB end


idStr extensions_string;

extern DeviceManager* deviceManager;

/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If R_IsInitialized() is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void idRenderBackend::Init()
{
	common->Printf( "----- R_InitOpenGL -----\n" );

	if( tr.IsInitialized() )
	{
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// DG: make sure SDL has setup video so getting supported modes in R_SetNewMode() works
	GLimp_PreInit();
	// DG end

	R_SetNewMode( true );

	// input and sound systems need to be tied to the new window
	Sys_InitInput();

	// Need to reinitialize this pass once this image is resized.
	renderProgManager.Init( deviceManager->GetDevice() );

	bindingCache.Init( deviceManager->GetDevice() );
	samplerCache.Init( deviceManager->GetDevice() );
	pipelineCache.Init( deviceManager->GetDevice() );
	commonPasses.Init( deviceManager->GetDevice() );
	hiZGenPass = nullptr;
	ssaoPass = nullptr;
	//fowardShadingPass.Init( deviceManager->GetDevice() );

	tr.SetInitialized();

	if( !commandList )
	{
		commandList = deviceManager->GetDevice()->createCommandList();
	}

	// allocate the vertex array range or vertex objects
	commandList->open();
	vertexCache.Init( glConfig.uniformBufferOffsetAlignment, commandList );
	renderProgManager.CommitConstantBuffer( commandList );
	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	// Reset our gamma
	R_SetColorMappings();

	slopeScaleBias = 0.f;
	depthBias = 0.f;

	currentBindingSets.SetNum( currentBindingSets.Max() );
	pendingBindingSetDescs.SetNum( pendingBindingSetDescs.Max() );

	// RB: prepare ImGui system
	//ImGui_Init();
}

void idRenderBackend::Shutdown()
{
	delete ssaoPass;
	GLimp_Shutdown();
}

/*
=============
idRenderBackend::DrawElementsWithCounters
=============
*/
void idRenderBackend::DrawElementsWithCounters( const drawSurf_t* surf )
{
	// Get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const uint vertOffset = ( uint )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	bool changeState = false;

	if( currentVertexOffset != vertOffset )
	{
		currentVertexOffset = vertOffset;
	}

	if( currentVertexBuffer != ( nvrhi::IBuffer* )vertexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		currentVertexBuffer = vertexBuffer->GetAPIObject();
		changeState = true;
	}

	// Get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	const uint indexOffset = ( uint )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	if( currentIndexOffset != indexOffset )
	{
		currentIndexOffset = indexOffset;
	}

	RENDERLOG_PRINTF( "Binding Buffers: %p:%i %p:%i\n", vertexBuffer, vertOffset, indexBuffer, indexOffset );

	if( currentIndexBuffer != ( nvrhi::IBuffer* )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		currentIndexBuffer = indexBuffer->GetAPIObject();
		changeState = true;
	}

	GetCurrentBindingLayout();

	// RB: for debugging
	int program = renderProgManager.CurrentProgram();
	int bindingLayoutType = renderProgManager.BindingLayoutType();
	auto& info = renderProgManager.GetProgramInfo( program );
	/*
	if( info.cs )
	{
		renderLog.OpenBlock( info.cs->getDesc().debugName.c_str() );
	}
	else
	{
		renderLog.OpenBlock( info.ps->getDesc().debugName.c_str() );
	}
	*/

	for( int i = 0; i < info.bindingLayouts->Num(); i++ )
	{
		if( !currentBindingSets[i] || *currentBindingSets[i]->getDesc() != pendingBindingSetDescs[i] )
		{
			currentBindingSets[i] = bindingCache.GetOrCreateBindingSet( pendingBindingSetDescs[i], ( *info.bindingLayouts )[i] );
			changeState = true;
		}
	}

	renderProgManager.CommitConstantBuffer( commandList );

	PipelineKey key{ glStateBits, program, depthBias, slopeScaleBias, currentFrameBuffer };
	auto pipeline = pipelineCache.GetOrCreatePipeline( key );

	if( currentPipeline != pipeline )
	{
		currentPipeline = pipeline;
		changeState = true;
	}

	if( changeState )
	{
		nvrhi::GraphicsState state;

		for( int i = 0; i < info.bindingLayouts->Num(); i++ )
		{
			state.bindings.push_back( currentBindingSets[i] );
		}

		state.indexBuffer = { currentIndexBuffer, nvrhi::Format::R16_UINT, 0 };
		state.vertexBuffers = { { currentVertexBuffer, 0, 0 } };
		state.pipeline = pipeline;
		state.framebuffer = currentFrameBuffer->GetApiObject();

		nvrhi::Viewport viewport{ ( float )currentViewport.x1,
								  ( float )currentViewport.x2,
								  ( float )currentViewport.y1,
								  ( float )currentViewport.y2,
								  currentViewport.zmin,
								  currentViewport.zmax };
		state.viewport.addViewportAndScissorRect( viewport );

		if( !currentScissor.IsEmpty() )
		{
			state.viewport.addScissorRect( nvrhi::Rect( currentScissor.x1, currentScissor.x2, currentScissor.y1, currentScissor.y2 ) );
		}

		commandList->setGraphicsState( state );
	}

	nvrhi::DrawArguments args;
	// FIXME idDrawShadowVert
	args.startVertexLocation = currentVertexOffset / sizeof( idDrawVert );
	args.startIndexLocation = currentIndexOffset / sizeof( uint16 );
	args.vertexCount = surf->numIndexes;
	commandList->drawIndexed( args );

	// RB: added stats
	pc.c_drawElements++;
	pc.c_drawIndexes += surf->numIndexes;

	//renderLog.CloseBlock();
}

void idRenderBackend::GetCurrentBindingLayout()
{
	auto& info = renderProgManager.GetProgramInfo( renderProgManager.CurrentProgram() );

	int type = info.bindingLayoutType;

	if( type == BINDING_LAYOUT_DEFAULT )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			//nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_PointWrapSampler )
			nvrhi::BindingSetItem::Sampler( 0, ( nvrhi::ISampler* )GetImageAt( 0 )->GetSampler( samplerCache ) )
		};
	}
	else if( type == BINDING_LAYOUT_GBUFFER )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() )
		};
	}
	else if( type == BINDING_LAYOUT_AMBIENT_LIGHTING_IBL )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 3, ( nvrhi::ITexture* )GetImageAt( 3 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 4, ( nvrhi::ITexture* )GetImageAt( 4 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 7, ( nvrhi::ITexture* )GetImageAt( 7 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 8, ( nvrhi::ITexture* )GetImageAt( 8 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 9, ( nvrhi::ITexture* )GetImageAt( 9 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 10, ( nvrhi::ITexture* )GetImageAt( 10 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_AnisotropicWrapSampler ),
			nvrhi::BindingSetItem::Sampler( 1, commonPasses.m_LinearClampSampler )
		};
	}
	else if( type == BINDING_LAYOUT_DRAW_AO )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_PointWrapSampler )  // blue noise
		};
	}
	/*
	else if( renderProgManager.BindingLayoutType() == BINDING_LAYOUT_DRAW_AO1 )
	{
		bindingSetDesc
		.addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 0, ( nvrhi::ISampler* )GetImageAt( 0 )->GetSampler( samplerCache ) ) );
	}
	*/
	else if( type == BINDING_LAYOUT_DRAW_INTERACTION )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 3, ( nvrhi::ITexture* )GetImageAt( 3 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 4, ( nvrhi::ITexture* )GetImageAt( 4 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_AnisotropicWrapSampler ),
			nvrhi::BindingSetItem::Sampler( 1, commonPasses.m_LinearClampSampler )
		};
	}
	else if( type == BINDING_LAYOUT_DRAW_INTERACTION_SM )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 3, ( nvrhi::ITexture* )GetImageAt( 3 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 4, ( nvrhi::ITexture* )GetImageAt( 4 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 5, ( nvrhi::ITexture* )GetImageAt( 5 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 6, ( nvrhi::ITexture* )GetImageAt( 6 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_AnisotropicWrapSampler ),
			nvrhi::BindingSetItem::Sampler( 1, commonPasses.m_LinearClampSampler ),
			nvrhi::BindingSetItem::Sampler( 2, commonPasses.m_LinearClampCompareSampler ),
			nvrhi::BindingSetItem::Sampler( 3, commonPasses.m_PointWrapSampler )  // blue noise
		};
	}
	else if( type == BINDING_LAYOUT_DRAW_FOG )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_LinearClampSampler ),
			nvrhi::BindingSetItem::Sampler( 1, commonPasses.m_LinearClampSampler )
		};
	}
	else if( type == BINDING_LAYOUT_POST_PROCESS_CNM )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_LinearClampSampler )
		};
	}
	else if( type == BINDING_LAYOUT_NORMAL_CUBE )
	{
		pendingBindingSetDescs[0].bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
			nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ),
			nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() )
		};

		pendingBindingSetDescs[1].bindings =
		{
			nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_LinearWrapSampler )
		};
	}
	else
	{
		common->FatalError( "Invalid binding set %d\n", renderProgManager.BindingLayoutType() );
	}
}

/*
=========================================================================================================

GL COMMANDS

=========================================================================================================
*/

/*
==================
idRenderBackend::GL_StartFrame
==================
*/
void idRenderBackend::GL_StartFrame()
{
	deviceManager->BeginFrame();

	commandList->open();
}

/*
==================
idRenderBackend::GL_EndFrame
==================
*/
void idRenderBackend::GL_EndFrame()
{
	commandList->close();

	deviceManager->GetDevice()->executeCommandList( commandList );

	// Make sure that all frames have finished rendering
	deviceManager->GetDevice()->waitForIdle();

	// Release all in-flight references to the render targets
	deviceManager->GetDevice()->runGarbageCollection();

	// Present to the swap chain.
	deviceManager->Present();
}

void idRenderBackend::GL_EndRenderPass()
{
#if defined( USE_NVRHI )
	commandList->close();

	deviceManager->GetDevice()->executeCommandList( commandList );

	deviceManager->GetDevice()->runGarbageCollection();

	commandList->open();
#endif
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
void idRenderBackend::GL_BlockingSwapBuffers()
{
	// Make sure that all frames have finished rendering
	deviceManager->GetDevice()->waitForIdle();
}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void idRenderBackend::GL_SetDefaultState()
{
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );

	// make sure our GL state vector is set correctly
	memset( &glcontext.tmu, 0, sizeof( glcontext.tmu ) );

	glStateBits = 0;

	GL_State( 0, true );

	GL_Scissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );

	renderProgManager.Unbind();

	Framebuffer::Unbind();
}

/*
====================
idRenderBackend::GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void idRenderBackend::GL_State( uint64 stateBits, bool forceGlState )
{
	glStateBits = stateBits | ( glStateBits & GLS_KEEP );
	if( viewDef != NULL && viewDef->isMirror )
	{
		glStateBits |= GLS_MIRROR_VIEW;
	}

	// the rest of this is handled by PipelineCache::GetOrCreatePipeline and GetRenderState similar to Vulkan
}

/*
====================
idRenderBackend::SelectTexture
====================
*/
void idRenderBackend::GL_SelectTexture( int unit )
{
	context.currentImageParm = unit;
}

/*
====================
idRenderBackend::GL_Scissor
====================
*/
void idRenderBackend::GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
	// TODO Check if this is right.
	//currentScissor.Clear();
	//currentScissor.AddPoint( x, y );
	//currentScissor.AddPoint( x + w, y + h );
}

/*
====================
idRenderBackend::GL_Viewport
====================
*/
void idRenderBackend::GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
	currentViewport.Clear();
	currentViewport.AddPoint( x, y );
	currentViewport.AddPoint( x + w, y + h );
}

/*
====================
idRenderBackend::GL_PolygonOffset
====================
*/
void idRenderBackend::GL_PolygonOffset( float scale, float bias )
{
	slopeScaleBias = scale;
	depthBias = bias;
}

/*
========================
idRenderBackend::GL_DepthBoundsTest
========================
*/
void idRenderBackend::GL_DepthBoundsTest( const float zmin, const float zmax )
{
	if( /*!glConfig.depthBoundsTestAvailable || */zmin > zmax )
	{
		return;
	}

	if( zmin == 0.0f && zmax == 0.0f )
	{
	}
	else
	{
		currentViewport.zmin = zmin;
		currentViewport.zmax = zmax;
	}
}

/*
====================
idRenderBackend::GL_Color
====================
*/
void idRenderBackend::GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

/*
========================
idRenderBackend::GL_Clear
========================
*/
void idRenderBackend::GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR )
{
	// TODO: Do something if there is no depth-stencil attachment.
	if( color )
	{
		nvrhi::utils::ClearColorAttachment( commandList, Framebuffer::GetActiveFramebuffer()->GetApiObject(), 0, nvrhi::Color( 0.f ) );
	}

	if( clearHDR )
	{
		nvrhi::utils::ClearColorAttachment( commandList, globalFramebuffers.hdrFBO->GetApiObject(), 0, nvrhi::Color( 0.f ) );
	}

	if( depth )
	{
		nvrhi::ITexture* depthTexture = ( nvrhi::ITexture* )( globalImages->currentDepthImage->GetTextureID() );
		const nvrhi::FormatInfo& depthFormatInfo = nvrhi::getFormatInfo( depthTexture->getDesc().format );
		commandList->clearDepthStencilTexture( depthTexture, nvrhi::AllSubresources, true, 1.f, depthFormatInfo.hasStencil, 0 );
	}
}


/*
=================
idRenderBackend::GL_GetCurrentState
=================
*/
uint64 idRenderBackend::GL_GetCurrentState() const
{
	return glStateBits;
}

/*
========================
idRenderBackend::GL_GetCurrentStateMinusStencil
========================
*/
uint64 idRenderBackend::GL_GetCurrentStateMinusStencil() const
{
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );
}


/*
=============
idRenderBackend::CheckCVars

See if some cvars that we watch have changed
=============
*/
void idRenderBackend::CheckCVars()
{
	// gamma stuff
	if( r_gamma.IsModified() || r_brightness.IsModified() )
	{
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		R_SetColorMappings();
	}

	// filtering
	/*if( r_maxAnisotropicFiltering.IsModified() || r_useTrilinearFiltering.IsModified() || r_lodBias.IsModified() )
	{
		idLib::Printf( "Updating texture filter parameters.\n" );
		r_maxAnisotropicFiltering.ClearModified();
		r_useTrilinearFiltering.ClearModified();
		r_lodBias.ClearModified();

		for( int i = 0; i < globalImages->images.Num(); i++ )
		{
			if( globalImages->images[i] )
			{
				globalImages->images[i]->Bind();
				globalImages->images[i]->SetTexParameters();
			}
		}
	}*/

	if( r_useSeamlessCubeMap.IsModified() )
	{
		r_useSeamlessCubeMap.ClearModified();
		if( glConfig.seamlessCubeMapAvailable )
		{
			if( r_useSeamlessCubeMap.GetBool() )
			{
				//glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
			else
			{
				//glDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
		}
	}


	// SRS - Enable SDL-driven vync changes without restart for UNIX-like OSs
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	extern idCVar r_swapInterval;
	if( r_swapInterval.IsModified() )
	{
		r_swapInterval.ClearModified();
#if SDL_VERSION_ATLEAST(2, 0, 0)
		if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "Vsync changes not supported without restart" );
		}
#else
		if( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "Vsync changes not supported without restart" );
		}
#endif
	}
#endif
	// SRS end
	if( r_antiAliasing.IsModified() )
	{
		switch( r_antiAliasing.GetInteger() )
		{
			case ANTI_ALIASING_MSAA_2X:
			case ANTI_ALIASING_MSAA_4X:
			case ANTI_ALIASING_MSAA_8X:
				if( r_antiAliasing.GetInteger() > 0 )
				{
					//glEnable( GL_MULTISAMPLE );
				}
				break;

			default:
				//glDisable( GL_MULTISAMPLE );
				break;
		}
	}

	if( r_usePBR.IsModified() ||
			r_useHDR.IsModified() ||
			r_useHalfLambertLighting.IsModified() ||
			r_pbrDebug.IsModified() )
	{
		bool needShaderReload = false;

		if( r_usePBR.GetBool() && r_useHalfLambertLighting.GetBool() )
		{
			r_useHalfLambertLighting.SetBool( false );

			needShaderReload = true;
		}

		needShaderReload |= r_useHDR.IsModified();
		needShaderReload |= r_pbrDebug.IsModified();

		r_usePBR.ClearModified();
		r_useHDR.ClearModified();
		r_useHalfLambertLighting.ClearModified();
		r_pbrDebug.ClearModified();

		renderProgManager.KillAllShaders();
		renderProgManager.LoadAllShaders();
	}

	// RB: turn off shadow mapping for OpenGL drivers that are too slow
	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_ES2:
		case GLDRV_OPENGL_ES3:
			//case GLDRV_OPENGL_MESA:
			r_useShadowMapping.SetInteger( 0 );
			break;

		default:
			break;
	}
	// RB end
}

void idRenderBackend::BackBufferResizing()
{
	currentPipeline = nullptr;
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
idRenderBackend::DrawFlickerBox
=============
*/
void idRenderBackend::DrawFlickerBox()
{
	if( !r_drawFlickerBox.GetBool() )
	{
		return;
	}
	//if( tr.frameCount & 1 )
	//{
	//	glClearColor( 1, 0, 0, 1 );
	//}
	//else
	//{
	//	glClearColor( 0, 1, 0, 1 );
	//}
	//glScissor( 0, 0, 256, 256 );
	//glClear( GL_COLOR_BUFFER_BIT );
}

/*
=============
idRenderBackend::SetBuffer
=============
*/
void idRenderBackend::SetBuffer( const void* data )
{
	// see which draw buffer we want to render the frame to

	const setBufferCommand_t* cmd = ( const setBufferCommand_t* )data;

	//RENDERLOG_PRINTF( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );

	renderLog.OpenBlock( "Render_SetBuffer" );

	currentScissor.Clear();
	currentScissor.AddPoint( 0, 0 );
	currentScissor.AddPoint( tr.GetWidth(), tr.GetHeight() );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() )
	{
		float c[3];
		if( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 )
		{
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f, true );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f, true );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f, true );
		}
		else
		{
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f, true );
		}
	}

	renderLog.CloseBlock();
}

/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/

/*
=====================
idRenderBackend::DrawStencilShadowPass
=====================
*/
extern idCVar r_useStencilShadowPreload;

void idRenderBackend::DrawStencilShadowPass( const drawSurf_t* drawSurf, const bool renderZPass )
{
}


/*
=============
idRenderBackend::idRenderBackend
=============
*/
idRenderBackend::idRenderBackend()
{
	glcontext.frameCounter = 0;
	glcontext.frameParity = 0;
	hiZGenPass = nullptr;
	ssaoPass = nullptr;

	memset( glcontext.tmu, 0, sizeof( glcontext.tmu ) );
	memset( glcontext.stencilOperations, 0, sizeof( glcontext.stencilOperations ) );

	memset( glcontext.renderLogMainBlockTimeQueryIds, 0, sizeof( glcontext.renderLogMainBlockTimeQueryIds ) );
	memset( glcontext.renderLogMainBlockTimeQueryIssued, 0, sizeof( glcontext.renderLogMainBlockTimeQueryIssued ) );
}

/*
=============
idRenderBackend::~idRenderBackend
=============
*/
idRenderBackend::~idRenderBackend()
{

}

/*
====================
R_MakeStereoRenderImage
====================
*/
static void R_MakeStereoRenderImage( idImage* image )
{
	idImageOpts	opts;
	opts.width = renderSystem->GetWidth();
	opts.height = renderSystem->GetHeight();
	opts.numLevels = 1;
	opts.format = FMT_RGBA8;
	image->AllocImage( opts, TF_LINEAR, TR_CLAMP );
}

/*
====================
idRenderBackend::StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void idRenderBackend::StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
{
}

void idRenderBackend::ImGui_RenderDrawLists( ImDrawData* draw_data )
{
	if( draw_data->CmdListsCount == 0 )
	{
		// Nothing to do.
		return;
	}

#if IMGUI_BFGUI
	tr.guiModel->EmitImGui( draw_data );
#endif
}

/*
====================
idRenderBackend::ResizeImages
====================
*/
void idRenderBackend::ResizeImages()
{
}

void idRenderBackend::SetCurrentImage( idImage* image )
{
	// load the image if necessary (FIXME: not SMP safe!)
	// RB: don't try again if last time failed
	if( !image->IsLoaded() && !image->IsDefaulted() )
	{
		// TODO(Stephen): Fix me.
		image->FinalizeImage( true, commandList );
	}

	context.imageParms[context.currentImageParm] = image;
}

idImage* idRenderBackend::GetCurrentImage()
{
	return context.imageParms[context.currentImageParm];
}

idImage* idRenderBackend::GetImageAt( int index )
{
	return context.imageParms[index];
}

void idRenderBackend::ResetPipelineCache()
{
	pipelineCache.Clear();
}
