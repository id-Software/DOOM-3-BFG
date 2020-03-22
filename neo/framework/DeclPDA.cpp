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

idCVar g_useOldPDAStrings( "g_useOldPDAStrings", "0", CVAR_BOOL, "Read strings from the .pda files rather than from the .lang file" );

/*
=================
idDeclPDA::Size
=================
*/
size_t idDeclPDA::Size() const
{
	return sizeof( idDeclPDA );
}

/*
===============
idDeclPDA::Print
===============
*/
void idDeclPDA::Print() const
{
	common->Printf( "Implement me\n" );
}

/*
===============
idDeclPDA::List
===============
*/
void idDeclPDA::List() const
{
	common->Printf( "Implement me\n" );
}

/*
================
idDeclPDA::Parse
================
*/
bool idDeclPDA::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;

	idStr baseStrId = va( "#str_%s_pda_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
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

		if( !token.Icmp( "name" ) )
		{
			src.ReadToken( &token );

			if( g_useOldPDAStrings.GetBool() )
			{
				pdaName = token;
			}
			else
			{
				pdaName = idLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if( !token.Icmp( "fullname" ) )
		{
			src.ReadToken( &token );

			if( g_useOldPDAStrings.GetBool() )
			{
				fullName = token;
			}
			else
			{
				fullName = idLocalization::GetString( baseStrId + "fullname" );
			}
			continue;
		}

		if( !token.Icmp( "icon" ) )
		{
			src.ReadToken( &token );
			icon = token;
			continue;
		}

		if( !token.Icmp( "id" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				id = token;
			}
			else
			{
				id = idLocalization::GetString( baseStrId + "id" );
			}
			continue;
		}

		if( !token.Icmp( "post" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				post = token;
			}
			else
			{
				post = idLocalization::GetString( baseStrId + "post" );
			}
			continue;
		}

		if( !token.Icmp( "title" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				title = token;
			}
			else
			{
				title = idLocalization::GetString( baseStrId + "title" );
			}
			continue;
		}

		if( !token.Icmp( "security" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				security = token;
			}
			else
			{
				security = idLocalization::GetString( baseStrId + "security" );
			}
			continue;
		}

		if( !token.Icmp( "pda_email" ) )
		{
			src.ReadToken( &token );
			emails.Append( static_cast<const idDeclEmail*>( declManager->FindType( DECL_EMAIL, token ) ) );
			continue;
		}

		if( !token.Icmp( "pda_audio" ) )
		{
			src.ReadToken( &token );
			audios.Append( static_cast<const idDeclAudio*>( declManager->FindType( DECL_AUDIO, token ) ) );
			continue;
		}

		if( !token.Icmp( "pda_video" ) )
		{
			src.ReadToken( &token );
			videos.Append( static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, token ) ) );
			continue;
		}

	}

	if( src.HadError() )
	{
		src.Warning( "PDA decl '%s' had a parse error", GetName() );
		return false;
	}

	originalVideos = videos.Num();
	originalEmails = emails.Num();
	return true;
}

/*
===================
idDeclPDA::DefaultDefinition
===================
*/
const char* idDeclPDA::DefaultDefinition() const
{
	return
		"{\n"
		"\t"		"name  \"default pda\"\n"
		"}";
}

/*
===================
idDeclPDA::FreeData
===================
*/
void idDeclPDA::FreeData()
{
	videos.Clear();
	audios.Clear();
	emails.Clear();
	originalEmails = 0;
	originalVideos = 0;
}

/*
=================
idDeclPDA::RemoveAddedEmailsAndVideos
=================
*/
void idDeclPDA::RemoveAddedEmailsAndVideos() const
{
	int num = emails.Num();
	if( originalEmails < num )
	{
		while( num && num > originalEmails )
		{
			emails.RemoveIndex( --num );
		}
	}
	num = videos.Num();
	if( originalVideos < num )
	{
		while( num && num > originalVideos )
		{
			videos.RemoveIndex( --num );
		}
	}
}

/*
=================
idDeclPDA::SetSecurity
=================
*/
void idDeclPDA::SetSecurity( const char* sec ) const
{
	security = sec;
}

/*
=================
idDeclEmail::Size
=================
*/
size_t idDeclEmail::Size() const
{
	return sizeof( idDeclEmail );
}

/*
===============
idDeclEmail::Print
===============
*/
void idDeclEmail::Print() const
{
	common->Printf( "Implement me\n" );
}

/*
===============
idDeclEmail::List
===============
*/
void idDeclEmail::List() const
{
	common->Printf( "Implement me\n" );
}

