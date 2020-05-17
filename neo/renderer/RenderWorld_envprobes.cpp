/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020 Robert Beckebans

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

/*
=============
R_SetEnvprobeDefViewEnvprobe

If the envprobeDef is not already on the viewEnvprobe list, create
a viewEnvprobe and add it to the list with an empty scissor rect.
=============
*/
viewEnvprobe_t* R_SetEnvprobeDefViewEnvprobe( RenderEnvprobeLocal* probe )
{
	if( probe->viewCount == tr.viewCount )
	{
		// already set up for this frame
		return probe->viewEnvprobe;
	}
	probe->viewCount = tr.viewCount;

	// add to the view light chain
	viewEnvprobe_t* vProbe = ( viewEnvprobe_t* )R_ClearedFrameAlloc( sizeof( *vProbe ), FRAME_ALLOC_VIEW_LIGHT );
	vProbe->envprobeDef = probe;

	// the scissorRect will be expanded as the envprobe bounds is accepted into visible portal chains
	// and the scissor will be reduced in R_AddSingleEnvprobe based on the screen space projection
	vProbe->scissorRect.Clear();

	// copy data used by backend
	// RB: this would normaly go into R_AddSingleEnvprobe
	vProbe->globalOrigin = probe->parms.origin;
	vProbe->inverseBaseLightProject = probe->inverseBaseLightProject;

	// link the view light
	vProbe->next = tr.viewDef->viewEnvprobes;
	tr.viewDef->viewEnvprobes = vProbe;

	probe->viewEnvprobe = vProbe;

	return vProbe;
}


/*
================
CullEnvprobeByPortals

Return true if the light frustum does not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullEnvprobeByPortals( const RenderEnvprobeLocal* probe, const portalStack_t* ps )
{
	if( r_useLightPortalCulling.GetInteger() == 1 )
	{
		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, probe->inverseBaseLightProject, bounds_zeroOneCube );
		for( int i = 0; i < ps->numPortalPlanes; i++ )
		{
			if( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT )
			{
				return true;
			}
		}

	}
	else if( r_useLightPortalCulling.GetInteger() >= 2 )
	{

		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, probe->baseLightProject, true, true );

		// exact clip of light faces against all planes
		for( int i = 0; i < 6; i++ )
		{
			// the light frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the light,
			// which must have some fragment inside the the portal stack planes to be visible
			if( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f )
			{
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for( int j = 0; j < 6; j++ )
			{
				if( j == i )
				{
					continue;
				}
				if( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}
			if( w.GetNumPoints() <= 2 )
			{
				continue;
			}

			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for( int j = 0; j < ps->numPortalPlanes - 1; j++ )
			{
				if( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}

			if( w.GetNumPoints() > 2 )
			{
				// part of the winding is visible through the portalStack,
				// so the light is not culled
				return false;
			}
		}

		// nothing was visible
		return true;
	}

	return false;
}

/*
===================
AddAreaViewEnvprobes

This is the only point where lights get added to the viewLights list.
Any lights that are visible through the current portalStack will have their scissor rect updated.
===================
*/
void idRenderWorldLocal::AddAreaViewEnvprobes( int areaNum, const portalStack_t* ps )
{
	portalArea_t* area = &portalAreas[ areaNum ];

	for( areaReference_t* lref = area->envprobeRefs.areaNext; lref != &area->envprobeRefs; lref = lref->areaNext )
	{
		RenderEnvprobeLocal* probe = lref->envprobe;

		// debug tool to allow viewing of only one light at a time
		if( r_singleEnvprobe.GetInteger() >= 0 && r_singleEnvprobe.GetInteger() != probe->index )
		{
			continue;
		}

		// check for being closed off behind a door
		// a light that doesn't cast shadows will still light even if it is behind a door
		if( r_useLightAreaCulling.GetBool() //&& !envprobe->LightCastsShadows()
				&& probe->areaNum != -1 && !tr.viewDef->connectedAreas[ probe->areaNum ] )
		{
			continue;
		}

		// cull frustum
		if( CullEnvprobeByPortals( probe, ps ) )
		{
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}

		viewEnvprobe_t* vProbe = R_SetEnvprobeDefViewEnvprobe( probe );

		// expand the scissor rect
		vProbe->scissorRect.Union( ps->rect );
	}
}

CONSOLE_COMMAND( generateEnvironmentProbes, "Generate environment probes", idCmdSystem::ArgCompletion_MapName )
{
	idStr			fullname;
	idStr			baseName;
	idMat3			axis[6], oldAxis;
	idVec3			oldPosition;
	renderView_t	ref;
	viewDef_t		primary;
	int				blends;
	const char*		extension;
	int				size;
	int				res_w, res_h, old_fov_x, old_fov_y;

	static const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

	res_w = renderSystem->GetWidth();
	res_h = renderSystem->GetHeight();

	baseName = tr.primaryWorld->mapName;
	baseName.StripFileExtension();

	size = 256;
	blends = 1;

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
	oldPosition = primary.renderView.vieworg;
	oldAxis = primary.renderView.viewaxis;
	old_fov_x = primary.renderView.fov_x;
	old_fov_y = primary.renderView.fov_y;

	for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
	{
		RenderEnvprobeLocal* def = tr.primaryWorld->envprobeDefs[i];
		if( def == NULL )
		{
			continue;
		}

		for( int j = 0 ; j < 6 ; j++ )
		{
			ref = primary.renderView;

			extension = envDirection[ j ];

			ref.fov_x = ref.fov_y = 90;

			ref.vieworg = def->parms.origin;
			ref.viewaxis = axis[j];
			fullname.Format( "env/%s_envprobe%i%s", baseName.c_str(), i, extension );

			tr.TakeScreenshot( size, size, fullname, blends, &ref, PNG );
			//tr.CaptureRenderToFile( fullname, false );
		}
	}

	// restore the original resolution, axis and fov
	ref.vieworg = oldPosition;
	ref.viewaxis = oldAxis;
	ref.fov_x = old_fov_x;
	ref.fov_y = old_fov_y;
	cvarSystem->SetCVarInteger( "r_windowWidth", res_w );
	cvarSystem->SetCVarInteger( "r_windowHeight", res_h );
	R_SetNewMode( false ); // the same as "vid_restart"

	common->Printf( "Wrote a env set with the name %s\n", baseName );
}

