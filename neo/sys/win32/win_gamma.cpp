/*
===========================================================================

Doom 3 BFG Edition GPL Source Code

===========================================================================
*/

#include "win_local.h"

static unsigned short s_oldHardwareGamma[3][256];

/*
** GLimp_SetGamma
**
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
    // Stub for cross-compilation
    (void)red;
    (void)green;
    (void)blue;
}

/*
** WG_GetOldGammaRamp
**
*/
void WG_GetOldGammaRamp( void ) {
    HDC hDC = GetDC( GetDesktopWindow() );
    GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
    ReleaseDC( GetDesktopWindow(), hDC );
}

/*
** WG_RestoreGamma
*/
void WG_RestoreGamma( void ) {
    HDC hDC;
    if ( s_oldHardwareGamma[0][255] == 0 ) {
        return;
    }
    hDC = GetDC( GetDesktopWindow() );
    SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
    ReleaseDC( GetDesktopWindow(), hDC );
}
