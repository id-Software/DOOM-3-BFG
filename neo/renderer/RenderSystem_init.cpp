/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2021 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop

#include "libs/imgui/imgui.h"

#include "RenderCommon.h"

#if defined( USE_NVRHI )
	#include "sys/DeviceManager.h"
#endif

// RB begin
#if defined(_WIN32)

	// Vista OpenGL wrapper check
	#include "../sys/win32/win_local.h"
#endif
// RB end

// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#define BUGFIXEDSCREENSHOTRESOLUTION 1
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	#include "../framework/Common_local.h"
#endif

//#include "idlib/HandleManager.h"


// DeviceContext bypasses RenderSystem to work directly with this
idGuiModel* tr_guiModel;

// functions that are not called every frame
glconfig_t	glConfig;

idCVar r_requestStereoPixelFormat( "r_requestStereoPixelFormat", "1", CVAR_RENDERER, "Ask for a stereo GL pixel format on startup" );
idCVar r_debugContext( "r_debugContext", "0", CVAR_RENDERER, "Enable various levels of context debug." );
#if defined( USE_NVRHI )
	#if defined( _WIN32 )
		idCVar r_graphicsAPI( "r_graphicsAPI", "dx12", CVAR_RENDERER | CVAR_INIT | CVAR_ARCHIVE, "Specifies the graphics api to use (dx12, vulkan)" );
	#else
		idCVar r_graphicsAPI( "r_graphicsAPI", "vulkan", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC, "Specifies the graphics api to use (vulkan)" );
	#endif

	idCVar r_useValidationLayers( "r_useValidationLayers", "1", CVAR_INTEGER | CVAR_INIT, "1 is just the NVRHI and 2 will turn on additional DX12, VK validation layers" );
