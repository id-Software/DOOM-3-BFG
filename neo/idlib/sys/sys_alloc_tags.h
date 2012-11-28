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
MEM_TAG( UNSET )		// This should never be used
MEM_TAG( STATIC_EXE	)	// The static exe, generally how much memory we are using before our main() function ever runs
MEM_TAG( DEBUG )		// Crap we don't care about, because it won't be in a retail build
MEM_TAG( NEW )			// Crap allocated with new which hasn't been given an explicit tag
MEM_TAG( BLOCKALLOC )	// Crap allocated with idBlockAlloc which hasn't been given an explicit tag
MEM_TAG( PHYSICAL )
MEM_TAG( TRI_VERTS )
MEM_TAG( TRI_INDEXES )
MEM_TAG( TRI_SHADOW )
MEM_TAG( TRI_PLANES )
MEM_TAG( TRI_SIL_INDEXES )
MEM_TAG( TRI_SIL_EDGE )
MEM_TAG( TRI_DOMINANT_TRIS )
MEM_TAG( TRI_MIR_VERT )
MEM_TAG( TRI_DUP_VERT )
MEM_TAG( SRFTRIS )
MEM_TAG( TEMP )			// Temp data which should be automatically freed at the end of the function
MEM_TAG( PAGE )
MEM_TAG( DEFRAG_BLOCK )
MEM_TAG( MATH )
MEM_TAG( MD5_WEIGHT )
MEM_TAG( MD5_BASE )
MEM_TAG( MD5_ANIM )
MEM_TAG( MD5_INDEX )
MEM_TAG( JOINTMAT )
MEM_TAG( DECAL )
MEM_TAG( CULLBITS )
MEM_TAG( TEXCOORDS )
MEM_TAG( VERTEXREMAP )
MEM_TAG( JOINTBUFFER )
MEM_TAG( IMAGE )
MEM_TAG( DXBUFFER )
MEM_TAG( AUDIO )
MEM_TAG( FUNC_CALLBACK )
MEM_TAG( SAVEGAMES )
MEM_TAG( IDFILE )
MEM_TAG( NETWORKING )
MEM_TAG( SWF )
MEM_TAG( STEAM )
MEM_TAG( IDLIB )
MEM_TAG( IDLIB_LIST )
MEM_TAG( IDLIB_LIST_IMAGE )
MEM_TAG( IDLIB_LIST_SURFACE )
MEM_TAG( IDLIB_LIST_PHYSICS )
MEM_TAG( IDLIB_LIST_DECL )
MEM_TAG( IDLIB_LIST_CMD )
MEM_TAG( IDLIB_LIST_TRIANGLES )
MEM_TAG( IDLIB_LIST_MATERIAL )
MEM_TAG( IDLIB_LIST_SOUND )
MEM_TAG( IDLIB_LIST_SNAPSHOT )
MEM_TAG( IDLIB_LIST_MENU )
MEM_TAG( IDLIB_LIST_MENUWIDGET )
MEM_TAG( IDLIB_LIST_PLAYER )
MEM_TAG( IDLIB_LIST_MAP )
MEM_TAG( IDLIB_HASH )
MEM_TAG( IDLIB_STRING )
MEM_TAG( IDLIB_SURFACE )
MEM_TAG( IDLIB_WINDING )
MEM_TAG( IDLIB_LEXER )
MEM_TAG( IDLIB_PARSER )
MEM_TAG( AF )
MEM_TAG( COLLISION )
MEM_TAG( COLLISION_QUERY )
MEM_TAG( DECLTEXT )
MEM_TAG( RSX )
MEM_TAG( CVAR )
MEM_TAG( CRAP )				// Crap allocated with new which hasn't been given an explicit tag
MEM_TAG( CINEMATIC )
MEM_TAG( FONT )
MEM_TAG( MATERIAL )
MEM_TAG( MODEL )
MEM_TAG( RENDER )
MEM_TAG( RENDER_TOOLS )
MEM_TAG( RENDER_WINDING )
MEM_TAG( RENDER_STATIC )
MEM_TAG( RENDER_ENTITY )
MEM_TAG( RENDER_LIGHT )
MEM_TAG( RENDER_INTERACTION )
MEM_TAG( SURFACE )
MEM_TAG( LIGHT )
MEM_TAG( AI )
MEM_TAG( SCRIPT )
MEM_TAG( EVENTS )
MEM_TAG( JPG )
MEM_TAG( AAS )
MEM_TAG( STRING )
MEM_TAG( THREAD )
MEM_TAG( DECL )
MEM_TAG( SYSTEM )
MEM_TAG( PSN )
MEM_TAG( OLD_UI )
MEM_TAG( ANIM )
MEM_TAG( ANIMWEB )
MEM_TAG( PHYSICS )
MEM_TAG( PARTICLE )
MEM_TAG( ENTITY )
MEM_TAG( GAME )
MEM_TAG( FX )
MEM_TAG( PVS )
MEM_TAG( IDCLASS )
MEM_TAG( ZIP )
MEM_TAG( JOBLIST )
MEM_TAG( AMPLITUDE )
MEM_TAG( RESOURCE )
MEM_TAG( ACHIEVEMENT )
MEM_TAG( TARGET )
MEM_TAG( BINK )
MEM_TAG( PROJECTILE )
MEM_TAG( MOVER )
MEM_TAG( ACTOR )
MEM_TAG( PHYSICS_CLIP )
MEM_TAG( PHYSICS_CLIP_MOVER )
MEM_TAG( PHYSICS_CLIP_AF )
MEM_TAG( PHYSICS_CLIP_BRITTLE )
MEM_TAG( PHYSICS_CLIP_ENTITY )
MEM_TAG( PHYSICS_BRITTLE )
MEM_TAG( PHYSICS_AF )
MEM_TAG( RENDERPROG )
#undef MEM_TAG
