/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

// This is for "official" HDMI 3D support with with the left eye above the right and a guard band in the middle
// Some displays which don't support this can still do stereo 3D by packing 2 eyes into a single (mono-sized) buffer
enum hdmi3DState_t
{
	HDMI3D_NOT_SUPPORTED,	// The TV doesn't support it
	HDMI3D_NOT_ENABLED,		// The TV supports it, but the user disabled it
	HDMI3D_NOT_ACTIVE,		// The TV supports it, and the user enabled it, but it's not active
	HDMI3D_ACTIVE
};


//================================================================================================
// class idRenderContext
//================================================================================================
class idRenderContext
{
public:
	idRenderContext() : depthHackValue( 0.0f ), weaponDepthHackValue( 1.0f ) {}
	virtual			~idRenderContext() {};

	void			InitContext();
	void			Shutdown();
	void			PrepareSwap();
	void			SwapBuffers( bool forceVsync = false );
	void			SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] );

	hdmi3DState_t	GetHDMI3DState();
	void			EnableHDMI3D( const bool enable );

	// Back End Rendering
	void			ExecuteBackendCommands( const emptyCommand_t* cmds );
	void			Clear( float r, float g, float b, float a );
	void			InitGraphicsAPIWrapper();

	// Debug Tools
	void			RenderDebugTools( drawSurf_t** drawSurfs, int numDrawSurfs );

	void			SetWrapperContext( const wrapperContext_t& context );
	void			SetWrapperConfig( const wrapperConfig_t& config );

	// Texture Resolves
	void			ResolveTargetColor( idImage* image );
	void			ResolveTargetDepth( idImage* image );
	void			ResolveTargetColor( idImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );
	void			ResolveTargetDepth( idImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );

	void			SetDepthHackValue( float depth )
	{
		depthHackValue = depth;
	}
	float			GetDepthHackValue() const
	{
		return depthHackValue;
	}
	void			SetWeaponDepthHackValue( float depth )
	{
		weaponDepthHackValue = depth;
	}
	float			GetWeaponDepthHackValue() const
	{
		return weaponDepthHackValue;
	}

	uint64			GetGPUFrameMicroSec() const
	{
		return GPUFrameMicroSec;
	}

private:
	float			depthHackValue;
	float			weaponDepthHackValue;
	uint64			GPUFrameMicroSec;
};

extern idRenderContext rRenderContext;

#endif	// !__RENDERCONTEXT_H__
