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
/*
** WIN_GAMMA.C
*/
#include <assert.h>
#include "win_local.h"
#include "../../renderer/tr_local.h"

static unsigned short s_oldHardwareGamma[3][256];

/*
** WG_GetOldGammaRamp
**
*/
void WG_GetOldGammaRamp( void )
{
	HDC			hDC;

	hDC = GetDC( GetDesktopWindow() );
	GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );


/*
** GLimp_SetGamma
**
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	unsigned short table[3][256];
	int i;

	if ( !glw_state.hDC )
	{
		return;
	}

	for ( i = 0; i < 256; i++ )
	{
		table[0][i] = ( ( ( unsigned short ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( unsigned short ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( unsigned short ) blue[i] ) << 8 ) | blue[i];
	}

	if ( !SetDeviceGammaRamp( glw_state.hDC, table ) ) {
		common->Printf( "WARNING: SetDeviceGammaRamp failed.\n" );
	}
}

/*
** WG_RestoreGamma
*/
void WG_RestoreGamma( void )
{
	HDC hDC;

	// if we never read in a reasonable looking
	// table, don't write it out
	if ( s_oldHardwareGamma[0][255] == 0 ) {
		return;
	}

	hDC = GetDC( GetDesktopWindow() );
	SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );
}

