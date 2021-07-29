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
#include "sys_local.h"

const char* sysLanguageNames[] =
{
	ID_LANG_ENGLISH, ID_LANG_FRENCH, ID_LANG_ITALIAN, ID_LANG_GERMAN, ID_LANG_SPANISH, ID_LANG_JAPANESE, NULL
};

const int numLanguages = sizeof( sysLanguageNames ) / sizeof sysLanguageNames[ 0 ] - 1;

// RB: allow sys_lang to be saved to config so it has to be set per cmdline only a single time
idCVar sys_lang( "sys_lang", ID_LANG_ENGLISH, CVAR_SYSTEM | CVAR_INIT | CVAR_ARCHIVE, "", sysLanguageNames, idCmdSystem::ArgCompletion_String<sysLanguageNames> );

idSysLocal			sysLocal;
idSys* 				sys = &sysLocal;

void idSysLocal::DebugPrintf( const char* fmt, ... )
{
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

void idSysLocal::DebugVPrintf( const char* fmt, va_list arg )
{
	Sys_DebugVPrintf( fmt, arg );
}

double idSysLocal::GetClockTicks()
{
	return Sys_GetClockTicks();
}

double idSysLocal::ClockTicksPerSecond()
{
	return Sys_ClockTicksPerSecond();
}

cpuid_t idSysLocal::GetProcessorId()
{
	return Sys_GetProcessorId();
}

const char* idSysLocal::GetProcessorString()
{
	return Sys_GetProcessorString();
}

const char* idSysLocal::FPU_GetState()
{
	return Sys_FPU_GetState();
}

bool idSysLocal::FPU_StackIsEmpty()
{
	return Sys_FPU_StackIsEmpty();
}

void idSysLocal::FPU_SetFTZ( bool enable )
{
	Sys_FPU_SetFTZ( enable );
}

void idSysLocal::FPU_SetDAZ( bool enable )
{
	Sys_FPU_SetDAZ( enable );
}

bool idSysLocal::LockMemory( void* ptr, int bytes )
{
	return Sys_LockMemory( ptr, bytes );
}

bool idSysLocal::UnlockMemory( void* ptr, int bytes )
{
	return Sys_UnlockMemory( ptr, bytes );
}

int idSysLocal::DLL_Load( const char* dllName )
{
	return Sys_DLL_Load( dllName );
}

void* idSysLocal::DLL_GetProcAddress( int dllHandle, const char* procName )
{
	return Sys_DLL_GetProcAddress( dllHandle, procName );
}

void idSysLocal::DLL_Unload( int dllHandle )
{
	Sys_DLL_Unload( dllHandle );
}

void idSysLocal::DLL_GetFileName( const char* baseName, char* dllName, int maxLength )
{
	idStr::snPrintf( dllName, maxLength, "%s" CPUSTRING ".dll", baseName );
}

sysEvent_t idSysLocal::GenerateMouseButtonEvent( int button, bool down )
{
	sysEvent_t ev;
	ev.evType = SE_KEY;
	ev.evValue = K_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

sysEvent_t idSysLocal::GenerateMouseMoveEvent( int deltax, int deltay )
{
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

void idSysLocal::FPU_EnableExceptions( int exceptions )
{
	Sys_FPU_EnableExceptions( exceptions );
}

/*
=================
Sys_TimeStampToStr
=================
*/
const char* Sys_TimeStampToStr( ID_TIME_T timeStamp )
{
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	time_t ts = ( time_t )timeStamp;
	tm*	time = localtime( &ts );
	if( time == NULL )
	{
		// String separated to prevent detection of trigraphs
		return "??" "/" "??" "/" "???? ??:??";
	}

	idStr out;

	idStr lang = cvarSystem->GetCVarString( "sys_lang" );
	if( lang.Icmp( ID_LANG_ENGLISH ) == 0 )
	{
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		if( time->tm_hour > 12 )
		{
			out += va( "%02d", time->tm_hour - 12 );
		}
		else if( time->tm_hour == 0 )
		{
			out += "12";
		}
		else
		{
			out += va( "%02d", time->tm_hour );
		}
		out += ":";
		out += va( "%02d", time->tm_min );
		if( time->tm_hour >= 12 )
		{
			out += "pm";
		}
		else
		{
			out += "am";
		}
	}
	else
	{
		// europeans get "day/month/year  24hour:min"
		out = va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		out += va( "%02d", time->tm_hour );
		out += ":";
		out += va( "%02d", time->tm_min );
	}
	idStr::Copynz( timeString, out, sizeof( timeString ) );

	return timeString;
}

/*
========================
Sys_SecToStr
========================
*/
const char* Sys_SecToStr( int sec )
{
	static char timeString[MAX_STRING_CHARS];

	int weeks = sec / ( 3600 * 24 * 7 );
	sec -= weeks * ( 3600 * 24 * 7 );

	int days = sec / ( 3600 * 24 );
	sec -= days * ( 3600 * 24 );

	int hours = sec / 3600;
	sec -= hours * 3600;

	int min = sec / 60;
	sec -= min * 60;

	if( weeks > 0 )
	{
		sprintf( timeString, "%dw, %dd, %d:%02d:%02d", weeks, days, hours, min, sec );
	}
	else if( days > 0 )
	{
		sprintf( timeString, "%dd, %d:%02d:%02d", days, hours, min, sec );
	}
	else
	{
		sprintf( timeString, "%d:%02d:%02d", hours, min, sec );
	}

	return timeString;
}

// return number of supported languages
int Sys_NumLangs()
{
	return numLanguages;
}

// get language name by index
const char* Sys_Lang( int idx )
{
	if( idx >= 0 && idx < numLanguages )
	{
		return sysLanguageNames[ idx ];
	}
	return "";
}

const char* Sys_DefaultLanguage()
{
	// sku breakdowns are as follows
	//  EFIGS	Digital
	//  EF  S	North America
	//   FIGS	EU
	//  E		UK
	// JE    	Japan

	// If japanese exists, default to japanese
	// else if english exists, defaults to english
	// otherwise, french

	if( !fileSystem->UsingResourceFiles() )
	{
		return ID_LANG_ENGLISH;
	}

	// GK: Prevent sys_lang to revert to english if is set manually
	if( idStr::Icmp( ID_LANG_ENGLISH, sys_lang.GetString() ) != 0 )
	{
		return sys_lang.GetString();
	}

	idStr fileName;

	//D3XP: Instead of just loading a single lang file for each language
	//we are going to load all files that begin with the language name
	//similar to the way pak files work. So you can place english001.lang
	//to add new strings to the english language dictionary
	idFileList* langFiles;
	langFiles = fileSystem->ListFilesTree( "strings", ".lang", true );

	idStrList langList = langFiles->GetList();

	// Loop through the list and filter
	idStrList currentLangList = langList;

	idStr temp;
	for( int i = 0; i < currentLangList.Num(); i++ )
	{
		temp = currentLangList[i];
		temp = temp.Right( temp.Length() - strlen( "strings/" ) );
		temp = temp.Left( temp.Length() - strlen( ".lang" ) );
		currentLangList[i] = temp;
	}

	if( currentLangList.Num() <= 0 )
	{
		// call it English if no lang files exist
		sys_lang.SetString( ID_LANG_ENGLISH );
	}
	else if( currentLangList.Num() == 1 )
	{
		sys_lang.SetString( currentLangList[0] );
	}
	else
	{
		if( currentLangList.Find( ID_LANG_ENGLISH ) )
		{
			sys_lang.SetString( ID_LANG_ENGLISH );
		}
		else if( currentLangList.Find( ID_LANG_JAPANESE ) )
		{
			sys_lang.SetString( ID_LANG_JAPANESE );
		}
		else if( currentLangList.Find( ID_LANG_FRENCH ) )
		{
			sys_lang.SetString( ID_LANG_FRENCH );
		}
		else if( currentLangList.Find( ID_LANG_GERMAN ) )
		{
			sys_lang.SetString( ID_LANG_GERMAN );
		}
		else if( currentLangList.Find( ID_LANG_ITALIAN ) )
		{
			sys_lang.SetString( ID_LANG_GERMAN );
		}
		else if( currentLangList.Find( ID_LANG_SPANISH ) )
		{
			sys_lang.SetString( ID_LANG_GERMAN );
		}
		else
		{
			sys_lang.SetString( currentLangList[0] );
		}
	}

	fileSystem->FreeFileList( langFiles );

	return sys_lang.GetString();// ID_LANG_ENGLISH;


}