#endif
// SRS - Added workaround for AMD OSX driver bugs caused by GL_EXT_timer_query when shadow mapping enabled; Intel bugs not present on OSX
#if defined(__APPLE__)
	idCVar r_skipIntelWorkarounds( "r_skipIntelWorkarounds", "1", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for Intel driver bugs" );
	idCVar r_skipAMDWorkarounds( "r_skipAMDWorkarounds", "0", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for AMD driver bugs" );
#else
	idCVar r_skipIntelWorkarounds( "r_skipIntelWorkarounds", "0", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for Intel driver bugs" );
	idCVar r_skipAMDWorkarounds( "r_skipAMDWorkarounds", "1", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for AMD driver bugs" );
#endif
// SRS end
// RB: disabled 16x MSAA
#if ID_MSAA
	idCVar r_antiAliasing( "r_antiAliasing", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, " 0 = None\n 1 = TAA 1x\n 2 = TAA + SMAA 1x\n 3 = MSAA 2x\n 4 = MSAA 4x\n", 0, ANTI_ALIASING_MSAA_4X );
#else
	idCVar r_antiAliasing( "r_antiAliasing", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, " 0 = None\n 1 = TAA 1x", 0, ANTI_ALIASING_TAA );
#endif
// RB end
idCVar r_vidMode( "r_vidMode", "0", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "fullscreen video mode number" );
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 240.0f );
#ifdef WIN32
	idCVar r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
#else
	// DG: add mode -2 for SDL, also defaulting to windowed mode, as that causes less trouble on linux
	idCVar r_fullscreen( "r_fullscreen", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "-2 = use current monitor, -1 = (reserved), 0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
	// DG end
#endif
idCVar r_customWidth( "r_customWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_vidMode to -1 to activate" );
idCVar r_customHeight( "r_customHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_vidMode to -1 to activate" );
idCVar r_windowX( "r_windowX", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowY( "r_windowY", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowWidth( "r_windowWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowHeight( "r_windowHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );

idCVar r_useViewBypass( "r_useViewBypass", "1", CVAR_RENDERER | CVAR_INTEGER, "bypass a frame of latency to the view" );
idCVar r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );
idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );
idCVar r_useSeamlessCubeMap( "r_useSeamlessCubeMap", "1", CVAR_RENDERER | CVAR_BOOL, "use ARB_seamless_cube_map if available" );
idCVar r_maxAnisotropicFiltering( "r_maxAnisotropicFiltering", "8", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "limit aniso filtering" );
idCVar r_useTrilinearFiltering( "r_useTrilinearFiltering", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "Extra quality filtering" );
// RB: not used anymore
idCVar r_lodBias( "r_lodBias", "0.5", CVAR_RENDERER | CVAR_ARCHIVE, "UNUSED: image lod bias" );
// RB end

idCVar r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );

idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", "1", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_swapInterval( "r_swapInterval", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = tear, 1 = swap-tear where available, 2 = always v-sync" );

idCVar r_gamma( "r_gamma", "1.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 3.0f );
idCVar r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

idCVar r_skipStaticInteractions( "r_skipStaticInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created at level load" );
idCVar r_skipDynamicInteractions( "r_skipDynamicInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created after level load" );
idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
#if defined( USE_VULKAN )
	idCVar r_skipPostProcess( "r_skipPostProcess", "1", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings except bloom" );
#else
	idCVar r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings except bloom" );
#endif
idCVar r_skipBloom( "r_skipBloom", "0", CVAR_RENDERER | CVAR_BOOL, "Skip bloom" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
// RB begin
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "DISABLED: NULL the rendering context during backend 3D rendering" );
// RB end
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all non-interaction drawing" );
idCVar r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
idCVar r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all fog lights" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipDecals( "r_skipDecals", "0", CVAR_RENDERER | CVAR_BOOL, "skip decal surfaces" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_INTEGER, "use black for diffuse" );
idCVar r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_skipShadows( "r_skipShadows", "0", CVAR_RENDERER | CVAR_BOOL  | CVAR_ARCHIVE, "disable shadows" );

idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
idCVar r_useLightScissors( "r_useLightScissors", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = no scissor, 1 = non-clipped scissor, 2 = near-clipped scissor, 3 = fully-clipped scissor", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );

idCVar r_offsetFactor( "r_offsetfactor", "0", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
// RB: offset factor was 0, and units were -600 which caused some very ugly polygon offsets on Android so I reverted the values to the same as in Q3A
#if defined(__ANDROID__)
	idCVar r_offsetUnits( "r_offsetunits", "-2", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
#else
	idCVar r_offsetUnits( "r_offsetunits", "-600", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
#endif
// RB end

idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "scale value for stencil shadow drawing" );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "3", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this", 0, 100 );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_skipPrelightShadows( "r_skipPrelightShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip the dmap generated static shadow volumes" );
idCVar r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
idCVar r_useLightDepthBounds( "r_useLightDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on lights to reduce both shadow and interaction fill" );
idCVar r_useShadowDepthBounds( "r_useShadowDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on individual shadow volumes to reduce shadow fill" );
// RB begin
idCVar r_useHalfLambertLighting( "r_useHalfLambertLighting", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use Half-Lambert lighting instead of classic Lambert, requires reloadShaders" );
// RB end

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleEnvprobe( "r_singleEnvprobe", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one environment probe" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );

// visual debugging info
idCVar r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color based on passed / not passed" );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all, 4 = draw with alpha", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showAddModel( "r_showAddModel", "0", CVAR_RENDERER | CVAR_BOOL, "report stats from tr_addModel" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
// RB begin
idCVar r_showShadowMaps( "r_showShadowMaps", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_showShadowMapLODs( "r_showShadowMapLODs", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
// RB end

idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

idCVar stereoRender_enable( "stereoRender_enable", "0", CVAR_INTEGER | CVAR_ARCHIVE, "1 = side-by-side compressed, 2 = top and bottom compressed, 3 = side-by-side, 4 = 720 frame packed, 5 = interlaced, 6 = OpenGL quad buffer" );
idCVar stereoRender_swapEyes( "stereoRender_swapEyes", "0", CVAR_BOOL | CVAR_ARCHIVE, "reverse eye adjustments" );
idCVar stereoRender_deGhost( "stereoRender_deGhost", "0.05", CVAR_FLOAT | CVAR_ARCHIVE, "subtract from opposite eye to reduce ghosting" );

idCVar r_useVirtualScreenResolution( "r_useVirtualScreenResolution", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "do 2D rendering at 640x480 and stretch to the current resolution" );

// RB: shadow mapping parameters
#if defined( USE_VULKAN )
	idCVar r_useShadowMapping( "r_useShadowMapping", "0", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC | CVAR_INTEGER, "use shadow mapping instead of stencil shadows" );
#else
	idCVar r_useShadowMapping( "r_useShadowMapping", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "use shadow mapping instead of stencil shadows" );
#endif
idCVar r_useShadowAtlas( "r_useShadowAtlas", "1", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_shadowMapAtlasSize( "r_shadowMapAtlasSize", "16384", CVAR_RENDERER | CVAR_INTEGER | CVAR_ROM, "size of the shadowmap atlas" );
idCVar r_shadowMapFrustumFOV( "r_shadowMapFrustumFOV", "92", CVAR_RENDERER | CVAR_FLOAT, "oversize FOV for point light side matching" );
idCVar r_shadowMapSingleSide( "r_shadowMapSingleSide", "-1", CVAR_RENDERER | CVAR_INTEGER, "only draw a single side (0-5) of point lights" );
idCVar r_shadowMapImageSize( "r_shadowMapImageSize", "1024", CVAR_RENDERER | CVAR_INTEGER, "", 128, 2048 );
idCVar r_shadowMapJitterScale( "r_shadowMapJitterScale", "2.5", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter offset" );
idCVar r_shadowMapBiasScale( "r_shadowMapBiasScale", "0.0001", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter bias" );
idCVar r_shadowMapRandomizeJitter( "r_shadowMapRandomizeJitter", "1", CVAR_RENDERER | CVAR_BOOL, "randomly offset jitter texture each draw" );
idCVar r_shadowMapSamples( "r_shadowMapSamples", "16", CVAR_RENDERER | CVAR_INTEGER, "1, 4, 12 or 16", 1, 64 );
idCVar r_shadowMapSplits( "r_shadowMapSplits", "3", CVAR_RENDERER | CVAR_INTEGER, "number of splits for cascaded shadow mapping with parallel lights", 0, 4 );
idCVar r_shadowMapSplitWeight( "r_shadowMapSplitWeight", "0.9", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodScale( "r_shadowMapLodScale", "1.4", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodBias( "r_shadowMapLodBias", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_shadowMapPolygonFactor( "r_shadowMapPolygonFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer" );
idCVar r_shadowMapPolygonOffset( "r_shadowMapPolygonOffset", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer" );
idCVar r_shadowMapOccluderFacing( "r_shadowMapOccluderFacing", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = front faces, 1 = back faces, 2 = twosided" );
idCVar r_shadowMapRegularDepthBiasScale( "r_shadowMapRegularDepthBiasScale", "0.999", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for point and spot lights" );
idCVar r_shadowMapSunDepthBiasScale( "r_shadowMapSunDepthBiasScale", "0.999991", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for cascaded shadow mapping with parallel lights" );

// RB: HDR parameters
#if defined( USE_VULKAN )
	idCVar r_useHDR( "r_useHDR", "0", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC | CVAR_BOOL, "Can't be changed, is broken on Vulkan backend" );
#else
	idCVar r_useHDR( "r_useHDR", "1", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC | CVAR_BOOL, "Can't be changed: Use high dynamic range rendering" );
#endif

idCVar r_hdrAutoExposure( "r_hdrAutoExposure", "0", CVAR_RENDERER | CVAR_BOOL, "EXPENSIVE: enables adapative HDR tone mapping otherwise the exposure is derived by r_exposure" );
idCVar r_hdrAdaptionRate( "r_hdrAdaptionRate", "1", CVAR_RENDERER | CVAR_FLOAT, "The rate of adapting the hdr exposure value`. Defaulted to a second." );
idCVar r_hdrMinLuminance( "r_hdrMinLuminance", "0.02", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrMaxLuminance( "r_hdrMaxLuminance", "0.5", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrKey( "r_hdrKey", "0.015", CVAR_RENDERER | CVAR_FLOAT, "magic exposure key that works well with Doom 3 maps" );
idCVar r_hdrContrastDynamicThreshold( "r_hdrContrastDynamicThreshold", "2", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is on, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastStaticThreshold( "r_hdrContrastStaticThreshold", "3", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is off, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastOffset( "r_hdrContrastOffset", "100", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrGlarePasses( "r_hdrGlarePasses", "8", CVAR_RENDERER | CVAR_INTEGER, "how many times the bloom blur is rendered offscreen. number should be even" );
idCVar r_hdrDebug( "r_hdrDebug", "0", CVAR_RENDERER | CVAR_FLOAT, "show scene luminance as heat map" );

idCVar r_ldrContrastThreshold( "r_ldrContrastThreshold", "1.1", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_ldrContrastOffset( "r_ldrContrastOffset", "3", CVAR_RENDERER | CVAR_FLOAT, "" );

idCVar r_useFilmicPostProcessing( "r_useFilmicPostProcessing", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "apply several post process effects to mimic a filmic look" );

#if defined( USE_VULKAN )
	idCVar r_forceAmbient( "r_forceAmbient", "0.5", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 0.75f );
#else
	idCVar r_forceAmbient( "r_forceAmbient", "0.5", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 1.0f );
#endif

idCVar r_useSSGI( "r_useSSGI", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space global illumination and reflections" );
idCVar r_ssgiDebug( "r_ssgiDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssgiFiltering( "r_ssgiFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );

#if defined( USE_VULKAN )
	idCVar r_useSSAO( "r_useSSAO", "0", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
#else
	idCVar r_useSSAO( "r_useSSAO", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
#endif
idCVar r_ssaoDebug( "r_ssaoDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssaoFiltering( "r_ssaoFiltering", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useHierarchicalDepthBuffer( "r_useHierarchicalDepthBuffer", "1", CVAR_RENDERER | CVAR_BOOL, "" );

// RB: only change r_usePBR if you are a developer
idCVar r_usePBR( "r_usePBR", "1", CVAR_RENDERER | CVAR_ROM | CVAR_STATIC | CVAR_BOOL, "use PBR and Image Based Lighting instead of old Quake 4 style ambient lighting" );
idCVar r_pbrDebug( "r_pbrDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "show which materials have PBR support (green = PBR, red = oldschool D3)" );
idCVar r_showViewEnvprobes( "r_showViewEnvprobes", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view environment probes, 2 = show irradiance" );
idCVar r_showLightGrid( "r_showLightGrid", "0", CVAR_RENDERER | CVAR_INTEGER, "show Quake 3 style light grid points" );

idCVar r_useLightGrid( "r_useLightGrid", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_exposure( "r_exposure", "0.5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "HDR exposure or LDR brightness [-4.0 .. 4.0]", -4.0f, 4.0f );

idCVar r_useTemporalAA( "r_useTemporalAA", "1", CVAR_RENDERER | CVAR_BOOL, "only disable for debugging" );
idCVar r_taaJitter( "r_taaJitter", "3", CVAR_RENDERER | CVAR_INTEGER, "0: None, 1: MSAA, 2: Halton, 3: R2 Sequence, 4: White Noise" );
idCVar r_taaEnableHistoryClamping( "r_taaEnableHistoryClamping", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_taaClampingFactor( "r_taaClampingFactor", "1.0", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_taaNewFrameWeight( "r_taaNewFrameWeight", "0.1", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_taaMaxRadiance( "r_taaMaxRadiance", "10000", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_taaMotionVectors( "r_taaMotionVectors", "1", CVAR_RENDERER | CVAR_BOOL, "" );
// RB end

const char* fileExten[4] = { "tga", "png", "jpg", "exr" };
const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };
const char* skyDirection[6] = { "_forward", "_back", "_left", "_right", "_up", "_down" };

#if defined( USE_NVRHI )
	DeviceManager* deviceManager;
#endif


bool R_UseTemporalAA()
{
	if( !r_useTemporalAA.GetBool() )
	{
		return false;
	}

	switch( r_antiAliasing.GetInteger() )
	{
		case ANTI_ALIASING_TAA:
			return true;

#if ID_MSAA
		case ANTI_ALIASING_TAA_SMAA_1X:
			return true;
#endif

		default:
			return false;
	}
}

uint R_GetMSAASamples()
{
#if ID_MSAA
	switch( r_antiAliasing.GetInteger() )
	{
		case ANTI_ALIASING_MSAA_2X:
			return 2;

		case ANTI_ALIASING_MSAA_4X:
			return 4;

		default:
			return 1;
	}
#else
	return 1;
#endif
}

/*
=============================
R_SetNewMode

r_fullScreen -1		borderless window at exact desktop coordinates
r_fullScreen 0		bordered window at exact desktop coordinates
r_fullScreen 1		fullscreen on monitor 1 at r_vidMode
r_fullScreen 2		fullscreen on monitor 2 at r_vidMode
...

r_vidMode -1		use r_customWidth / r_customHeight, even if they don't appear on the mode list
r_vidMode 0			use first mode returned by EnumDisplaySettings()
r_vidMode 1			use second mode returned by EnumDisplaySettings()
...

r_displayRefresh 0	don't specify refresh
r_displayRefresh 70	specify 70 hz, etc
=============================
*/
void R_SetNewMode( const bool fullInit )
{
	// try up to three different configurations

	for( int i = 0 ; i < 3 ; i++ )
	{
		if( i == 0 && stereoRender_enable.GetInteger() != STEREO3D_QUAD_BUFFER )
		{
			continue;		// don't even try for a stereo mode
		}

		glimpParms_t	parms;

		if( r_fullscreen.GetInteger() <= 0 )
		{
			// use explicit position / size for window
			parms.x = r_windowX.GetInteger();
			parms.y = r_windowY.GetInteger();
			parms.width = r_windowWidth.GetInteger();
			parms.height = r_windowHeight.GetInteger();
			// may still be -1 to force a borderless window
			parms.fullScreen = r_fullscreen.GetInteger();
			parms.displayHz = 0;		// ignored
		}
		else
		{
			// get the mode list for this monitor
			idList<vidMode_t> modeList;
			if( !R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList ) )
			{
				idLib::Printf( "r_fullscreen reset from %i to 1 because mode list failed.", r_fullscreen.GetInteger() );
				r_fullscreen.SetInteger( 1 );
				R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList );
			}

			if( modeList.Num() < 1 )
			{
				idLib::Printf( "Going to safe mode because mode list failed." );
				goto safeMode;
			}

			parms.x = 0;		// ignored
			parms.y = 0;		// ignored
			parms.fullScreen = r_fullscreen.GetInteger();

			// set the parameters we are trying
			if( r_vidMode.GetInteger() < 0 )
			{
				// try forcing a specific mode, even if it isn't on the list
				parms.width = r_customWidth.GetInteger();
				parms.height = r_customHeight.GetInteger();
				parms.displayHz = r_displayRefresh.GetInteger();
			}
			else
			{
				if( r_vidMode.GetInteger() >= modeList.Num() )
				{
					idLib::Printf( "r_vidMode reset from %i to 0.\n", r_vidMode.GetInteger() );
					r_vidMode.SetInteger( 0 );
				}

				parms.width = modeList[ r_vidMode.GetInteger() ].width;
				parms.height = modeList[ r_vidMode.GetInteger() ].height;
				parms.displayHz = modeList[ r_vidMode.GetInteger() ].displayHz;
			}
		}

		switch( r_antiAliasing.GetInteger() )
		{
#if ID_MSAA
			case ANTI_ALIASING_MSAA_2X:
				parms.multiSamples = 2;
				break;
			case ANTI_ALIASING_MSAA_4X:
				parms.multiSamples = 4;
				break;
#endif

			default:
				parms.multiSamples = 1;
				break;
		}

		if( i == 0 )
		{
			parms.stereo = ( stereoRender_enable.GetInteger() == STEREO3D_QUAD_BUFFER );
		}
		else
		{
			parms.stereo = false;
		}

		if( fullInit )
		{
			// create the context as well as setting up the window

#if defined( USE_NVRHI )
			nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::D3D12;
			if( !idStr::Icmp( r_graphicsAPI.GetString(), "vulkan" ) )
			{
				api = nvrhi::GraphicsAPI::VULKAN;
			}
			else if( !idStr::Icmp( r_graphicsAPI.GetString(), "dx12" ) )
			{
				api = nvrhi::GraphicsAPI::D3D12;
			}
			deviceManager = DeviceManager::Create( api );
#endif

#if defined( VULKAN_USE_PLATFORM_SDL )
			if( VKimp_Init( parms ) )
#else
			if( GLimp_Init( parms ) )
#endif
			{
				ImGuiHook::Init( parms.width, parms.height );
				break;
			}
		}
		else
		{
			// just rebuild the window

#if defined( VULKAN_USE_PLATFORM_SDL )
			if( VKimp_SetScreenParms( parms ) )
#else
			if( GLimp_SetScreenParms( parms ) )
#endif
			{
				Framebuffer::ResizeFramebuffers();
				ImGuiHook::NotifyDisplaySizeChanged( parms.width, parms.height );
				break;
			}
		}

		if( i == 2 )
		{
			common->FatalError( "Unable to initialize OpenGL" );
		}

		if( i == 0 )
		{
			// same settings, no stereo
			continue;
		}

safeMode:
		// if we failed, set everything back to "safe mode"
		// and try again
		r_vidMode.SetInteger( 0 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_antiAliasing.SetInteger( 0 );
	}
}


/*
=====================
R_ReloadSurface_f

Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const idCmdArgs& args )
{
	modelTrace_t mt;
	idVec3 start, end;

	if( !tr.primaryView )
	{
		return;
	}

	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	if( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		return;
	}

	common->Printf( "Reloading %s\n", mt.material->GetName() );

	// reload the decl
	mt.material->base->Reload();

#if defined( USE_NVRHI )
	nvrhi::CommandListHandle commandList = deviceManager->GetDevice()->createCommandList();

	commandList->open();
#endif

	// reload any images used by the decl
	mt.material->ReloadImages( false, commandList );

#if defined( USE_NVRHI )
	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );
#endif
}

/*
==============
R_ListModes_f
==============
*/
static void R_ListModes_f( const idCmdArgs& args )
{
	for( int displayNum = 0 ; ; displayNum++ )
	{
		idList<vidMode_t> modeList;
		if( !R_GetModeListForDisplay( displayNum, modeList ) )
		{
			break;
		}
		for( int i = 0; i < modeList.Num() ; i++ )
		{
			common->Printf( "Monitor %i, mode %3i: %4i x %4i @ %ihz\n", displayNum + 1, i, modeList[i].width, modeList[i].height, modeList[i].displayHz );
		}
	}
}

/*
=============
R_TestImage_f

Display the given image centered on the screen.
testimage <number>
testimage <filename>
=============
*/
void R_TestImage_f( const idCmdArgs& args )
{
	int imageNum;

	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;

	if( args.Argc() != 2 )
	{
		return;
	}

	if( idStr::IsNumeric( args.Argv( 1 ) ) )
	{
		imageNum = atoi( args.Argv( 1 ) );
		if( imageNum >= 0 && imageNum < globalImages->images.Num() )
		{
			tr.testImage = globalImages->images[imageNum];
		}
	}
	else
	{
		tr.testImage = globalImages->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	}
}

/*
=============
R_TestVideo_f

Plays the cinematic file in a testImage
=============
*/
void R_TestVideo_f( const idCmdArgs& args )
{
	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;

	if( args.Argc() < 2 )
	{
		return;
	}

	tr.testImage = globalImages->ImageFromFile( "_scratch", TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	tr.testVideo = idCinematic::Alloc();
	tr.testVideo->InitFromFile( args.Argv( 1 ), true, NULL );

	cinData_t	cin;

	// FIXME commandList
	cin = tr.testVideo->ImageForTime( 0, NULL );
	// SRS - Also handle ffmpeg and original RoQ decoders for test videos (using cin.image)
	if( cin.imageY == NULL && cin.image == NULL )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
		tr.testImage = NULL;
		return;
	}

	common->Printf( "%i x %i images\n", cin.imageWidth, cin.imageHeight );

	int	len = tr.testVideo->AnimationLength();
	common->Printf( "%5.1f seconds of video\n", len * 0.001 );

	// try to play the matching wav file
	idStr	wavString = args.Argv( ( args.Argc() == 2 ) ? 1 : 2 );
	wavString.StripFileExtension();
	wavString = wavString + ".wav";
	common->SW()->PlayShaderDirectly( wavString.c_str() );
}

static int R_QsortSurfaceAreas( const void* a, const void* b )
{
	const idMaterial*	ea, *eb;
	int	ac, bc;

	ea = *( idMaterial** )a;
	if( !ea->EverReferenced() )
	{
		ac = 0;
	}
	else
	{
		ac = ea->GetSurfaceArea();
	}
	eb = *( idMaterial** )b;
	if( !eb->EverReferenced() )
	{
		bc = 0;
	}
	else
	{
		bc = eb->GetSurfaceArea();
	}

	if( ac < bc )
	{
		return -1;
	}
	if( ac > bc )
	{
		return 1;
	}

	return idStr::Icmp( ea->GetName(), eb->GetName() );
}


/*
===================
R_ReportSurfaceAreas_f

Prints a list of the materials sorted by surface area
===================
*/
#pragma warning( disable: 6385 ) // This is simply to get pass a false defect for /analyze -- if you can figure out a better way, please let Shawn know...
void R_ReportSurfaceAreas_f( const idCmdArgs& args )
{
	unsigned int		i;
	idMaterial**	list;

	const unsigned int count = declManager->GetNumDecls( DECL_MATERIAL );
	if( count == 0 )
	{
		return;
	}

	list = ( idMaterial** )_alloca( count * sizeof( *list ) );

	for( i = 0 ; i < count ; i++ )
	{
		list[i] = ( idMaterial* )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}

	qsort( list, count, sizeof( list[0] ), R_QsortSurfaceAreas );

	// skip over ones with 0 area
	for( i = 0 ; i < count ; i++ )
	{
		if( list[i]->GetSurfaceArea() > 0 )
		{
			break;
		}
	}

	for( ; i < count ; i++ )
	{
		// report size in "editor blocks"
		int	blocks = list[i]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[i]->GetName() );
	}
}
#pragma warning( default: 6385 )


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

bool R_ReadPixelsRGB8( nvrhi::IDevice* device, CommonRenderPasses* pPasses, nvrhi::ITexture* texture, nvrhi::ResourceStates textureState, const char* fullname )
{
	nvrhi::TextureDesc desc = texture->getDesc();
	nvrhi::TextureHandle tempTexture;
	nvrhi::FramebufferHandle tempFramebuffer;

	nvrhi::CommandListHandle commandList = device->createCommandList();
	commandList->open();

	if( textureState != nvrhi::ResourceStates::Unknown )
	{
		commandList->beginTrackingTextureState( texture, nvrhi::TextureSubresourceSet( 0, 1, 0, 1 ), textureState );
	}

	switch( desc.format )
	{
		case nvrhi::Format::RGBA8_UNORM:
		case nvrhi::Format::SRGBA8_UNORM:
			tempTexture = texture;
			break;
		default:
			desc.format = nvrhi::Format::SRGBA8_UNORM;
			desc.isRenderTarget = true;
			desc.initialState = nvrhi::ResourceStates::RenderTarget;
			desc.keepInitialState = true;

			tempTexture = device->createTexture( desc );
			tempFramebuffer = device->createFramebuffer( nvrhi::FramebufferDesc().addColorAttachment( tempTexture ) );

			pPasses->BlitTexture( commandList, tempFramebuffer, texture );
	}

	nvrhi::StagingTextureHandle stagingTexture = device->createStagingTexture( desc, nvrhi::CpuAccessMode::Read );
	commandList->copyTexture( stagingTexture, nvrhi::TextureSlice(), tempTexture, nvrhi::TextureSlice() );

	if( textureState != nvrhi::ResourceStates::Unknown )
	{
		commandList->setTextureState( texture, nvrhi::TextureSubresourceSet( 0, 1, 0, 1 ), textureState );
		commandList->commitBarriers();
	}

	commandList->close();
	device->executeCommandList( commandList );

	size_t rowPitch = 0;
	void* pData = device->mapStagingTexture( stagingTexture, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch );

	if( !pData )
	{
		return false;
	}

	uint32_t* newData = nullptr;

	if( rowPitch != desc.width * 4 )
	{
		newData = new uint32_t[desc.width * desc.height];

		for( uint32_t row = 0; row < desc.height; row++ )
		{
			memcpy( newData + row * desc.width, static_cast<char*>( pData ) + row * rowPitch, desc.width * sizeof( uint32_t ) );
		}

		pData = newData;
	}

	byte* data = static_cast<byte*>( pData );

#if 0
	// fill with red for debugging
	for( int i = 0; i < ( desc.width * desc.height ); i++ )
	{
		data[ i * 4 + 0 ] = 255;
		data[ i * 4 + 1 ] = 0;
		data[ i * 4 + 2 ] = 0;
	}
#endif

	// fix alpha
	for( int i = 0; i < ( desc.width * desc.height ); i++ )
	{
		data[ i * 4 + 3 ] = 0xff;
	}

	R_WritePNG( fullname, static_cast<byte*>( pData ), 4, desc.width, desc.height, true, "fs_basepath" );

	if( newData )
	{
		delete[] newData;
		newData = nullptr;
	}

	device->unmapStagingTexture( stagingTexture );

	return true;
}

bool R_ReadPixelsRGB16F( nvrhi::IDevice* device, CommonRenderPasses* pPasses, nvrhi::ITexture* texture, nvrhi::ResourceStates textureState, byte** pic, int picWidth, int picHeight )
{
	nvrhi::TextureDesc desc = texture->getDesc();
	nvrhi::TextureHandle tempTexture;
	nvrhi::FramebufferHandle tempFramebuffer;

#if 0
	if( desc.width != picWidth || desc.height != picHeight )
	{
		return false;
	}
#endif

	nvrhi::CommandListHandle commandList = device->createCommandList();
	commandList->open();

	if( textureState != nvrhi::ResourceStates::Unknown )
	{
		commandList->beginTrackingTextureState( texture, nvrhi::TextureSubresourceSet( 0, 1, 0, 1 ), textureState );
	}

	switch( desc.format )
	{
		case nvrhi::Format::RGBA16_FLOAT:
			tempTexture = texture;
			break;
		default:
			desc.format = nvrhi::Format::RGBA16_FLOAT;
			desc.isRenderTarget = true;
			desc.initialState = nvrhi::ResourceStates::RenderTarget;
			desc.keepInitialState = true;

			tempTexture = device->createTexture( desc );
			tempFramebuffer = device->createFramebuffer( nvrhi::FramebufferDesc().addColorAttachment( tempTexture ) );

			pPasses->BlitTexture( commandList, tempFramebuffer, texture );
	}

	nvrhi::StagingTextureHandle stagingTexture = device->createStagingTexture( desc, nvrhi::CpuAccessMode::Read );
	commandList->copyTexture( stagingTexture, nvrhi::TextureSlice(), tempTexture, nvrhi::TextureSlice() );

	if( textureState != nvrhi::ResourceStates::Unknown )
	{
		commandList->setTextureState( texture, nvrhi::TextureSubresourceSet( 0, 1, 0, 1 ), textureState );
		commandList->commitBarriers();
	}

	commandList->close();
	device->executeCommandList( commandList );

	size_t rowPitch = 0;
	void* pData = device->mapStagingTexture( stagingTexture, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch );

	if( !pData )
	{
		return false;
	}

	uint16_t* newData = nullptr;

	if( rowPitch != desc.width * 8 )
	{
		newData = new uint16_t[desc.width * desc.height * 2];

		for( uint32_t row = 0; row < desc.height; row++ )
		{
			memcpy( newData + row * desc.width, static_cast<char*>( pData ) + row * rowPitch, desc.width * sizeof( uint16_t ) * 4 );
		}

		pData = newData;
	}

	int pix = picWidth * picHeight;
	const int bufferSize = pix * 3 * 2;

	void* floatRGB16F = R_StaticAlloc( bufferSize );
	*pic = ( byte* ) floatRGB16F;

	// copy from RGBA16F to RGB16F
	uint16_t* data = static_cast<uint16_t*>( pData );
	uint16_t* outData = static_cast<uint16_t*>( floatRGB16F );

#if 0
	for( int i = 0; i < ( desc.width * desc.height ); i++ )
	{
		outData[ i * 3 + 0 ] = F32toF16( 1 );
		outData[ i * 3 + 1 ] = F32toF16( 0 );
		outData[ i * 3 + 2 ] = F32toF16( 0 );
	}
#endif

	for( int i = 0; i < ( desc.width * desc.height ); i++ )
	{
		outData[ i * 3 + 0 ] = data[ i * 4 + 0 ];
		outData[ i * 3 + 1 ] = data[ i * 4 + 1 ];
		outData[ i * 3 + 2 ] = data[ i * 4 + 2 ];
	}

	// RB: filter out garbage and reset it to black
	// this is a rare case but with a high visual impact
	bool isCorrupted = false;

	const idVec3 LUMINANCE_LINEAR( 0.299f, 0.587f, 0.144f );
	idVec3 rgb;

	for( int i = 0; i < ( desc.width * desc.height ); i++ )
	{
		rgb.x = F16toF32( outData[ i * 3 + 0 ] );
		rgb.y = F16toF32( outData[ i * 3 + 1 ] );
		rgb.z = F16toF32( outData[ i * 3 + 2 ] );

		if( IsNAN( rgb.x ) || IsNAN( rgb.y ) || IsNAN( rgb.z ) )
		{
			isCorrupted = true;
			break;
		}

		// captures within the Doom 3 main campaign usually have a luminance of ~ 0.5 - 4.0
		// the threshold is a bit higher and might need to be adapted for total conversion content
		float luminance = rgb * LUMINANCE_LINEAR;
		if( luminance > 20.0f )
		{
			isCorrupted = true;
			break;
		}
	}

	if( isCorrupted )
	{
		for( int i = 0; i < ( desc.width * desc.height ); i++ )
		{
			outData[ i * 3 + 0 ] = F32toF16( 0 );
			outData[ i * 3 + 1 ] = F32toF16( 0 );
			outData[ i * 3 + 2 ] = F32toF16( 0 );
		}
	}

	if( newData )
	{
		delete[] newData;
		newData = nullptr;
	}

	device->unmapStagingTexture( stagingTexture );

	return ( !isCorrupted );
}

/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

If ref == NULL, common->UpdateScreen will be used
==================
*/
void idRenderSystemLocal::TakeScreenshot( int widthIgnored, int heightIgnored, const char* fileName, renderView_t* ref )
{
	takingScreenshot = true;

	// make sure the game / draw thread has completed
	commonLocal.WaitGameThread();

	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );
	if( ref )
	{
		// ref is only used by envShot, Event_camShot, etc to grab screenshots of things in the world,
		// so this omits the hud and other effects
		tr.primaryWorld->RenderScene( ref );
	}
	else
	{
		// build all the draw commands without running a new game tic
		commonLocal.Draw();
	}
	// this should exit right after vsync, with the GPU idle and ready to draw
	const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

	// get the GPU busy with new commands
	tr.RenderCommandBuffers( cmd );

	// discard anything currently on the list (this triggers SwapBuffers)
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

	R_ReadPixelsRGB8( deviceManager->GetDevice(), &tr.backend.GetCommonPasses(), globalImages->ldrImage->GetTextureHandle() , nvrhi::ResourceStates::RenderTarget, fileName );

	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

	takingScreenshot = false;
}

// RB: TODO FINISH or REMOVE
byte* idRenderSystemLocal::CaptureRenderToBuffer( int width, int height, renderView_t* ref )
{
	byte*		buffer;

	takingScreenshot = true;

	int pix = width * height;
	//const int bufferSize = pix * 3 * 2;

	// HDR only for now
	//if( exten == EXR )
	{
		buffer = ( byte* )R_StaticAlloc( pix * 3 * 2 );
	}
	//else if( exten == PNG )
	//{
	//	buffer = ( byte* )R_StaticAlloc( pix * 3 );
	//}

	//R_ReadTiledPixels( width, height, buffer, ref );

	takingScreenshot = false;

	return buffer;
}

/*
==================
R_ScreenshotFilename

Returns a filename with digits appended
if we have saved a previous screenshot, don't scan
from the beginning, because recording demo avis can involve
thousands of shots
==================
*/
void R_ScreenshotFilename( int& lastNumber, const char* base, idStr& fileName )
{
	bool restrict = cvarSystem->GetCVarBool( "fs_restrict" );
	cvarSystem->SetCVarBool( "fs_restrict", false );

	lastNumber++;
	if( lastNumber > 99999 )
	{
		lastNumber = 99999;
	}
	for( ; lastNumber < 99999 ; lastNumber++ )
	{

		// RB: added date to screenshot name
#if 0
		int	frac = lastNumber;
		int	a, b, c, d, e;

		a = frac / 10000;
		frac -= a * 10000;
		b = frac / 1000;
		frac -= b * 1000;
		c = frac / 100;
		frac -= c * 100;
		d = frac / 10;
		frac -= d * 10;
		e = frac;

		sprintf( fileName, "%s%i%i%i%i%i.png", base, a, b, c, d, e );
#else
		time_t aclock;
		time( &aclock );
		struct tm* t = localtime( &aclock );

		sprintf( fileName, "%s%s-%04d%02d%02d-%02d%02d%02d-%03d.png", base, "rbdoom-3-bfg",
				 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, lastNumber );
#endif
		// RB end
		if( lastNumber == 99999 )
		{
			break;
		}
		int len = fileSystem->ReadFile( fileName, NULL, NULL );
		if( len <= 0 )
		{
			break;
		}
		// check again...
	}
	cvarSystem->SetCVarBool( "fs_restrict", restrict );
}

/*
==================
R_BlendedScreenShot

screenshot
screenshot [filename]
screenshot [width] [height]
==================
*/
void R_ScreenShot_f( const idCmdArgs& args )
{
	static int lastNumber = 0;
	idStr checkname;

	int width = renderSystem->GetWidth();
	int height = renderSystem->GetHeight();
	int	blends = 0;

	switch( args.Argc() )
	{
		case 1:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;

		case 2:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			checkname = args.Argv( 1 );
			break;

		case 3:
			width = atoi( args.Argv( 1 ) );
			height = atoi( args.Argv( 2 ) );
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;

		default:
			common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>" );
			return;
	}

	// put the console away
	console->Close();

	tr.TakeScreenshot( width, height, checkname, NULL );

	common->Printf( "Wrote %s\n", checkname.c_str() );
}



/*
==================
R_EnvShot_f

envshot <basename>

Saves out env/<basename>_ft.tga, etc

RB: This is outdated and probably a relict from Rage. It could be updated to dump panorama images for tools like Blender or Substance Painter
==================
*/
void R_EnvShot_f( const idCmdArgs& args )
{
	idStr		fullname;
	const char*	baseName;
	int			i;
	idMat3		axis[6], oldAxis;
	renderView_t	ref;
	viewDef_t	primary;
	int			blends;
	const char*  extension;
	int			size;
	int         res_w, res_h, old_fov_x, old_fov_y;

	res_w = renderSystem->GetWidth();
	res_h = renderSystem->GetHeight();

	if( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 )
	{
		common->Printf( "USAGE: envshot <basename> [size] [blends]\n" );
		return;
	}
	baseName = args.Argv( 1 );

	blends = 1;
	if( args.Argc() == 4 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	}
	else if( args.Argc() == 3 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	}
	else
	{
		size = 256;
		blends = 1;
	}

	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}

	primary = *tr.primaryView;

	memset( &axis, 0, sizeof( axis ) );

	// +X
	axis[0][0][0] = 1;
	axis[0][1][2] = 1;
	axis[0][2][1] = 1;

	// -X
	axis[1][0][0] = -1;
	axis[1][1][2] = -1;
	axis[1][2][1] = 1;

	// +Y
	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = -1;

	// -Y
	axis[3][0][1] = -1;
	axis[3][1][0] = -1;
	axis[3][2][2] = 1;

	// +Z
	axis[4][0][2] = 1;
	axis[4][1][0] = -1;
	axis[4][2][1] = 1;

	// -Z
	axis[5][0][2] = -1;
	axis[5][1][0] = 1;
	axis[5][2][1] = 1;

	// let's get the game window to a "size" resolution
	if( ( res_w != size ) || ( res_h != size ) )
	{
		cvarSystem->SetCVarInteger( "r_windowWidth", size );
		cvarSystem->SetCVarInteger( "r_windowHeight", size );
		R_SetNewMode( false ); // the same as "vid_restart"
	} // FIXME that's a hack!!

	// so we return to that axis and fov after the fact.
	oldAxis = primary.renderView.viewaxis;
	old_fov_x = primary.renderView.fov_x;
	old_fov_y = primary.renderView.fov_y;

	for( i = 0 ; i < 6 ; i++ )
	{
		ref = primary.renderView;

		extension = envDirection[ i ];

		ref.fov_x = ref.fov_y = 90;
		ref.viewaxis = axis[i];
		fullname.Format( "env/%s%s", baseName, extension );

		tr.TakeScreenshot( size, size, fullname, &ref );
	}

	// restore the original resolution, axis and fov
	ref.viewaxis = oldAxis;
	ref.fov_x = old_fov_x;
	ref.fov_y = old_fov_y;
	cvarSystem->SetCVarInteger( "r_windowWidth", res_w );
	cvarSystem->SetCVarInteger( "r_windowHeight", res_h );
	R_SetNewMode( false ); // the same as "vid_restart"

	common->Printf( "Wrote a env set with the name %s\n", baseName );
}

//============================================================================

void R_TransformCubemap( const char* orgDirection[6], const char* orgDir, const char* destDirection[6], const char* destDir, const char* baseName )
{
	idStr fullname;
	int			i;
	bool        errorInOriginalImages = false;
	byte*		buffers[6];
	int			width = 0, height = 0;

	for( i = 0 ; i < 6 ; i++ )
	{
		// read every image images
		fullname.Format( "%s/%s%s.tga", orgDir, baseName, orgDirection[i] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[i], &width, &height, NULL, true, NULL );

		//check if the buffer is troublesome
		if( !buffers[i] )
		{
			common->Printf( "failed.\n" );
			errorInOriginalImages = true;
		}
		else if( width != height )
		{
			common->Printf( "wrong size pal!\n\n\nget your shit together and set the size according to your images!\n\n\ninept programmers are inept!\n" );
			errorInOriginalImages = true; // yeah, but don't just choke on a joke!
		}
		else
		{
			errorInOriginalImages = false;
		}

		if( errorInOriginalImages )
		{
			errorInOriginalImages = false;
			for( i-- ; i >= 0 ; i-- )
			{
				Mem_Free( buffers[i] ); // clean up every buffer from this stage down
			}

			return;
		}

		// apply rotations and flips
		R_ApplyCubeMapTransforms( i, buffers[i], width );

		//save the images with the appropiate skybox naming convention
		fullname.Format( "%s/%s/%s%s.tga", destDir, baseName, baseName, destDirection[i] );
		common->Printf( "writing %s\n", fullname.c_str() );
		common->UpdateScreen( false );
		R_WriteTGA( fullname, buffers[i], width, width, false, "fs_basepath" );
	}

	for( i = 0 ; i < 6 ; i++ )
	{
		if( buffers[i] )
		{
			Mem_Free( buffers[i] );
		}
	}
}

/*
==================
R_TransformEnvToSkybox_f

R_TransformEnvToSkybox_f <basename>

transforms env textures (of the type px, py, pz, nx, ny, nz)
to skybox textures ( forward, back, left, right, up, down)
==================
*/
void R_TransformEnvToSkybox_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: envToSky <basename>\n" );
		return;
	}

	R_TransformCubemap( envDirection, "env", skyDirection, "skybox", args.Argv( 1 ) );
}

/*
==================
R_TransformSkyboxToEnv_f

R_TransformSkyboxToEnv_f <basename>

transforms skybox textures ( forward, back, left, right, up, down)
to env textures (of the type px, py, pz, nx, ny, nz)
==================
*/

void R_TransformSkyboxToEnv_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: skyToEnv <basename>\n" );
		return;
	}

	R_TransformCubemap( skyDirection, "skybox", envDirection, "env", args.Argv( 1 ) );
}

//============================================================================


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings()
{
	float b = r_brightness.GetFloat();
	float invg = 1.0f / r_gamma.GetFloat();

	float j = 0.0f;
	for( int i = 0; i < 256; i++, j += b )
	{
		int inf = idMath::Ftoi( 0xffff * pow( j / 255.0f, invg ) + 0.5f );
		tr.gammaTable[i] = idMath::ClampInt( 0, 0xFFFF, inf );
	}
// SRS - Generalized Vulkan SDL platform
#if defined( VULKAN_USE_PLATFORM_SDL )
	VKimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
#else
	GLimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
#endif
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( const idCmdArgs& args )
{
	common->Printf( "CPU: %s\n", Sys_GetProcessorString() );

	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );
	common->Printf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
#if !defined( USE_NVRHI )
	if( glConfig.wgl_extensions_string )
	{
		common->Printf( "WGL_EXTENSIONS: %s\n", glConfig.wgl_extensions_string );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_TEXTURE_COORDS_ARB: %d\n", glConfig.maxTextureCoords );
	common->Printf( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", glConfig.maxTextureImageUnits );
#endif

	// print all the display adapters, monitors, and video modes
	//void DumpAllDisplayDevices();
	//DumpAllDisplayDevices();

	//common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_vidMode.GetInteger(), renderSystem->GetWidth(), renderSystem->GetHeight(), fsstrings[r_fullscreen.GetBool()] );
	if( glConfig.displayFrequency )
	{
		common->Printf( "%d\n", glConfig.displayFrequency );
	}
	else
	{
		common->Printf( "N/A\n" );
	}

	common->Printf( "-------\n" );

	// RB begin
#if defined(_WIN32) && !defined(USE_VULKAN) && !defined(USE_NVRHI)
	// WGL_EXT_swap_interval
	if( r_swapInterval.GetInteger() && wglSwapIntervalEXT != NULL )
	{
		common->Printf( "Forcing swapInterval %i\n", r_swapInterval.GetInteger() );
	}
	else
	{
		common->Printf( "swapInterval not forced\n" );
	}
#endif
	// RB end

	if( glConfig.stereoPixelFormatAvailable && glConfig.isStereoPixelFormat )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format active\n" );
	}
	else if( glConfig.stereoPixelFormatAvailable )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel available but not selected\n" );
	}
	else
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format not available\n" );
	}

	idLib::Printf( "Stereo mode: " );
	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_OFF:
			idLib::Printf( "STEREO3D_OFF\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE_COMPRESSED\n" );
			break;
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			idLib::Printf( "STEREO3D_TOP_AND_BOTTOM_COMPRESSED\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE\n" );
			break;
		case STEREO3D_HDMI_720:
			idLib::Printf( "STEREO3D_HDMI_720\n" );
			break;
		case STEREO3D_INTERLACED:
			idLib::Printf( "STEREO3D_INTERLACED\n" );
			break;
		case STEREO3D_QUAD_BUFFER:
			idLib::Printf( "STEREO3D_QUAD_BUFFER\n" );
			break;
		default:
			idLib::Printf( "Unknown (%i)\n", renderSystem->GetStereo3DMode() );
			break;
	}

	idLib::Printf( "%i multisamples\n", glConfig.multisamples );

	common->Printf( "%5.1f cm screen width (%4.1f\" diagonal)\n",
					glConfig.physicalScreenWidthInCentimeters, glConfig.physicalScreenWidthInCentimeters / 2.54f
					* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	extern idCVar r_forceScreenWidthCentimeters;
	if( r_forceScreenWidthCentimeters.GetFloat() )
	{
		common->Printf( "screen size manually forced to %5.1f cm width (%4.1f\" diagonal)\n",
						renderSystem->GetPhysicalScreenWidthInCentimeters(), renderSystem->GetPhysicalScreenWidthInCentimeters() / 2.54f
						* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	}

	if( glConfig.gpuSkinningAvailable )
	{
		common->Printf( S_COLOR_GREEN "GPU skeletal animation available\n" );
	}
	else
	{
		common->Printf( S_COLOR_RED "GPU skeletal animation not available (slower CPU path active)\n" );
	}
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const idCmdArgs& args )
{
	// if OpenGL isn't started, do nothing
	if( !tr.IsInitialized() )
	{
		return;
	}

	// set the mode without re-initializing the context
	R_SetNewMode( false );
}

/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials()
{
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if( !tr.defaultMaterial )
	{
		common->FatalError( "_default material not found" );
	}
	tr.defaultPointLight = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultProjectedLight = declManager->FindMaterial( "lights/defaultProjectedLight" );
	tr.whiteMaterial = declManager->FindMaterial( "_white", false );
	tr.charSetMaterial = declManager->FindMaterial( "textures/bigchars" );

	// RB: create implicit material
	tr.imgGuiMaterial = declManager->FindMaterial( "_imguiFont", true );

#if IMGUI_BFGUI
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->TexID = ( void* )( intptr_t )tr.imgGuiMaterial;
#endif
}


/*
=================
R_SizeUp_f

Keybinding command
=================
*/
static void R_SizeUp_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() + 10 > 100 )
	{
		r_screenFraction.SetInteger( 100 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}


/*
=================
R_SizeDown_f

Keybinding command
=================
*/
static void R_SizeDown_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() - 10 < 10 )
	{
		r_screenFraction.SetInteger( 10 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}


/*
===============
TouchGui_f

  this is called from the main thread
===============
*/
void R_TouchGui_f( const idCmdArgs& args )
{
	const char*	gui = args.Argv( 1 );

	if( !gui[0] )
	{
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}

	common->Printf( "touchGui %s\n", gui );
	const bool captureToImage = false;
	common->UpdateScreen( captureToImage );
	uiManager->Touch( gui );
}



/*
=================
R_InitCommands
=================
*/
void R_InitCommands()
{
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );
	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );
	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "envToSky", R_TransformEnvToSkybox_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms environment textures to sky box textures" );
	cmdSystem->AddCommand( "skyToEnv", R_TransformSkyboxToEnv_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms sky box textures to environment textures" );
	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", idCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", idCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "listModes", R_ListModes_f, CMD_FL_RENDERER, "lists all video modes" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear()
{
	registered = false;
	frameCount = 0;
	viewCount = 0;
	frameShaderTime = 0.0f;
	ambientLightVector.Zero();
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	ambientCubeImage = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	currentColorNativeBytesOrder = 0xFFFFFFFF;
	currentGLState = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	memset( &cubeAxis, 0, sizeof( cubeAxis ) ); // RB
	takingScreenshot = false;
	takingEnvprobe = false;

	if( unitSquareTriangles != NULL )
	{
		Mem_Free( unitSquareTriangles );
		unitSquareTriangles = NULL;
	}

	if( zeroOneCubeTriangles != NULL )
	{
		Mem_Free( zeroOneCubeTriangles );
		zeroOneCubeTriangles = NULL;
	}

	if( zeroOneSphereTriangles != NULL )
	{
		Mem_Free( zeroOneSphereTriangles );
		zeroOneSphereTriangles = NULL;
	}

	if( testImageTriangles != NULL )
	{
		Mem_Free( testImageTriangles );
		testImageTriangles = NULL;
	}

	frontEndJobList = NULL;

	// RB
	envprobeJobList = NULL;
	envprobeJobs.Clear();
	lightGridJobs.Clear();
}

/*
=============
R_MakeFullScreenTris
=============
*/
static srfTriangles_t* R_MakeFullScreenTris()
{
	// copy verts and indexes
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	idDrawVert* verts = tri->verts;

	triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].SetTexCoord( 0.0f, 1.0f );

	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].SetTexCoord( 1.0f, 1.0f );

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].SetTexCoord( 1.0f, 0.0f );

	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].SetTexCoord( 0.0f, 0.0f );

	for( int i = 0 ; i < 4 ; i++ )
	{
		verts[i].SetColor( 0xffffffff );
	}


	return tri;
}

/*
=============
R_MakeZeroOneCubeTris
=============
*/
static srfTriangles_t* R_MakeZeroOneCubeTris()
{
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numVerts = 8;
	tri->numIndexes = 36;

	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	idDrawVert* verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	idVec3 center( 0.0f );
	idVec3 mx( low, 0.0f, 0.0f );
	idVec3 px( high, 0.0f, 0.0f );
	idVec3 my( 0.0f,  low, 0.0f );
	idVec3 py( 0.0f, high, 0.0f );
	idVec3 mz( 0.0f, 0.0f,  low );
	idVec3 pz( 0.0f, 0.0f, high );

	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;

	// bottom
	tri->indexes[ 0 * 3 + 0] = 2;
	tri->indexes[ 0 * 3 + 1] = 3;
	tri->indexes[ 0 * 3 + 2] = 0;
	tri->indexes[ 1 * 3 + 0] = 1;
	tri->indexes[ 1 * 3 + 1] = 2;
	tri->indexes[ 1 * 3 + 2] = 0;
	// back
	tri->indexes[ 2 * 3 + 0] = 5;
	tri->indexes[ 2 * 3 + 1] = 1;
	tri->indexes[ 2 * 3 + 2] = 0;
	tri->indexes[ 3 * 3 + 0] = 4;
	tri->indexes[ 3 * 3 + 1] = 5;
	tri->indexes[ 3 * 3 + 2] = 0;
	// left
	tri->indexes[ 4 * 3 + 0] = 7;
	tri->indexes[ 4 * 3 + 1] = 4;
	tri->indexes[ 4 * 3 + 2] = 0;
	tri->indexes[ 5 * 3 + 0] = 3;
	tri->indexes[ 5 * 3 + 1] = 7;
	tri->indexes[ 5 * 3 + 2] = 0;
	// right
	tri->indexes[ 6 * 3 + 0] = 1;
	tri->indexes[ 6 * 3 + 1] = 5;
	tri->indexes[ 6 * 3 + 2] = 6;
	tri->indexes[ 7 * 3 + 0] = 2;
	tri->indexes[ 7 * 3 + 1] = 1;
	tri->indexes[ 7 * 3 + 2] = 6;
	// front
	tri->indexes[ 8 * 3 + 0] = 3;
	tri->indexes[ 8 * 3 + 1] = 2;
	tri->indexes[ 8 * 3 + 2] = 6;
	tri->indexes[ 9 * 3 + 0] = 7;
	tri->indexes[ 9 * 3 + 1] = 3;
	tri->indexes[ 9 * 3 + 2] = 6;
	// top
	tri->indexes[10 * 3 + 0] = 4;
	tri->indexes[10 * 3 + 1] = 7;
	tri->indexes[10 * 3 + 2] = 6;
	tri->indexes[11 * 3 + 0] = 5;
	tri->indexes[11 * 3 + 1] = 4;
	tri->indexes[11 * 3 + 2] = 6;

	for( int i = 0 ; i < 4 ; i++ )
	{
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

// RB begin
static srfTriangles_t* R_MakeZeroOneSphereTris()
{
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	const float radius = 1.0f;
	const int rings = 20.0f;
	const int sectors = 20.0f;

	tri->numVerts = ( rings * sectors );
	tri->numIndexes = ( ( rings - 1 ) * sectors ) * 6;

	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	idDrawVert* verts = tri->verts;

	float const R = 1.0f / ( float )( rings - 1 );
	float const S = 1.0f / ( float )( sectors - 1 );

	int numTris = 0;
	int numVerts = 0;
	for( int r = 0; r < rings; ++r )
	{
		for( int s = 0; s < sectors; ++s )
		{
			const float y = sin( -idMath::HALF_PI +  idMath::PI * r * R );
			const float x = cos( 2 * idMath::PI * s * S ) * sin( idMath::PI * r * R );
			const float z = sin( 2 * idMath::PI * s * S ) * sin( idMath::PI * r * R );

			verts[ numVerts ].SetTexCoord( s * S, r * R );
			verts[ numVerts ].xyz = idVec3( x, y, z ) * radius;
			verts[ numVerts ].SetNormal( x, y, z );
			verts[ numVerts ].SetColor( 0xffffffff );
			numVerts++;

			if( r < ( rings - 1 ) )
			{
				int curRow = r * sectors;
				int nextRow = ( r + 1 ) * sectors;
				int nextS = ( s + 1 ) % sectors;

				tri->indexes[( numTris * 3 ) + 2] = ( curRow + s );
				tri->indexes[( numTris * 3 ) + 1] = ( nextRow + s );
				tri->indexes[( numTris * 3 ) + 0] = ( nextRow + nextS );

				numTris += 1;

				tri->indexes[( numTris * 3 ) + 2] = ( curRow + s );
				tri->indexes[( numTris * 3 ) + 1] = ( nextRow + nextS );
				tri->indexes[( numTris * 3 ) + 0] = ( curRow + nextS );

				numTris += 1;
			}
		}
	}

	return tri;
}
// RB end

/*
================
R_MakeTestImageTriangles

Initializes the Test Image Triangles
================
*/
srfTriangles_t* R_MakeTestImageTriangles()
{
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	ALIGNTYPE16 triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	idDrawVert* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].SetTexCoord( 0.0, 0.0f );

	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].SetTexCoord( 1.0f, 0.0f );

	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].SetTexCoord( 1.0f, 1.0f );

	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].SetTexCoord( 0.0f, 1.0f );

	for( int i = 0; i < 4; i++ )
	{
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init()
{
	common->Printf( "------- Initializing renderSystem --------\n" );

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	R_InitCommands();

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	guiModel = new( TAG_RENDER ) idGuiModel;
	guiModel->Clear();
	tr_guiModel = guiModel;	// for DeviceContext fast path

	UpdateStereo3DMode();

	globalImages->Init();

	// RB begin
	Framebuffer::Init();
	// RB end

	idCinematic::InitCinematic();

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterials();

	renderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix[0 * 4 + 0] = 1.0f;
	identitySpace.modelMatrix[1 * 4 + 1] = 1.0f;
	identitySpace.modelMatrix[2 * 4 + 2] = 1.0f;

	// set cubemap axis for cubemap sampling tools

	// +X
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = 1;
	cubeAxis[0][2][1] = 1;

	// -X
	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = -1;
	cubeAxis[1][2][1] = 1;

	// +Y
	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = -1;
	cubeAxis[2][2][2] = -1;

	// -Y
	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = -1;
	cubeAxis[3][2][2] = 1;

	// +Z
	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = -1;
	cubeAxis[4][2][1] = 1;

	// -Z
	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = 1;
	cubeAxis[5][2][1] = 1;

	// make sure the tr.unitSquareTriangles data is current in the vertex / index cache
	if( unitSquareTriangles == NULL )
	{
		unitSquareTriangles = R_MakeFullScreenTris();
	}

	// make sure the tr.zeroOneCubeTriangles data is current in the vertex / index cache
	if( zeroOneCubeTriangles == NULL )
	{
		zeroOneCubeTriangles = R_MakeZeroOneCubeTris();
		R_DeriveTangents( zeroOneCubeTriangles ); // RB: we need normals for debugging reflections
	}

	// RB make sure the tr.zeroOneSphereTriangles data is current in the vertex / index cache
	if( zeroOneSphereTriangles == NULL )
	{
		zeroOneSphereTriangles = R_MakeZeroOneSphereTris();
		//R_DeriveTangents( zeroOneSphereTriangles );
	}

	// make sure the tr.testImageTriangles data is current in the vertex / index cache
	if( testImageTriangles == NULL )
	{
		testImageTriangles = R_MakeTestImageTriangles();
	}

	frontEndJobList = parallelJobManager->AllocJobList( JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL );
	envprobeJobList = parallelJobManager->AllocJobList( JOBLIST_UTILITY, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL ); // RB

	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		// avoid GL_BlockingSwapBuffers
		omitSwapBuffers = true;
	}

	// make sure the command buffers are ready to accept the first screen update
	SwapCommandBuffers( NULL, NULL, NULL, NULL, NULL, NULL );

	common->Printf( "renderSystem initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown()
{
	extern idCVar com_smp;

	common->Printf( "idRenderSystem::Shutdown()\n" );

	fonts.DeleteContents();

	if( IsInitialized() )
	{
		globalImages->PurgeAllImages();
	}

	renderModelManager->Shutdown();

	idCinematic::ShutdownCinematic();

	globalImages->Shutdown();

	// RB begin
	Framebuffer::Shutdown();
	// RB end

	// free frame memory
	R_ShutdownFrameData();

	UnbindBufferObjects();

	// SRS - wait for fence to hit before freeing any resources the GPU may be using, otherwise get Vulkan validation layer errors on shutdown
	// SRS - skip this step if we are in Doom 3 mode (com_smp = -1) which has already finished and presented
	if( com_smp.GetInteger() != -1 )
	{
		backend.GL_BlockingSwapBuffers();
	}

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	RB_ShutdownDebugTools();

	delete guiModel;

	parallelJobManager->FreeJobList( envprobeJobList );
	parallelJobManager->FreeJobList( frontEndJobList );

	Clear();

#if defined( USE_NVRHI )
	commandList.Reset();
#endif

	ShutdownOpenGL();

	bInitialized = false;
}

/*
========================
idRenderSystemLocal::ResetGuiModels
========================
*/
void idRenderSystemLocal::ResetGuiModels()
{
	delete guiModel;
	guiModel = new( TAG_RENDER ) idGuiModel;
	guiModel->Clear();
	guiModel->BeginFrame();
	tr_guiModel = guiModel;	// for DeviceContext fast path
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad()
{
	// clear binding sets for previous level images and light data #676
	backend.ClearCaches();

	globalImages->BeginLevelLoad();
	renderModelManager->BeginLevelLoad();

	// Re-Initialize the Default Materials if needed.
	R_InitMaterials();
}

/*
========================
idRenderSystemLocal::LoadLevelImages
========================
*/
void idRenderSystemLocal::LoadLevelImages()
{
	globalImages->LoadLevelImages( false );

#if defined( USE_NVRHI )
	deviceManager->GetDevice()->waitForIdle();
	deviceManager->GetDevice()->runGarbageCollection();
#endif
}

/*
========================
idRenderSystemLocal::Preload
========================
*/
void idRenderSystemLocal::Preload( const idPreloadManifest& manifest, const char* mapName )
{
	globalImages->Preload( manifest, true );
	uiManager->Preload( mapName );
	renderModelManager->Preload( manifest );
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad()
{
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
}

/*
========================
idRenderSystemLocal::BeginAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon )
{
}

/*
========================
idRenderSystemLocal::EndAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::EndAutomaticBackgroundSwaps()
{
}

/*
========================
idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning
========================
*/
bool idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* icon ) const
{
	return false;
}

/*
============
idRenderSystemLocal::RegisterFont
============
*/
idFont* idRenderSystemLocal::RegisterFont( const char* fontName )
{

	idStrStatic< MAX_OSPATH > baseFontName = fontName;
	baseFontName.Replace( "fonts/", "" );
	for( int i = 0; i < fonts.Num(); i++ )
	{
		if( idStr::Icmp( fonts[i]->GetName(), baseFontName ) == 0 )
		{
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont* newFont = new( TAG_FONT ) idFont( baseFontName );
	fonts.Append( newFont );
	return newFont;
}

/*
========================
idRenderSystemLocal::ResetFonts
========================
*/
void idRenderSystemLocal::ResetFonts()
{
	fonts.DeleteContents( true );
}
/*
========================
idRenderSystemLocal::InitOpenGL
========================
*/
void idRenderSystemLocal::InitBackend()
{
	// if OpenGL isn't started, start it now
	if( !IsInitialized() )
	{
		backend.Init();

#if defined( USE_NVRHI )
		if( !commandList )
		{
			commandList = deviceManager->GetDevice()->createCommandList();
		}

		commandList->open();

		// Reloading images here causes the rendertargets to get deleted. Figure out how to handle this properly on 360
		globalImages->ReloadImages( true, commandList );

		commandList->close();
		deviceManager->GetDevice()->executeCommandList( commandList );
#else
		// Reloading images here causes the rendertargets to get deleted. Figure out how to handle this properly on 360
		//globalImages->ReloadImages( true );

#if !defined(USE_VULKAN)
		int err = glGetError();
		if( err != GL_NO_ERROR )
		{
			common->Printf( "glGetError() = 0x%x\n", err );
		}
#endif

#endif
	}
}

/*
========================
idRenderSystemLocal::ShutdownOpenGL
========================
*/
void idRenderSystemLocal::ShutdownOpenGL()
{
	// free the context and close the window
	R_ShutdownFrameData();

	backend.Shutdown();
}

/*
========================
idRenderSystemLocal::IsOpenGLRunning
========================
*/
bool idRenderSystemLocal::IsOpenGLRunning() const
{
	return IsInitialized();
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen() const
{
	return glConfig.isFullscreen != 0;
}

/*
========================
idRenderSystemLocal::GetWidth
========================
*/
int idRenderSystemLocal::GetWidth() const
{
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE || glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE_COMPRESSED )
	{
		return glConfig.nativeScreenWidth >> 1;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetHeight
========================
*/
int idRenderSystemLocal::GetHeight() const
{
	if( glConfig.stereo3Dmode == STEREO3D_HDMI_720 )
	{
		return 720;
	}
	extern idCVar stereoRender_warp;
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE && stereoRender_warp.GetBool() )
	{
		// for the Rift, render a square aspect view that will be symetric for the optics
		return glConfig.nativeScreenWidth >> 1;
	}
	if( glConfig.stereo3Dmode == STEREO3D_INTERLACED || glConfig.stereo3Dmode == STEREO3D_TOP_AND_BOTTOM_COMPRESSED )
	{
		return glConfig.nativeScreenHeight >> 1;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetVirtualWidth
========================
*/
int idRenderSystemLocal::GetVirtualWidth() const
{
// jmarshall - never strech
	//if( r_useVirtualScreenResolution.GetBool() )
	//{
	//	return SCREEN_WIDTH;
	//}
// jmarshall end
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetVirtualHeight
========================
*/
int idRenderSystemLocal::GetVirtualHeight() const
{
// jmarshall - never strech
	//if( r_useVirtualScreenResolution.GetBool() )
	//{
	//	return SCREEN_HEIGHT;
	//}
// jmarshall end
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetStereo3DMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereo3DMode() const
{
	return glConfig.stereo3Dmode;
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
bool idRenderSystemLocal::IsStereoScopicRenderingSupported() const
{
	return true;
}

/*
========================
idRenderSystemLocal::HasQuadBufferSupport
========================
*/
bool idRenderSystemLocal::HasQuadBufferSupport() const
{
	return glConfig.stereoPixelFormatAvailable;
}

/*
========================
idRenderSystemLocal::UpdateStereo3DMode
========================
*/
void idRenderSystemLocal::UpdateStereo3DMode()
{
	if( glConfig.nativeScreenWidth == 1280 && glConfig.nativeScreenHeight == 1470 )
	{
		glConfig.stereo3Dmode = STEREO3D_HDMI_720;
	}
	else
	{
		glConfig.stereo3Dmode = GetStereoScopicRenderingMode();
	}
}

/*
========================
idRenderSystemLocal::GetStereoScopicRenderingMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereoScopicRenderingMode() const
{
	return ( !IsStereoScopicRenderingSupported() ) ? STEREO3D_OFF : ( stereo3DMode_t )stereoRender_enable.GetInteger();
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
void idRenderSystemLocal::EnableStereoScopicRendering( const stereo3DMode_t mode ) const
{
	stereoRender_enable.SetInteger( mode );
}

/*
========================
idRenderSystemLocal::GetPixelAspect
========================
*/
float idRenderSystemLocal::GetPixelAspect() const
{
	switch( glConfig.stereo3Dmode )
	{
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			return glConfig.pixelAspect * 2.0f;
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
		case STEREO3D_INTERLACED:
			return glConfig.pixelAspect * 0.5f;
		default:
			return glConfig.pixelAspect;
	}
}

/*
========================
idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters

This is used to calculate stereoscopic screen offset for a given interocular distance.
========================
*/
idCVar	r_forceScreenWidthCentimeters( "r_forceScreenWidthCentimeters", "0", CVAR_RENDERER | CVAR_ARCHIVE, "Override screen width returned by hardware" );
float idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters() const
{
	if( r_forceScreenWidthCentimeters.GetFloat() > 0 )
	{
		return r_forceScreenWidthCentimeters.GetFloat();
	}
	return glConfig.physicalScreenWidthInCentimeters;
}
