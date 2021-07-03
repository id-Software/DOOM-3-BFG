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

#pragma hdrstop
#include "precompiled.h"

#include "ListGUILocal.h"

/*
====================
idListGUILocal::StateChanged
====================
*/
void idListGUILocal::StateChanged()
{
	int i;

	if( !m_stateUpdates )
	{
		return;
	}

	for( i = 0; i < Num(); i++ )
	{
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), ( *this )[i].c_str() );
	}
	for( i = Num() ; i < m_water ; i++ )
	{
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), "" );
	}
	m_water = Num();
	m_pGUI->StateChanged( Sys_Milliseconds() );
}

/*
====================
idListGUILocal::GetNumSelections
====================
*/
int idListGUILocal::GetNumSelections()
{
	return m_pGUI->State().GetInt( va( "%s_numsel", m_name.c_str() ) );
}

/*
====================
idListGUILocal::GetSelection
====================
*/
int idListGUILocal::GetSelection( char* s, int size, int _sel ) const
{
	if( s )
	{
		s[ 0 ] = '\0';
	}
	int sel = m_pGUI->State().GetInt( va( "%s_sel_%i", m_name.c_str(), _sel ), "-1" );
	if( sel == -1 || sel >= m_ids.Num() )
	{
		return -1;
	}
	if( s )
	{
		idStr::snPrintf( s, size, m_pGUI->State().GetString( va( "%s_item_%i", m_name.c_str(), sel ), "" ) );
	}
	// don't let overflow
	if( sel >= m_ids.Num() )
	{
		sel = 0;
	}
	m_pGUI->SetStateInt( va( "%s_selid_0", m_name.c_str() ), m_ids[ sel ] );
	return m_ids[ sel ];
}

/*
====================
idListGUILocal::SetSelection
====================
*/
void idListGUILocal::SetSelection( int sel )
{
	m_pGUI->SetStateInt( va( "%s_sel_0", m_name.c_str() ), sel );
	StateChanged();
}

/*
====================
idListGUILocal::Add
====================
*/
void idListGUILocal::Add( int id, const idStr& s )
{
	int i = m_ids.FindIndex( id );
	if( i == -1 )
	{
		Append( s );
		m_ids.Append( id );
	}
	else
	{
		( *this )[ i ] = s;
	}
	StateChanged();
}

/*
====================
idListGUILocal::Push
====================
*/
void idListGUILocal::Push( const idStr& s )
{
	Append( s );
	m_ids.Append( m_ids.Num() );
	StateChanged();
}

/*
====================
idListGUILocal::Del
====================
*/
bool idListGUILocal::Del( int id )
{
	int i = m_ids.FindIndex( id );
	if( i == -1 )
	{
		return false;
	}
	m_ids.RemoveIndex( i );
	this->RemoveIndex( i );
	StateChanged();
	return true;
}

/*
====================
idListGUILocal::Clear
====================
*/
void idListGUILocal::Clear()
{
	m_ids.Clear();
	idList<idStr, TAG_OLD_UI>::Clear();
	if( m_pGUI )
	{
		// will clear all the GUI variables and will set m_water back to 0
		StateChanged();
	}
}

/*
====================
idListGUILocal::IsConfigured
====================
*/
bool idListGUILocal::IsConfigured() const
{
	return m_pGUI != NULL;
}

/*
====================
idListGUILocal::SetStateChanges
====================
*/
void idListGUILocal::SetStateChanges( bool enable )
{
	m_stateUpdates = enable;
	StateChanged();
}

/*
====================
idListGUILocal::Shutdown
====================
*/
void idListGUILocal::Shutdown()
{
	m_pGUI = NULL;
	m_name.Clear();
	Clear();
}
