/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"

int		c_glfaces;

int PortalVisibleSides( uPortal_t* p )
{
	int		fcon, bcon;
	
	if( !p->onnode )
		return 0;		// outside
		
	fcon = p->nodes[0]->opaque;
	bcon = p->nodes[1]->opaque;
	
	// same contents never create a face
	if( fcon == bcon )
		return 0;
		
	if( !fcon )
		return 1;
	if( !bcon )
		return 2;
	return 0;
}

void OutputWinding( idWinding* w, idFile* glview )
{
	static	int	level = 128;
	float		light;
	int			i;
	
	glview->WriteFloatString( "%i\n", w->GetNumPoints() );
	level += 28;
	light = ( level & 255 ) / 255.0;
	for( i = 0; i < w->GetNumPoints(); i++ )
	{
		glview->WriteFloatString( "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
								  ( *w )[i][0],
								  ( *w )[i][1],
								  ( *w )[i][2],
								  light,
								  light,
								  light );
	}
	glview->WriteFloatString( "\n" );
}

/*
=============
OutputPortal
=============
*/
void OutputPortal( uPortal_t* p, idFile* glview )
{
	idWinding*	w;
	int		sides;
	
	sides = PortalVisibleSides( p );
	if( !sides )
	{
		return;
	}
	
	c_glfaces++;
	
	w = p->winding;
	
	if( sides == 2 )  		// back side
	{
		w = w->Reverse();
	}
	
	OutputWinding( w, glview );
	
	if( sides == 2 )
	{
		delete w;
	}
}

/*
=============
WriteGLView_r
=============
*/
void WriteGLView_r( node_t* node, idFile* glview )
{
	uPortal_t*	p, *nextp;
	
	if( node->planenum != PLANENUM_LEAF )
	{
		WriteGLView_r( node->children[0], glview );
		WriteGLView_r( node->children[1], glview );
		return;
	}
	
	// write all the portals
	for( p = node->portals; p; p = nextp )
	{
		if( p->nodes[0] == node )
		{
			OutputPortal( p, glview );
			nextp = p->next[0];
		}
		else
		{
			nextp = p->next[1];
		}
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView( tree_t* tree, char* source )
{
	idFile* glview;
	
	c_glfaces = 0;
	common->Printf( "Writing %s\n", source );
	
	glview = fileSystem->OpenExplicitFileWrite( source );
	if( !glview )
	{
		common->Error( "Couldn't open %s", source );
	}
	WriteGLView_r( tree->headnode, glview );
	fileSystem->CloseFile( glview );
	
	common->Printf( "%5i c_glfaces\n", c_glfaces );
}

