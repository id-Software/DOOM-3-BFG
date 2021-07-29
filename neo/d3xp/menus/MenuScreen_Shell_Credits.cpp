/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
#include "../Game_local.h"

static const int NUM_CREDIT_LINES = 16;

void idMenuScreen_Shell_Credits::SetupCreditList()
{

	class idRefreshCredits : public idSWFScriptFunction_RefCounted
	{
	public:
		idRefreshCredits( idMenuScreen_Shell_Credits* _screen ) :
			screen( _screen )
		{
		}

		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{

			if( screen == NULL )
			{
				return idSWFScriptVar();
			}

			screen->UpdateCredits();
			return idSWFScriptVar();
		}
	private:
		idMenuScreen_Shell_Credits* screen;
	};

	if( GetSWFObject() )
	{
		GetSWFObject()->SetGlobal( "updateCredits", new( TAG_SWF ) idRefreshCredits( this ) );
	}


	creditList.Clear();
	creditList.Append( creditInfo_t( 3,	"RBDOOM 3 BFG EDITION"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Project Owner"	) );
	creditList.Append( creditInfo_t( 0,	"Robert Beckebans - Renderer + Engine upgrades, Linux"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Programming"	) );
	creditList.Append( creditInfo_t( 0,	"Daniel Gibson - Tons of code cleanups, netcode++"	) );
	creditList.Append( creditInfo_t( 0,	"Jonathan Young - Bugfixes, misc improvements" ) );
	creditList.Append( creditInfo_t( 0,	"Felix Rueegg - Doomclassic Linux support" ) );
	creditList.Append( creditInfo_t( 0,	"Carl Kenner - Bink video support" ) );
	creditList.Append( creditInfo_t( 0,	"Pat Raynor - Compiler tools, QOL patches" ) );
	creditList.Append( creditInfo_t( 0,	"Biel B. de Luna - Intro skipping, QOL patches" ) );
	creditList.Append( creditInfo_t( 0,	"Dustin Land - Initial Vulkan backend" ) );
	creditList.Append( creditInfo_t( 0,	"Stephen Saunders - macOS support" ) );
	creditList.Append( creditInfo_t( 0,	"Justin Marshall - QOL patches from IcedHellFire branch" ) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 0,	"Thanks to everyone who provided valuable feedback on" ) );
	creditList.Append( creditInfo_t( 0,	"GitHub and the id Tech 4 Discord" ) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"DOOM 3 BFG EDITION"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 2,	"DEVELOPMENT TEAM"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Executive Producer"	) );
	creditList.Append( creditInfo_t( 0,	"Eric Webb"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Lead Programmer"	) );
	creditList.Append( creditInfo_t( 0,	"Brian Harris"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Lead Designer"	) );
	creditList.Append( creditInfo_t( 0,	"Jerry Keehan"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Programming"	) );
	creditList.Append( creditInfo_t( 0,	"Curtis Arink"	) );
	creditList.Append( creditInfo_t( 0,	"Robert Duffy"	) );
	creditList.Append( creditInfo_t( 0,	"Jeff Farrand"	) );
	creditList.Append( creditInfo_t( 0,	"Ryan Gerleve"	) );
	creditList.Append( creditInfo_t( 0,	"Billy Khan"	) );
	creditList.Append( creditInfo_t( 0,	"Gloria Kennickell"	) );
	creditList.Append( creditInfo_t( 0,	"Mike Maynard"	) );
	creditList.Append( creditInfo_t( 0,	"John Roberts"	) );
	creditList.Append( creditInfo_t( 0,	"Steven Serafin"	) );
	creditList.Append( creditInfo_t( 0,	"Jan Paul Van Waveren"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Design"	) );
	creditList.Append( creditInfo_t( 0,	"Steve Rescoe"	) );
	creditList.Append( creditInfo_t( 0,	"Chris Voss"	) );
	creditList.Append( creditInfo_t( 0,	"David Vargo"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Art"	) );
	creditList.Append( creditInfo_t( 0,	"Kevin Cloud"	) );
	creditList.Append( creditInfo_t( 0,	"Andy Chang"	) );
	creditList.Append( creditInfo_t( 0,	"Pat Duffy"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Audio"	) );
	creditList.Append( creditInfo_t( 0,	"Christian Antkow"	) );
	creditList.Append( creditInfo_t( 0,	"Chris Hite"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Support"	) );
	creditList.Append( creditInfo_t( 0,	"Roger Berrones"	) );
	creditList.Append( creditInfo_t( 0,	"Brad Bramlett"	) );
	creditList.Append( creditInfo_t( 0,	"John Casey"	) );
	creditList.Append( creditInfo_t( 0,	"Jeremy Cook"	) );
	creditList.Append( creditInfo_t( 0,	"Chris Hays"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Hooper"	) );
	creditList.Append( creditInfo_t( 0,	"Danny Keys"	) );
	creditList.Append( creditInfo_t( 0,	"Jason Kim"	) );
	creditList.Append( creditInfo_t( 0,	"Brian Kowalczyk"	) );
	creditList.Append( creditInfo_t( 0,	"Dustin Land"	) );
	creditList.Append( creditInfo_t( 0,	"Stephane Lebrun"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Nelson"	) );
	creditList.Append( creditInfo_t( 0,	"John Pollard"	) );
	creditList.Append( creditInfo_t( 0,	"Alan Rogers"	) );
	creditList.Append( creditInfo_t( 0,	"Jah Raphael"	) );
	creditList.Append( creditInfo_t( 0,	"Jarrod Showers"	) );
	creditList.Append( creditInfo_t( 0,	"Shale Williams"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Studio Director"	) );
	creditList.Append( creditInfo_t( 0,	"Tim Willits"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Studio President"	) );
	creditList.Append( creditInfo_t( 0,	"Todd Hollenshead"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Technical Director"	) );
	creditList.Append( creditInfo_t( 0,	"John Carmack"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Human Resources"	) );
	creditList.Append( creditInfo_t( 0,	"Carrie Barcroft"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"id Mom"	) );
	creditList.Append( creditInfo_t( 0,	"Donna Jackson"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"id Software IT"	) );
	creditList.Append( creditInfo_t( 0,	"Duncan Welch"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Cave"	) );
	creditList.Append( creditInfo_t( 0,	"Josh Shoemate"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Musick"	) );
	creditList.Append( creditInfo_t( 0,	"Alex Brandt"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"DOOM 3 DEVELOPED BY"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 2,	"id Software"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Technical Director "	) );
	creditList.Append( creditInfo_t( 0,	"John Carmack"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Lead Programmer"	) );
	creditList.Append( creditInfo_t( 0,	"Robert A. Duffy"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Lead Designer"	) );
	creditList.Append( creditInfo_t( 0,	"Tim Willits"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Lead Artist"	) );
	creditList.Append( creditInfo_t( 0,	"Kenneth Scott"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Animation"	) );
	creditList.Append( creditInfo_t( 0,	"James Houska"	) );
	creditList.Append( creditInfo_t( 0,	"Fredrik Nilsson"	) );
	creditList.Append( creditInfo_t( 0,	"Eric Webb"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Art"	) );
	creditList.Append( creditInfo_t( 0,	"Adrian Carmack"	) );
	creditList.Append( creditInfo_t( 0,	"Andy Chang"	) );
	creditList.Append( creditInfo_t( 0,	"Kevin Cloud"	) );
	creditList.Append( creditInfo_t( 0,	"Seneca Menard"	) );
	creditList.Append( creditInfo_t( 0,	"Patrick Thomas"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Design"	) );
	creditList.Append( creditInfo_t( 0,	"Mal Blackwell"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Hooper"	) );
	creditList.Append( creditInfo_t( 0,	"Jerry Keehan"	) );
	creditList.Append( creditInfo_t( 0,	"Steve Rescoe"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"UI Artist"	) );
	creditList.Append( creditInfo_t( 0,	"Pat Duffy"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Programming"	) );
	creditList.Append( creditInfo_t( 0,	"Timothee Besset"	) );
	creditList.Append( creditInfo_t( 0,	"Jim Dose"	) );
	creditList.Append( creditInfo_t( 0,	"Jan Paul van Waveren"	) );
	creditList.Append( creditInfo_t( 0,	"Jonathan Wright"	) );
	creditList.Append( creditInfo_t( 0,	"Brian Harris"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sound Design "	) );
	creditList.Append( creditInfo_t( 0,	"Christian Antkow"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Dir. Business Development"	) );
	creditList.Append( creditInfo_t( 0,	"Marty Stratton"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Development Assistant"	) );
	creditList.Append( creditInfo_t( 0,	"Eric Webb"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 2,	"3rd PARTY"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"DOOM 3: Ressurection of Evil Developed by"	) );
	creditList.Append( creditInfo_t( 0,	"Nerve Software"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Sound Design"	) );
	creditList.Append( creditInfo_t( 0,	"Ed Lima"	) );
	creditList.Append( creditInfo_t( 0,	"Danetracks, Inc."	) );
	creditList.Append( creditInfo_t( 0,	"Defacto Sound"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Theme for DOOM 3 Produced by"	) );
	creditList.Append( creditInfo_t( 0,	"Chris Vrenna"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Theme for DOOM 3 Composed by"	) );
	creditList.Append( creditInfo_t( 0,	"Clint Walsh"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Story and Dialog"	) );
	creditList.Append( creditInfo_t( 0,	"Matthew J. Costello"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Casting and Voice Direction"	) );
	creditList.Append( creditInfo_t( 0,	"Margaret Tang - Womb Music"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Voice Editorial and Post"	) );
	creditList.Append( creditInfo_t( 0,	"Rik Schaffer - Womb Music"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Voice Recording"	) );
	creditList.Append( creditInfo_t( 0,	"Womb Music"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Voice Performers"	) );
	creditList.Append( creditInfo_t( 0,	"Paul Eiding"	) );
	creditList.Append( creditInfo_t( 0,	"Jim Cummings"	) );
	creditList.Append( creditInfo_t( 0,	"Liam O'Brien"	) );
	creditList.Append( creditInfo_t( 0,	"Nicholas Guest"	) );
	creditList.Append( creditInfo_t( 0,	"Vanessa Marshall"	) );
	creditList.Append( creditInfo_t( 0,	"Neill Ross"	) );
	creditList.Append( creditInfo_t( 0,	"Phillip Clarke"	) );
	creditList.Append( creditInfo_t( 0,	"Andy Chanley"	) );
	creditList.Append( creditInfo_t( 0,	"Charles Dennis"	) );
	creditList.Append( creditInfo_t( 0,	"Grey Delisle"	) );
	creditList.Append( creditInfo_t( 0,	"Jennifer Hale"	) );
	creditList.Append( creditInfo_t( 0,	"Grant Albrecht"	) );
	creditList.Append( creditInfo_t( 0,	"Dee Baker"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Bell"	) );
	creditList.Append( creditInfo_t( 0,	"Steve Blum"	) );
	creditList.Append( creditInfo_t( 0,	"S. Scott Bullock"	) );
	creditList.Append( creditInfo_t( 0,	"Cam Clarke"	) );
	creditList.Append( creditInfo_t( 0,	"Robin Atkin Downes"	) );
	creditList.Append( creditInfo_t( 0,	"Keith Ferguson"	) );
	creditList.Append( creditInfo_t( 0,	"Jay Gordon"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Gough"	) );
	creditList.Append( creditInfo_t( 0,	"Bill Harper"	) );
	creditList.Append( creditInfo_t( 0,	"Nick Jameson"	) );
	creditList.Append( creditInfo_t( 0,	"David Kaye"	) );
	creditList.Append( creditInfo_t( 0,	"Phil La Marr"	) );
	creditList.Append( creditInfo_t( 0,	"Scott Menville"	) );
	creditList.Append( creditInfo_t( 0,	"Jim Meskimen"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Morton"	) );
	creditList.Append( creditInfo_t( 0,	"Daran Norris"	) );
	creditList.Append( creditInfo_t( 0,	"Rob Paulsen"	) );
	creditList.Append( creditInfo_t( 0,	"Phil Proctor"	) );
	creditList.Append( creditInfo_t( 0,	"Rino Romoano"	) );
	creditList.Append( creditInfo_t( 0,	"Andre Sogliuzzo"	) );
	creditList.Append( creditInfo_t( 0,	"Jim Ward"	) );
	creditList.Append( creditInfo_t( 0,	"Wally Wingert"	) );
	creditList.Append( creditInfo_t( 0,	"Edward Yin"	) );
	creditList.Append( creditInfo_t( 0,	"Keone Young"	) );
	creditList.Append( creditInfo_t( 0,	"Ryun Yu"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional User Interface"	) );
	creditList.Append( creditInfo_t( 0,	"Double-Action Design"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Motion Capture Services"	) );
	creditList.Append( creditInfo_t( 0,	"Janimation"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"UAC Promotional Videos"	) );
	creditList.Append( creditInfo_t( 0,	"Six Foot Studios"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Multiplayer Design"	) );
	creditList.Append( creditInfo_t( 0,	"Splash Damage, Ltd."	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional Programming"	) );
	creditList.Append( creditInfo_t( 0,	"Graeme Devine"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Localization Services"	) );
	creditList.Append( creditInfo_t( 0,	"Synthesis"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"BETHESDA SOFTWORKS"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Senior Producer"	) );
	creditList.Append( creditInfo_t( 0,	"Laffy Taylor"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sr. Producer, Submissions"	) );
	creditList.Append( creditInfo_t( 0,	"Timothy Beggs"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"VP, Product Development"	) );
	creditList.Append( creditInfo_t( 0,	"Todd Vaughn"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"External Technical Director"	) );
	creditList.Append( creditInfo_t( 0,	"Jonathan Williams"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"President"	) );
	creditList.Append( creditInfo_t( 0,	"Vlatko Andonov"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"VP, Public Relations and Marketing"	) );
	creditList.Append( creditInfo_t( 0,	"Pete Hines"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Director of Global Marketing"	) );
	creditList.Append( creditInfo_t( 0,	"Steve Perkins"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Director of Global PR"	) );
	creditList.Append( creditInfo_t( 0,	"Tracey Thompson"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Senior Brand Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Rakhi Gupta"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Associate Brand Manager"	) );
	creditList.Append( creditInfo_t( 0,	"David Clayman"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Event and Trade Show Director"	) );
	creditList.Append( creditInfo_t( 0,	"Henry Mobley"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Senior Community Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Matthew Grandstaff"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Community Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Nick Breckon"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Senior PR Coordinator"	) );
	creditList.Append( creditInfo_t( 0,	"Angela Ramsey-Chapman"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Marketing Assistant"	) );
	creditList.Append( creditInfo_t( 0,	"Jenny McWhorter"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Video Production"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Killmon"	) );
	creditList.Append( creditInfo_t( 0,	"Sal Goldenburg"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Graphic Design"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Wagner"	) );
	creditList.Append( creditInfo_t( 0,	"Lindsay Westcott"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"VP, Sales"	) );
	creditList.Append( creditInfo_t( 0,	"Ron Seger"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Operations Director"	) );
	creditList.Append( creditInfo_t( 0,	"Todd Curtis"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales and Operations Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Jill Bralove"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Account Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Michelle Ferrara"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Regional Sales Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Donnellan"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Channel Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Michelle Burgess"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Assistants"	) );
	creditList.Append( creditInfo_t( 0,	"Sara Simpson"	) );
	creditList.Append( creditInfo_t( 0,	"Jason Snead"	) );
	creditList.Append( creditInfo_t( 0,	"Jessica Williams"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Operations Assistant"	) );
	creditList.Append( creditInfo_t( 0,	"Scott Mills"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Director of Quality Assurance"	) );
	creditList.Append( creditInfo_t( 0,	"Darren Manes"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Quality Assurance Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Rob Gray"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Quality Assurance Leads"	) );
	creditList.Append( creditInfo_t( 0,	"Terry Dunn"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Weil"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Quality Assurance"	) );
	creditList.Append( creditInfo_t( 0,	"Dan Silva"	) );
	creditList.Append( creditInfo_t( 0,	"Alan Webb"	) );
	creditList.Append( creditInfo_t( 0,	"Cory Andrews"	) );
	creditList.Append( creditInfo_t( 0,	"George Churchill"	) );
	creditList.Append( creditInfo_t( 0,	"Jonathan DeVriendt"	) );
	creditList.Append( creditInfo_t( 0,	"Spencer Gottlieb"	) );
	creditList.Append( creditInfo_t( 0,	"Gary Powell"	) );
	creditList.Append( creditInfo_t( 0,	"Samuel Papke"	) );
	creditList.Append( creditInfo_t( 0,	"Amanda Sheehan"	) );
	creditList.Append( creditInfo_t( 0,	"Philip Spangrud"	) );
	creditList.Append( creditInfo_t( 0,	"Larry Waldman"	) );
	creditList.Append( creditInfo_t( 0,	"Donald Anderson"	) );
	creditList.Append( creditInfo_t( 0,	"James Audet"	) );
	creditList.Append( creditInfo_t( 0,	"Hunter Calvert"	) );
	creditList.Append( creditInfo_t( 0,	"Max Cameron"	) );
	creditList.Append( creditInfo_t( 0,	"Donald Harris"	) );
	creditList.Append( creditInfo_t( 0,	"Peter Garcia"	) );
	creditList.Append( creditInfo_t( 0,	"Joseph Lopatta"	) );
	creditList.Append( creditInfo_t( 0,	"Scott LoPresti"	) );
	creditList.Append( creditInfo_t( 0,	"Collin Mackett"	) );
	creditList.Append( creditInfo_t( 0,	"Gerard Nagy"	) );
	creditList.Append( creditInfo_t( 0,	"William Pegus"	) );
	creditList.Append( creditInfo_t( 0,	"Angela Rupinen"	) );
	creditList.Append( creditInfo_t( 0,	"Gabriel Scaringello"	) );
	creditList.Append( creditInfo_t( 0,	"Drew Slotkin"	) );
	creditList.Append( creditInfo_t( 0,	"Kyle Wallace"	) );
	creditList.Append( creditInfo_t( 0,	"Patrick Walsh"	) );
	creditList.Append( creditInfo_t( 0,	"John Welkner"	) );
	creditList.Append( creditInfo_t( 0,	"Jason Wilkin"	) );
	creditList.Append( creditInfo_t( 0,	"John Benoit"	) );
	creditList.Append( creditInfo_t( 0,	"Jacob Clayman"	) );
	creditList.Append( creditInfo_t( 0,	"Colin McInerney"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Additional QA"	) );
	creditList.Append( creditInfo_t( 0,	"James Costantino"	) );
	creditList.Append( creditInfo_t( 0,	"Chris Krietz"	) );
	creditList.Append( creditInfo_t( 0,	"Joseph Mueller"	) );
	creditList.Append( creditInfo_t( 0,	"Andrew Scharf"	) );
	creditList.Append( creditInfo_t( 0,	"Jen Tonon"	) );
	creditList.Append( creditInfo_t( 0,	"Justin McSweeney"	) );
	creditList.Append( creditInfo_t( 0,	"Sean Palomino"	) );
	creditList.Append( creditInfo_t( 0,	"Erica Stead"	) );
	creditList.Append( creditInfo_t( 0,	"Wil Cookman"	) );
	creditList.Append( creditInfo_t( 0,	"Garrett Hohl"	) );
	creditList.Append( creditInfo_t( 0,	"Tim Hartgrave"	) );
	creditList.Append( creditInfo_t( 0,	"Heath Hollenshead"	) );
	creditList.Append( creditInfo_t( 0,	"Brandon Korbel"	) );
	creditList.Append( creditInfo_t( 0,	"Daniel Korecki"	) );
	creditList.Append( creditInfo_t( 0,	"Max Morrison"	) );
	creditList.Append( creditInfo_t( 0,	"Doug Ransley"	) );
	creditList.Append( creditInfo_t( 0,	"Mauricio Rivera"	) );
	creditList.Append( creditInfo_t( 0,	"Aaron Walsh"	) );
	creditList.Append( creditInfo_t( 0,	"Daniel Wathen"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Special Thanks"	) );
	creditList.Append( creditInfo_t( 0,	"Jason Bergman"	) );
	creditList.Append( creditInfo_t( 0,	"Darren Chukitus"	) );
	creditList.Append( creditInfo_t( 0,	"Matt Dickenson"	) );
	creditList.Append( creditInfo_t( 0,	"Will Noble"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX MEDIA"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Chairman & CEO"	) );
	creditList.Append( creditInfo_t( 0,	"Robert Altman"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"President"	) );
	creditList.Append( creditInfo_t( 0,	"Ernie Del"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"EVP & COO"	) );
	creditList.Append( creditInfo_t( 0,	"Jamie Leder"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"EVP & CFO"	) );
	creditList.Append( creditInfo_t( 0,	"Cindy Tallent"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"EVP Legal & Secretary"	) );
	creditList.Append( creditInfo_t( 0,	"Grif Lesher"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"SVP Finance & Controller"	) );
	creditList.Append( creditInfo_t( 0,	"Denise Kidd"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Legal Lead"	) );
	creditList.Append( creditInfo_t( 0,	"Joshua Gillespie"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Legal"	) );
	creditList.Append( creditInfo_t( 0,	"Diana Bender"	) );
	creditList.Append( creditInfo_t( 0,	"Adam Carter"	) );
	creditList.Append( creditInfo_t( 0,	"Candice Garner-Groves"	) );
	creditList.Append( creditInfo_t( 0,	"Marcia Mitnick"	) );
	creditList.Append( creditInfo_t( 0,	"Amy Yeung"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"VP, Information Technology"	) );
	creditList.Append( creditInfo_t( 0,	"Steve Bloom"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Information Technology"	) );
	creditList.Append( creditInfo_t( 0,	"Rob Havlovick"	) );
	creditList.Append( creditInfo_t( 0,	"Nicholas Lea"	) );
	creditList.Append( creditInfo_t( 0,	"Drew McCartney"	) );
	creditList.Append( creditInfo_t( 0,	"Josh Mosby"	) );
	creditList.Append( creditInfo_t( 0,	"Joseph Owens"	) );
	creditList.Append( creditInfo_t( 0,	"Paul Tuttle"	) );
	creditList.Append( creditInfo_t( 0,	"Keelian Wardle"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Director of Global HR"	) );
	creditList.Append( creditInfo_t( 0,	"Tammy Boyd-Shumway"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Human Resources"	) );
	creditList.Append( creditInfo_t( 0,	"Michelle Cool"	) );
	creditList.Append( creditInfo_t( 0,	"Andrea Glinski"	) );
	creditList.Append( creditInfo_t( 0,	"Katrina Lang"	) );
	creditList.Append( creditInfo_t( 0,	"Valery Ridore"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Administration"	) );
	creditList.Append( creditInfo_t( 0,	"Melissa Ayala"	) );
	creditList.Append( creditInfo_t( 0,	"Brittany Bezawada-Joseph"	) );
	creditList.Append( creditInfo_t( 0,	"Rosanna Campanile  "	) );
	creditList.Append( creditInfo_t( 0,	"Katherine Edwards"	) );
	creditList.Append( creditInfo_t( 0,	"Douglas Fredrick    "	) );
	creditList.Append( creditInfo_t( 0,	"Jon Freund"	) );
	creditList.Append( creditInfo_t( 0,	"Ken Garcia "	) );
	creditList.Append( creditInfo_t( 0,	"Gerard Garnica"	) );
	creditList.Append( creditInfo_t( 0,	"Betty Kouatelay"	) );
	creditList.Append( creditInfo_t( 0,	"Ho Joong Lee"	) );
	creditList.Append( creditInfo_t( 0,	"Barb Manning"	) );
	creditList.Append( creditInfo_t( 0,	"Stephane Marquis"	) );
	creditList.Append( creditInfo_t( 0,	"Michael Masciola"	) );
	creditList.Append( creditInfo_t( 0,	"Tanuja Mistry"	) );
	creditList.Append( creditInfo_t( 0,	"Rissa Monzano"	) );
	creditList.Append( creditInfo_t( 0,	"Patrick Nolan "	) );
	creditList.Append( creditInfo_t( 0,	"Patti Pulupa"	) );
	creditList.Append( creditInfo_t( 0,	"Dave Rasmussen "	) );
	creditList.Append( creditInfo_t( 0,	"Heather Spurrier"	) );
	creditList.Append( creditInfo_t( 0,	"Claudia Umana"	) );
	creditList.Append( creditInfo_t( 0,	"Melissa Washabaugh"	) );
	creditList.Append( creditInfo_t( 0,	"Eric Weis"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Administrative Assistants"	) );
	creditList.Append( creditInfo_t( 0,	"Bernice Guice"	) );
	creditList.Append( creditInfo_t( 0,	"Paula Kasey"	) );
	creditList.Append( creditInfo_t( 0,	"Shana Reed"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Executive Chef"	) );
	creditList.Append( creditInfo_t( 0,	"Kenny McDonald"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX EUROPE"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Managing Director"	) );
	creditList.Append( creditInfo_t( 0,	"Sean Brennan"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Marketing & PR Director"	) );
	creditList.Append( creditInfo_t( 0,	"Sarah Seaby"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Sales Director"	) );
	creditList.Append( creditInfo_t( 0,	"Paul Oughton"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Director"	) );
	creditList.Append( creditInfo_t( 0,	"Greg Baverstock"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Finance Director"	) );
	creditList.Append( creditInfo_t( 0,	"Robert Ford"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Brand Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Thach Quach"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Brand Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Alex Price"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European PR Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Alistair Hatch"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"European Assistant PR Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Nicholas Heller"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"International Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Rosemarie Dalton"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Manager Trade Shows/Events"	) );
	creditList.Append( creditInfo_t( 0,	"Gareth Swann"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"UK Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Gregory Weller"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"UK Sales Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Gethyn Deakins"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Creative Services Artist"	) );
	creditList.Append( creditInfo_t( 0,	"Morgan Gibbons"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Legal Counsel"	) );
	creditList.Append( creditInfo_t( 0,	"Adam Carter"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Localisation Director"	) );
	creditList.Append( creditInfo_t( 0,	"Harald Simon"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Localisation Project Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Ruth Granados Garcia"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Administrator"	) );
	creditList.Append( creditInfo_t( 0,	"Heather Clarke"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Operations Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Isabelle Midrouillet"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Operations Coordinator"	) );
	creditList.Append( creditInfo_t( 0,	"David Gordon"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Financial Controller"	) );
	creditList.Append( creditInfo_t( 0,	"Paul New"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Accounts Assistant"	) );
	creditList.Append( creditInfo_t( 0,	"Charlotte Ovens"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Contracts Administrator/Paralegal"	) );
	creditList.Append( creditInfo_t( 0,	"Katie Brooks"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"IT Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Joseph Owens"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Office Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Angela Clement"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX FRANCE"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"General Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Julie Chalmette"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Yvan Rault"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Key Account Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Gaelle Gombert"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Trade Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Laurent Chatain"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Geraldine Mazot"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"PR Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Jerome Firon"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Finance Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Cecile De Freitas"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Finance Assistant"	) );
	creditList.Append( creditInfo_t( 0,	"Adeline Nonis"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX GERMANY"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Managing Director"	) );
	creditList.Append( creditInfo_t( 0,	"Frank Matzke"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Director"	) );
	creditList.Append( creditInfo_t( 0,	"Thomas Huber"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Head of Marketing & PR"	) );
	creditList.Append( creditInfo_t( 0,	"Marcel Jung"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Product Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Stefan Dettmering"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"PR Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Peter Langhofer"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Trade Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Andrea Reuth"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Key Account Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Juergen Pahl"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Finance Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Joern Hoehling"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales and Office Administrator"	) );
	creditList.Append( creditInfo_t( 0,	"Christiane Jauss"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX BENELUX"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"General Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Menno Eijck"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Stefan Koppers"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Marketing Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Jurgen Stirnweis"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"PR Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Maikel van Dijk"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales and Marketing Administrator"	) );
	creditList.Append( creditInfo_t( 0,	"Pam van Griethuysen"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 3,	"ZENIMAX ASIA"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"General Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Tetsu Takahashi"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Localization Producer "	) );
	creditList.Append( creditInfo_t( 0,	"Kei Iwamoto"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Associate Producer "	) );
	creditList.Append( creditInfo_t( 0,	"Takayuki Tanaka"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Product Manager "	) );
	creditList.Append( creditInfo_t( 0,	"Seigen Ko"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Localization Programmer"	) );
	creditList.Append( creditInfo_t( 0,	"Masayuki Nagahashi"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Marketing and PR Manager "	) );
	creditList.Append( creditInfo_t( 0,	"Eiichi Yaji"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Sales Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Hiroaki Yanagiguchi"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Office Manager"	) );
	creditList.Append( creditInfo_t( 0,	"Myongsuk Rim"	) );
	creditList.Append( creditInfo_t() );
	creditList.Append( creditInfo_t( 1,	"Web Director"	) );
	creditList.Append( creditInfo_t( 0,	"Tanaka Keisuke"	) );
	creditList.Append( creditInfo_t( 0,	"Kosuke Fujita"	) );
};

/*
========================
idMenuScreen_Shell_Credits::Initialize
========================
*/
void idMenuScreen_Shell_Credits::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuCredits" );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_02305" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	SetupCreditList();
}

/*
========================
idMenuScreen_Shell_Credits::Update
========================
*/
void idMenuScreen_Shell_Credits::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();

			idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
			bool complete = false;
			if( shell != NULL )
			{
				complete = shell->GetGameComplete();
			}

			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			if( !complete )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_00395";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
			}
			else
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_swf_continue";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
			}
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_02218" );
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_Credits::ShowScreen
========================
*/
void idMenuScreen_Shell_Credits::ShowScreen( const mainMenuTransition_t transitionType )
{

	if( menuData != NULL )
	{
		idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
		bool complete = false;
		if( shell != NULL )
		{
			complete = shell->GetGameComplete();
		}

		if( complete )
		{
			menuData->PlaySound( GUI_SOUND_MUSIC );
		}
	}

	idMenuScreen::ShowScreen( transitionType );
	creditIndex = 0;
	UpdateCredits();
}

/*
========================
idMenuScreen_Shell_Credits::HideScreen
========================
*/
void idMenuScreen_Shell_Credits::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Credits::HandleAction
========================
*/
bool idMenuScreen_Shell_Credits::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_CREDITS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{

			idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
			bool complete = false;
			if( shell != NULL )
			{
				complete = shell->GetGameComplete();
			}

			if( complete )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
			}
			else
			{
				menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_Shell_Credits::UpdateCredits
========================
*/
void idMenuScreen_Shell_Credits::UpdateCredits()
{

	if( menuData == NULL || GetSWFObject() == NULL )
	{
		return;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_CREDITS && menuData->NextScreen() != SHELL_AREA_CREDITS )
	{
		return;
	}

	if( creditIndex >= creditList.Num() + NUM_CREDIT_LINES )
	{
		idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
		bool complete = false;
		if( shell != NULL )
		{
			complete = shell->GetGameComplete();
		}

		if( complete )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		}
		else
		{
			menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
		}
		return;
	}

	idSWFScriptObject* options = GetSWFObject()->GetRootObject().GetNestedObj( "menuCredits", "info", "options" );
	if( options != NULL )
	{
		for( int i = 15; i >= 0; --i )
		{
			int curIndex = creditIndex - i;
			idSWFTextInstance* heading = options->GetNestedText( va( "item%d", 15 - i ), "heading" );
			idSWFTextInstance* subHeading = options->GetNestedText( va( "item%d", 15 - i ), "subHeading" );
			idSWFTextInstance* title = options->GetNestedText( va( "item%d", 15 - i ), "title" );
			idSWFTextInstance* txtEntry = options->GetNestedText( va( "item%d", 15 - i ), "entry" );

			if( curIndex >= 0 && curIndex < creditList.Num() )
			{

				int type = creditList[ curIndex ].type;
				idStr entry = creditList[ curIndex ].entry;

				if( heading )
				{
					heading->SetText( type == 3 ? entry : "" );
					heading->SetStrokeInfo( true );
				}

				if( subHeading )
				{
					subHeading->SetText( type == 2 ? entry : "" );
					subHeading->SetStrokeInfo( true );
				}

				if( title )
				{
					title->SetText( type == 1 ? entry : "" );
					title->SetStrokeInfo( true );
				}

				if( txtEntry )
				{
					txtEntry->SetText( type == 0 ? entry : "" );
					txtEntry->SetStrokeInfo( true );
				}

			}
			else
			{

				if( heading )
				{
					heading->SetText( "" );
				}

				if( subHeading )
				{
					subHeading->SetText( "" );
				}

				if( txtEntry )
				{
					txtEntry->SetText( "" );
				}

				if( title )
				{
					title->SetText( "" );
				}

			}
		}
		if( options->GetSprite() )
		{
			options->GetSprite()->PlayFrame( "roll" );
		}
	}

	creditIndex++;
}