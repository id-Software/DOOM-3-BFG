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
#ifndef __SYS_STATS_H__
#define __SYS_STATS_H__

//------------------------
// leaderboardError_t
//------------------------
enum leaderboardError_t
{
	LEADERBOARD_ERROR_NONE,
	LEADERBOARD_ERROR_FAILED,				// General error occurred
	LEADERBOARD_ERROR_NOT_ONLINE,			// Not online to request leaderboards
	LEADERBOARD_ERROR_BUSY,					// Unable to download leaderboards right now (download already in progress)
	LEADERBOARD_ERROR_INVALID_USER,			// Unable to request leaderboards as the given user
	LEADERBOARD_ERROR_INVALID_REQUEST,		// The leaderboard request was invalid
	LEADERBOARD_ERROR_DOWNLOAD,				// An error occurred while downloading the leaderboard
	LEADERBOARD_ERROR_MAX
};

/*
================================================
idLeaderboardCallback
================================================
*/
class idLeaderboardCallback : public idCallback
{
public:
	struct row_t
	{
		bool		hasAttachment;
		int64		attachmentID;
		idStr		name;
		int64		rank;
		idArray< int64, MAX_LEADERBOARD_COLUMNS > 	columns;
	};

	idLeaderboardCallback() : def( NULL ), startIndex( -1 ), localIndex( -1 ), numRowsInLeaderboard( -1 ), errorCode( LEADERBOARD_ERROR_NONE ) { }
	virtual idLeaderboardCallback* Clone() const = 0;

	// Used by the platform handlers to set data
	void 							ResetRows()
	{
		rows.Clear();
	}
	void 							AddRow( const row_t& row )
	{
		rows.Append( row );
	}
	void 							SetNumRowsInLeaderboard( int32 i )
	{
		numRowsInLeaderboard = i;
	}
	void 							SetDef( const leaderboardDefinition_t* def_ )
	{
		def = def_;
	}
	void 							SetStartIndex( int startIndex_ )
	{
		startIndex = startIndex_;
	}
	void 							SetLocalIndex( int localIndex_ )
	{
		localIndex = localIndex_;
	}
	void							SetErrorCode( leaderboardError_t errorCode )
	{
		this->errorCode = errorCode;
	}

	// Used in user callback for information retrieval
	const leaderboardDefinition_t* 	GetDef() const
	{
		return def;
	}
	int								GetStartIndex() const
	{
		return startIndex;
	}
	const idList< row_t >& 			GetRows() const
	{
		return rows;
	}
	int								GetNumRowsInLeaderboard() const
	{
		return numRowsInLeaderboard;
	}
	int								GetLocalIndex() const
	{
		return localIndex;
	}
	leaderboardError_t				GetErrorCode() const
	{
		return this->errorCode;
	}

protected:
	const leaderboardDefinition_t* 	def;					// leaderboard def
	int								startIndex;				// where the first row starts in the online leaderboard
	int								localIndex;				// if player is in the rows, this is the offset of him within the returned rows
	idList< row_t >					rows;					// leaderboard entries for the request
	int								numRowsInLeaderboard;	// total number of rows in the online leaderboard
	leaderboardError_t				errorCode;				// error, if any, that occurred during last operation
};



#endif // !__SYS_STATS_H__
