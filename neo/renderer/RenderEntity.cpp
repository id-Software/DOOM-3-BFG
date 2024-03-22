/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

idRenderEntityLocal::idRenderEntityLocal()
{
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

	world					= NULL;
	index					= 0;
	lastModifiedFrameNum	= 0;
	dynamicModel			= NULL;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= NULL;
	localReferenceBounds	= bounds_zero;
	globalReferenceBounds	= bounds_zero;
	viewCount				= 0;
	viewEntity				= NULL;
	decals					= NULL;
	overlays				= NULL;
	entityRefs				= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
	needsPortalSky			= false;
}

void idRenderEntityLocal::FreeRenderEntity()
{
}

void idRenderEntityLocal::UpdateRenderEntity( const renderEntity_t* re, bool forceUpdate )
{
}

void idRenderEntityLocal::GetRenderEntity( renderEntity_t* re )
{
}

void idRenderEntityLocal::ForceUpdate()
{
}

int idRenderEntityLocal::GetIndex()
{
	return index;
}

void idRenderEntityLocal::ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial* material )
{
}
void idRenderEntityLocal::RemoveDecals()
{
}

//======================================================================

idRenderLightLocal::idRenderLightLocal()
{
	memset( &parms, 0, sizeof( parms ) );
	memset( lightProject, 0, sizeof( lightProject ) );

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;

	baseLightProject.Zero();
	inverseBaseLightProject.Zero();
}

void idRenderLightLocal::FreeRenderLight()
{
}
void idRenderLightLocal::UpdateRenderLight( const renderLight_t* re, bool forceUpdate )
{
}
void idRenderLightLocal::GetRenderLight( renderLight_t* re )
{
}
void idRenderLightLocal::ForceUpdate()
{
}
int idRenderLightLocal::GetIndex()
{
	return index;
}

//======================================================================

RenderEnvprobeLocal::RenderEnvprobeLocal()
{
	memset( &parms, 0, sizeof( parms ) );

	envprobeHasMoved		= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	references				= NULL;
}

void RenderEnvprobeLocal::FreeRenderEnvprobe()
{
}
void RenderEnvprobeLocal::UpdateRenderEnvprobe( const renderEnvironmentProbe_t* ep, bool forceUpdate )
{
}
void RenderEnvprobeLocal::GetRenderEnvprobe( renderEnvironmentProbe_t* ep )
{
}
void RenderEnvprobeLocal::ForceUpdate()
{
}
int RenderEnvprobeLocal::GetIndex()
{
	return index;
}

