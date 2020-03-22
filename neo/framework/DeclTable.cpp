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

#include "precompiled.h"
#pragma hdrstop


/*
=================
idDeclTable::TableLookup
=================
*/
float idDeclTable::TableLookup( float index ) const
{
	int iIndex;
	float iFrac;

	int domain = values.Num() - 1;

	if( domain <= 1 )
	{
		return 1.0f;
	}

	if( clamp )
	{
		index *= ( domain - 1 );
		if( index >= domain - 1 )
		{
			return values[domain - 1];
		}
		else if( index <= 0 )
		{
			return values[0];
		}
		iIndex = idMath::Ftoi( index );
		iFrac = index - iIndex;
	}
	else
	{
		index *= domain;

		if( index < 0 )
		{
			index += domain * idMath::Ceil( -index / domain );
		}

		iIndex = idMath::Ftoi( idMath::Floor( index ) );
		iFrac = index - iIndex;
		iIndex = iIndex % domain;
	}

	if( !snap )
	{
		// we duplicated the 0 index at the end at creation time, so we
		// don't need to worry about wrapping the filter
		return values[iIndex] * ( 1.0f - iFrac ) + values[iIndex + 1] * iFrac;
	}

	return values[iIndex];
}

/*
=================
idDeclTable::Size
=================
*/
size_t idDeclTable::Size() const
{
	return sizeof( idDeclTable ) + values.Allocated();
}

/*
=================
idDeclTable::FreeData
=================
*/
void idDeclTable::FreeData()
{
	snap = false;
	clamp = false;
	values.Clear();
}

/*
=================
idDeclTable::DefaultDefinition
=================
*/
const char* idDeclTable::DefaultDefinition() const
{
	return "{ { 0 } }";
}

/*
=================
idDeclTable::Parse
=================
*/
bool idDeclTable::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;
	float v;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	snap = false;
	clamp = false;
	values.Clear();

	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "}" )
		{
			break;
		}

		if( token.Icmp( "snap" ) == 0 )
		{
			snap = true;
		}
		else if( token.Icmp( "clamp" ) == 0 )
		{
			clamp = true;
		}
		else if( token.Icmp( "{" ) == 0 )
		{

			while( 1 )
			{
				bool errorFlag;

				v = src.ParseFloat( &errorFlag );
				if( errorFlag )
				{
					// we got something non-numeric
					MakeDefault();
					return false;
				}

				values.Append( v );

				src.ReadToken( &token );
				if( token == "}" )
				{
					break;
				}
				if( token == "," )
				{
					continue;
				}
				src.Warning( "expected comma or brace" );
				MakeDefault();
				return false;
			}

		}
		else
		{
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// copy the 0 element to the end, so lerping doesn't
	// need to worry about the wrap case
	float val = values[0];		// template bug requires this to not be in the Append()?
	values.Append( val );

	return true;
}
