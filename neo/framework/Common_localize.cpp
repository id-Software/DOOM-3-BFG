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

#include "Common_local.h"

idCVar com_product_lang_ext( "com_product_lang_ext", "1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_ARCHIVE, "Extension to use when creating language files." );

/*
=================
LoadMapLocalizeData
=================
*/
typedef idHashTable<idStrList> ListHash;
void LoadMapLocalizeData( ListHash& listHash )
{

	idStr fileName = "map_localize.cfg";
	const char* buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if( fileSystem->ReadFile( fileName, ( void** )&buffer ) > 0 )
	{
		src.LoadMemory( buffer, strlen( buffer ), fileName );
		if( src.IsLoaded() )
		{
			idStr classname;
			idToken token;



			while( src.ReadToken( &token ) )
			{
				classname = token;
				src.ExpectTokenString( "{" );

				idStrList list;
				while( src.ReadToken( &token ) )
				{
					if( token == "}" )
					{
						break;
					}
					list.Append( token );
				}

				listHash.Set( classname, list );
			}
		}
		fileSystem->FreeFile( ( void* )buffer );
	}

}

void LoadGuiParmExcludeList( idStrList& list )
{

	idStr fileName = "guiparm_exclude.cfg";
	const char* buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if( fileSystem->ReadFile( fileName, ( void** )&buffer ) > 0 )
	{
		src.LoadMemory( buffer, strlen( buffer ), fileName );
		if( src.IsLoaded() )
		{
			idStr classname;
			idToken token;



			while( src.ReadToken( &token ) )
			{
				list.Append( token );
			}
		}
		fileSystem->FreeFile( ( void* )buffer );
	}
}

bool TestMapVal( idStr& str )
{
	//Already Localized?
	if( str.Find( "#str_" ) != -1 )
	{
		return false;
	}

	return true;
}

bool TestGuiParm( const char* parm, const char* value, idStrList& excludeList )
{

	idStr testVal = value;

	//Already Localized?
	if( testVal.Find( "#str_" ) != -1 )
	{
		return false;
	}

	//Numeric
	if( testVal.IsNumeric() )
	{
		return false;
	}

	//Contains ::
	if( testVal.Find( "::" ) != -1 )
	{
		return false;
	}

	//Contains /
	if( testVal.Find( "/" ) != -1 )
	{
		return false;
	}

	if( excludeList.Find( testVal ) )
	{
		return false;
	}

	return true;
}

void GetFileList( const char* dir, const char* ext, idStrList& list )
{

	//Recurse Subdirectories
	idStrList dirList;
	Sys_ListFiles( dir, "/", dirList );
	for( int i = 0; i < dirList.Num(); i++ )
	{
		if( dirList[i] == "." || dirList[i] == ".." )
		{
			continue;
		}
		idStr fullName = va( "%s/%s", dir, dirList[i].c_str() );
		GetFileList( fullName, ext, list );
	}

	idStrList fileList;
	Sys_ListFiles( dir, ext, fileList );
	for( int i = 0; i < fileList.Num(); i++ )
	{
		idStr fullName = va( "%s/%s", dir, fileList[i].c_str() );
		list.Append( fullName );
	}
}

int LocalizeMap( const char* mapName, idLangDict& langDict, ListHash& listHash, idStrList& excludeList, bool writeFile )
{

	common->Printf( "Localizing Map '%s'\n", mapName );

	int strCount = 0;

	idMapFile map;
	if( map.Parse( mapName, false, false ) )
	{
		int count = map.GetNumEntities();
		for( int j = 0; j < count; j++ )
		{
			idMapEntity* ent = map.GetEntity( j );
			if( ent )
			{

				idStr classname = ent->epairs.GetString( "classname" );

				//Hack: for info_location
				bool hasLocation = false;

				idStrList* list;
				listHash.Get( classname, &list );
				if( list )
				{

					for( int k = 0; k < list->Num(); k++ )
					{

						idStr val = ent->epairs.GetString( ( *list )[k], "" );

						if( val.Length() && classname == "info_location" && ( *list )[k] == "location" )
						{
							hasLocation = true;
						}

						if( val.Length() && TestMapVal( val ) )
						{

							if( !hasLocation || ( *list )[k] == "location" )
							{
								//Localize it!!!
								strCount++;
								ent->epairs.Set( ( *list )[k], langDict.AddString( val ) );
							}
						}
					}
				}

				listHash.Get( "all", &list );
				if( list )
				{
					for( int k = 0; k < list->Num(); k++ )
					{
						idStr val = ent->epairs.GetString( ( *list )[k], "" );
						if( val.Length() && TestMapVal( val ) )
						{
							//Localize it!!!
							strCount++;
							ent->epairs.Set( ( *list )[k], langDict.AddString( val ) );
						}
					}
				}

				//Localize the gui_parms
				const idKeyValue* kv = ent->epairs.MatchPrefix( "gui_parm" );
				while( kv )
				{
					if( TestGuiParm( kv->GetKey(), kv->GetValue(), excludeList ) )
					{
						//Localize It!
						strCount++;
						ent->epairs.Set( kv->GetKey(), langDict.AddString( kv->GetValue() ) );
					}
					kv = ent->epairs.MatchPrefix( "gui_parm", kv );
				}
			}
		}
		if( writeFile && strCount > 0 )
		{
			//Before we write the map file lets make a backup of the original
			idStr file =  fileSystem->RelativePathToOSPath( mapName );
			idStr bak = file.Left( file.Length() - 4 );
			bak.Append( ".bak_loc" );
			fileSystem->CopyFile( file, bak );

			map.Write( mapName, ".map" );
		}
	}

	common->Printf( "Count: %d\n", strCount );
	return strCount;
}

/*
=================
LocalizeMaps_f
=================
*/
CONSOLE_COMMAND( localizeMaps, "localize maps", NULL )
{
	if( args.Argc() < 2 )
	{
		common->Printf( "Usage: localizeMaps <count | dictupdate | all> <map>\n" );
		return;
	}

	int strCount = 0;

	bool count = false;
	bool dictUpdate = false;
	bool write = false;

	if( idStr::Icmp( args.Argv( 1 ), "count" ) == 0 )
	{
		count = true;
	}
	else if( idStr::Icmp( args.Argv( 1 ), "dictupdate" ) == 0 )
	{
		count = true;
		dictUpdate = true;
	}
	else if( idStr::Icmp( args.Argv( 1 ), "all" ) == 0 )
	{
		count = true;
		dictUpdate = true;
		write = true;
	}
	else
	{
		common->Printf( "Invalid Command\n" );
		common->Printf( "Usage: localizeMaps <count | dictupdate | all>\n" );
		return;

	}

	idLangDict strTable;
	idStr filename = va( "strings/english%.3i.lang", com_product_lang_ext.GetInteger() );

	{
		// I think this is equivalent...
		const byte* buffer = NULL;
		int len = fileSystem->ReadFile( filename, ( void** )&buffer );
		if( verify( len > 0 ) )
		{
			strTable.Load( buffer, len, filename );
		}
		fileSystem->FreeFile( ( void* )buffer );

		// ... to this
		//if ( strTable.Load( filename ) == false) {
		//	//This is a new file so set the base index
		//	strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
		//}
	}

	common->SetRefreshOnPrint( true );

	ListHash listHash;
	LoadMapLocalizeData( listHash );

	idStrList excludeList;
	LoadGuiParmExcludeList( excludeList );

	if( args.Argc() == 3 )
	{
		strCount += LocalizeMap( args.Argv( 2 ), strTable, listHash, excludeList, write );
	}
	else
	{
		idStrList files;
		GetFileList( "z:/d3xp/d3xp/maps/game", "*.map", files );
		for( int i = 0; i < files.Num(); i++ )
		{
			idStr file =  fileSystem->OSPathToRelativePath( files[i] );
			strCount += LocalizeMap( file, strTable, listHash, excludeList, write );
		}
	}

	if( count )
	{
		common->Printf( "Localize String Count: %d\n", strCount );
	}

	common->SetRefreshOnPrint( false );

	if( dictUpdate )
	{
		strTable.Save( filename );
	}
}

/*
=================
LocalizeGuis_f
=================
*/
CONSOLE_COMMAND( localizeGuis, "localize guis", NULL )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "Usage: localizeGuis <all | gui>\n" );
		return;
	}

	idLangDict strTable;

	idStr filename = va( "strings/english%.3i.lang", com_product_lang_ext.GetInteger() );

	{
		// I think this is equivalent...
		const byte* buffer = NULL;
		int len = fileSystem->ReadFile( filename, ( void** )&buffer );
		if( verify( len > 0 ) )
		{
			strTable.Load( buffer, len, filename );
		}
		fileSystem->FreeFile( ( void* )buffer );

		// ... to this
		//if(strTable.Load( filename ) == false) {
		//	//This is a new file so set the base index
		//	strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
		//}
	}

	idFileList* files;
	if( idStr::Icmp( args.Argv( 1 ), "all" ) == 0 )
	{
		idStr game = cvarSystem->GetCVarString( "game_expansion" );
		if( game.Length() )
		{
			files = fileSystem->ListFilesTree( "guis", "*.gui", true, game );
		}
		else
		{
			files = fileSystem->ListFilesTree( "guis", "*.gui", true );
		}
		for( int i = 0; i < files->GetNumFiles(); i++ )
		{
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

		if( game.Length() )
		{
			files = fileSystem->ListFilesTree( "guis", "*.pd", true, game );
		}
		else
		{
			files = fileSystem->ListFilesTree( "guis", "*.pd", true, false, "d3xp" );
		}

		for( int i = 0; i < files->GetNumFiles(); i++ )
		{
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

	}
	else
	{
		commonLocal.LocalizeGui( args.Argv( 1 ), strTable );
	}
	strTable.Save( filename );
}

CONSOLE_COMMAND( localizeGuiParmsTest, "Create test files that show gui parms localized and ignored.", NULL )
{

	common->SetRefreshOnPrint( true );

	idFile* localizeFile = fileSystem->OpenFileWrite( "gui_parm_localize.csv" );
	idFile* noLocalizeFile = fileSystem->OpenFileWrite( "gui_parm_nolocalize.csv" );

	idStrList excludeList;
	LoadGuiParmExcludeList( excludeList );

	idStrList files;
	GetFileList( "z:/d3xp/d3xp/maps/game", "*.map", files );

	for( int i = 0; i < files.Num(); i++ )
	{

		common->Printf( "Testing Map '%s'\n", files[i].c_str() );
		idMapFile map;

		idStr file =  fileSystem->OSPathToRelativePath( files[i] );
		if( map.Parse( file, false, false ) )
		{
			int count = map.GetNumEntities();
			for( int j = 0; j < count; j++ )
			{
				idMapEntity* ent = map.GetEntity( j );
				if( ent )
				{
					const idKeyValue* kv = ent->epairs.MatchPrefix( "gui_parm" );
					while( kv )
					{
						if( TestGuiParm( kv->GetKey(), kv->GetValue(), excludeList ) )
						{
							idStr out = va( "%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str() );
							localizeFile->Write( out.c_str(), out.Length() );
						}
						else
						{
							idStr out = va( "%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str() );
							noLocalizeFile->Write( out.c_str(), out.Length() );
						}
						kv = ent->epairs.MatchPrefix( "gui_parm", kv );
					}
				}
			}
		}
	}

	fileSystem->CloseFile( localizeFile );
	fileSystem->CloseFile( noLocalizeFile );

	common->SetRefreshOnPrint( false );
}


CONSOLE_COMMAND( localizeMapsTest, "Create test files that shows which strings will be localized.", NULL )
{

	ListHash listHash;
	LoadMapLocalizeData( listHash );


	common->SetRefreshOnPrint( true );

	idFile* localizeFile = fileSystem->OpenFileWrite( "map_localize.csv" );

	idStrList files;
	GetFileList( "z:/d3xp/d3xp/maps/game", "*.map", files );

	for( int i = 0; i < files.Num(); i++ )
	{

		common->Printf( "Testing Map '%s'\n", files[i].c_str() );
		idMapFile map;

		idStr file =  fileSystem->OSPathToRelativePath( files[i] );
		if( map.Parse( file, false, false ) )
		{
			int count = map.GetNumEntities();
			for( int j = 0; j < count; j++ )
			{
				idMapEntity* ent = map.GetEntity( j );
				if( ent )
				{

					//Temp code to get a list of all entity key value pairs
					/*idStr classname = ent->epairs.GetString("classname");
					if(classname == "worldspawn" || classname == "func_static" || classname == "light" || classname == "speaker" || classname.Left(8) == "trigger_") {
						continue;
					}
					for( int i = 0; i < ent->epairs.GetNumKeyVals(); i++) {
						const idKeyValue* kv = ent->epairs.GetKeyVal(i);
						idStr out = va("%s,%s,%s,%s\r\n", classname.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str(), file.c_str());
						localizeFile->Write( out.c_str(), out.Length() );
					}*/

					idStr classname = ent->epairs.GetString( "classname" );

					//Hack: for info_location
					bool hasLocation = false;

					idStrList* list;
					listHash.Get( classname, &list );
					if( list )
					{

						for( int k = 0; k < list->Num(); k++ )
						{

							idStr val = ent->epairs.GetString( ( *list )[k], "" );

							if( classname == "info_location" && ( *list )[k] == "location" )
							{
								hasLocation = true;
							}

							if( val.Length() && TestMapVal( val ) )
							{

								if( !hasLocation || ( *list )[k] == "location" )
								{
									idStr out = va( "%s,%s,%s\r\n", val.c_str(), ( *list )[k].c_str(), file.c_str() );
									localizeFile->Write( out.c_str(), out.Length() );
								}
							}
						}
					}

					listHash.Get( "all", &list );
					if( list )
					{
						for( int k = 0; k < list->Num(); k++ )
						{
							idStr val = ent->epairs.GetString( ( *list )[k], "" );
							if( val.Length() && TestMapVal( val ) )
							{
								idStr out = va( "%s,%s,%s\r\n", val.c_str(), ( *list )[k].c_str(), file.c_str() );
								localizeFile->Write( out.c_str(), out.Length() );
							}
						}
					}
				}
			}
		}
	}

	fileSystem->CloseFile( localizeFile );

	common->SetRefreshOnPrint( false );
}

/*
===============
idCommonLocal::LocalizeSpecificMapData
===============
*/
void idCommonLocal::LocalizeSpecificMapData( const char* fileName, idLangDict& langDict, const idLangDict& replaceArgs )
{
	idStr out, ws, work;

	idMapFile map;
	if( map.Parse( fileName, false, false ) )
	{
		int count = map.GetNumEntities();
		for( int i = 0; i < count; i++ )
		{
			idMapEntity* ent = map.GetEntity( i );
			if( ent )
			{
				for( int j = 0; j < replaceArgs.GetNumKeyVals(); j++ )
				{
					const idLangKeyValue* kv = replaceArgs.GetKeyVal( j );
					const char* temp = ent->epairs.GetString( kv->key );
					if( ( temp != NULL ) && *temp )
					{
						idStr val = kv->value;
						if( val == temp )
						{
							ent->epairs.Set( kv->key, langDict.AddString( temp ) );
						}
					}
				}
			}
		}
		map.Write( fileName, ".map" );
	}
}

/*
===============
idCommonLocal::LocalizeMapData
===============
*/
void idCommonLocal::LocalizeMapData( const char* fileName, idLangDict& langDict )
{
	const char* buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	common->SetRefreshOnPrint( true );

	if( fileSystem->ReadFile( fileName, ( void** )&buffer ) > 0 )
	{
		src.LoadMemory( buffer, strlen( buffer ), fileName );
		if( src.IsLoaded() )
		{
			common->Printf( "Processing %s\n", fileName );
			idStr mapFileName;
			idToken token, token2;
			idLangDict replaceArgs;
			while( src.ReadToken( &token ) )
			{
				mapFileName = token;
				replaceArgs.Clear();
				src.ExpectTokenString( "{" );
				while( src.ReadToken( &token ) )
				{
					if( token == "}" )
					{
						break;
					}
					if( src.ReadToken( &token2 ) )
					{
						if( token2 == "}" )
						{
							break;
						}
						replaceArgs.AddKeyVal( token, token2 );
					}
				}
				common->Printf( "  localizing map %s...\n", mapFileName.c_str() );
				LocalizeSpecificMapData( mapFileName, langDict, replaceArgs );
			}
		}
		fileSystem->FreeFile( ( void* )buffer );
	}

	common->SetRefreshOnPrint( false );
}

/*
===============
idCommonLocal::LocalizeGui
===============
*/
void idCommonLocal::LocalizeGui( const char* fileName, idLangDict& langDict )
{
	idStr out, ws, work;
	const char* buffer = NULL;
	out.Empty();
	int k;
	char ch;
	char slash = '\\';
	char tab = 't';
	char nl = 'n';
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if( fileSystem->ReadFile( fileName, ( void** )&buffer ) > 0 )
	{
		src.LoadMemory( buffer, strlen( buffer ), fileName );
		if( src.IsLoaded() )
		{
			idFile* outFile = fileSystem->OpenFileWrite( fileName );
			common->Printf( "Processing %s\n", fileName );

			const bool captureToImage = false;
			UpdateScreen( captureToImage );
			idToken token;
			while( src.ReadToken( &token ) )
			{
				src.GetLastWhiteSpace( ws );
				out += ws;
				if( token.type == TT_STRING )
				{
					out += va( "\"%s\"", token.c_str() );
				}
				else
				{
					out += token;
				}
				if( out.Length() > 200000 )
				{
					outFile->Write( out.c_str(), out.Length() );
					out = "";
				}
				work = token.Right( 6 );
				if( token.Icmp( "text" ) == 0 || work.Icmp( "::text" ) == 0 || token.Icmp( "choices" ) == 0 )
				{
					if( src.ReadToken( &token ) )
					{
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						token = langDict.AddString( token );
						out += "\"";
						for( k = 0; k < token.Length(); k++ )
						{
							ch = token[k];
							if( ch == '\t' )
							{
								out += slash;
								out += tab;
							}
							else if( ch == '\n' || ch == '\r' )
							{
								out += slash;
								out += nl;
							}
							else
							{
								out += ch;
							}
						}
						out += "\"";
					}
				}
				else if( token.Icmp( "comment" ) == 0 )
				{
					if( src.ReadToken( &token ) )
					{
						// need to write these out by hand to preserve any \n's
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						out += "\"";
						for( k = 0; k < token.Length(); k++ )
						{
							ch = token[k];
							if( ch == '\t' )
							{
								out += slash;
								out += tab;
							}
							else if( ch == '\n' || ch == '\r' )
							{
								out += slash;
								out += nl;
							}
							else
							{
								out += ch;
							}
						}
						out += "\"";
					}
				}
			}
			outFile->Write( out.c_str(), out.Length() );
			fileSystem->CloseFile( outFile );
		}
		fileSystem->FreeFile( ( void* )buffer );
	}
}