/*
================
idDeclEmail::Parse
================
*/
bool idDeclEmail::Parse( const char* _text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;

	idStr baseStrId = va( "#str_%s_email_", GetName() );

	src.LoadMemory( _text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	text = "";
	// scan through, identifying each individual parameter
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

		if( !token.Icmp( "subject" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				subject = token;
			}
			else
			{
				subject = idLocalization::GetString( baseStrId + "subject" );
			}
			continue;
		}

		if( !token.Icmp( "to" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				to = token;
			}
			else
			{
				to = idLocalization::GetString( baseStrId + "to" );
			}
			continue;
		}

		if( !token.Icmp( "from" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				from = token;
			}
			else
			{
				from = idLocalization::GetString( baseStrId + "from" );
			}
			continue;
		}

		if( !token.Icmp( "date" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				date = token;
			}
			else
			{
				date = idLocalization::GetString( baseStrId + "date" );
			}
			continue;
		}

		if( !token.Icmp( "text" ) )
		{
			src.ReadToken( &token );
			if( token != "{" )
			{
				src.Warning( "Email decl '%s' had a parse error", GetName() );
				return false;
			}
			while( src.ReadToken( &token ) && token != "}" )
			{
				text += token;
			}
			if( !g_useOldPDAStrings.GetBool() )
			{
				text = idLocalization::GetString( baseStrId + "text" );
			}
			continue;
		}
	}

	if( src.HadError() )
	{
		src.Warning( "Email decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
idDeclEmail::DefaultDefinition
===================
*/
const char* idDeclEmail::DefaultDefinition() const
{
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"to\t5Mail recipient\n"
		"\t\t"		"subject\t5Nothing\n"
		"\t\t"		"from\t5No one\n"
		"\t"	"}\n"
		"}";
}

/*
===================
idDeclEmail::FreeData
===================
*/
void idDeclEmail::FreeData()
{
}

/*
=================
idDeclVideo::Size
=================
*/
size_t idDeclVideo::Size() const
{
	return sizeof( idDeclVideo );
}

/*
===============
idDeclVideo::Print
===============
*/
void idDeclVideo::Print() const
{
	common->Printf( "Implement me\n" );
}

/*
===============
idDeclVideo::List
===============
*/
void idDeclVideo::List() const
{
	common->Printf( "Implement me\n" );
}

/*
================
idDeclVideo::Parse
================
*/
bool idDeclVideo::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;

	idStr baseStrId = va( "#str_%s_video_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
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

		if( !token.Icmp( "name" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				videoName = token;
			}
			else
			{
				videoName = idLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if( !token.Icmp( "preview" ) )
		{
			src.ReadToken( &token );
			preview = declManager->FindMaterial( token );
			continue;
		}

		if( !token.Icmp( "video" ) )
		{
			src.ReadToken( &token );
			video = declManager->FindMaterial( token );
			continue;
		}

		if( !token.Icmp( "info" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				info = token;
			}
			else
			{
				info = idLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}

		if( !token.Icmp( "audio" ) )
		{
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

	}

	if( src.HadError() )
	{
		src.Warning( "Video decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
idDeclVideo::DefaultDefinition
===================
*/
const char* idDeclVideo::DefaultDefinition() const
{
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Video\n"
		"\t"	"}\n"
		"}";
}

/*
===================
idDeclVideo::FreeData
===================
*/
void idDeclVideo::FreeData()
{
}

/*
=================
idDeclAudio::Size
=================
*/
size_t idDeclAudio::Size() const
{
	return sizeof( idDeclAudio );
}

/*
===============
idDeclAudio::Print
===============
*/
void idDeclAudio::Print() const
{
	common->Printf( "Implement me\n" );
}

/*
===============
idDeclAudio::List
===============
*/
void idDeclAudio::List() const
{
	common->Printf( "Implement me\n" );
}

/*
================
idDeclAudio::Parse
================
*/
bool idDeclAudio::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;

	idStr baseStrId = va( "#str_%s_audio_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
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

		if( !token.Icmp( "name" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				audioName = token;
			}
			else
			{
				audioName = idLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if( !token.Icmp( "audio" ) )
		{
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

		if( !token.Icmp( "info" ) )
		{
			src.ReadToken( &token );
			if( g_useOldPDAStrings.GetBool() )
			{
				info = token;
			}
			else
			{
				info = idLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}
	}

	if( src.HadError() )
	{
		src.Warning( "Audio decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
idDeclAudio::DefaultDefinition
===================
*/
const char* idDeclAudio::DefaultDefinition() const
{
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Audio\n"
		"\t"	"}\n"
		"}";
}

/*
===================
idDeclAudio::FreeData
===================
*/
void idDeclAudio::FreeData()
{
}
