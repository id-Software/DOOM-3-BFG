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

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "GameBearShootWindow.h"


#define BEAR_GRAVITY 240
#define BEAR_SIZE 24.f
#define BEAR_SHRINK_TIME 2000.f

#define MAX_WINDFORCE 100.f

idCVar bearTurretAngle( "bearTurretAngle", "0", CVAR_FLOAT, "" );
idCVar bearTurretForce( "bearTurretForce", "200", CVAR_FLOAT, "" );

/*
*****************************************************************************
* BSEntity
****************************************************************************
*/
BSEntity::BSEntity( idGameBearShootWindow* _game )
{
	game = _game;
	visible = true;

	entColor = colorWhite;
	materialName = "";
	material = NULL;
	width = height = 8;
	rotation = 0.f;
	rotationSpeed = 0.f;
	fadeIn = false;
	fadeOut = false;

	position.Zero();
	velocity.Zero();
}

BSEntity::~BSEntity()
{
}

/*
======================
BSEntity::WriteToSaveGame
======================
*/
void BSEntity::WriteToSaveGame( idFile* savefile )
{

	game->WriteSaveGameString( materialName, savefile );

	savefile->Write( &width, sizeof( width ) );
	savefile->Write( &height, sizeof( height ) );
	savefile->Write( &visible, sizeof( visible ) );

	savefile->Write( &entColor, sizeof( entColor ) );
	savefile->Write( &position, sizeof( position ) );
	savefile->Write( &rotation, sizeof( rotation ) );
	savefile->Write( &rotationSpeed, sizeof( rotationSpeed ) );
	savefile->Write( &velocity, sizeof( velocity ) );

	savefile->Write( &fadeIn, sizeof( fadeIn ) );
	savefile->Write( &fadeOut, sizeof( fadeOut ) );
}

/*
======================
BSEntity::ReadFromSaveGame
======================
*/
void BSEntity::ReadFromSaveGame( idFile* savefile, idGameBearShootWindow* _game )
{
	game = _game;

	game->ReadSaveGameString( materialName, savefile );
	SetMaterial( materialName );

	savefile->Read( &width, sizeof( width ) );
	savefile->Read( &height, sizeof( height ) );
	savefile->Read( &visible, sizeof( visible ) );

	savefile->Read( &entColor, sizeof( entColor ) );
	savefile->Read( &position, sizeof( position ) );
	savefile->Read( &rotation, sizeof( rotation ) );
	savefile->Read( &rotationSpeed, sizeof( rotationSpeed ) );
	savefile->Read( &velocity, sizeof( velocity ) );

	savefile->Read( &fadeIn, sizeof( fadeIn ) );
	savefile->Read( &fadeOut, sizeof( fadeOut ) );
}

/*
======================
BSEntity::SetMaterial
======================
*/
void BSEntity::SetMaterial( const char* name )
{
	materialName = name;
	material = declManager->FindMaterial( name );
	material->SetSort( SS_GUI );
}

/*
======================
BSEntity::SetSize
======================
*/
void BSEntity::SetSize( float _width, float _height )
{
	width = _width;
	height = _height;
}

/*
======================
BSEntity::SetVisible
======================
*/
void BSEntity::SetVisible( bool isVisible )
{
	visible = isVisible;
}

/*
======================
BSEntity::Update
======================
*/
void BSEntity::Update( float timeslice )
{

	if( !visible )
	{
		return;
	}

	// Fades
	if( fadeIn && entColor.w < 1.f )
	{
		entColor.w += 1 * timeslice;
		if( entColor.w >= 1.f )
		{
			entColor.w = 1.f;
			fadeIn = false;
		}
	}
	if( fadeOut && entColor.w > 0.f )
	{
		entColor.w -= 1 * timeslice;
		if( entColor.w <= 0.f )
		{
			entColor.w = 0.f;
			fadeOut = false;
		}
	}

	// Move the entity
	position += velocity * timeslice;

	// Rotate Entity
	rotation += rotationSpeed * timeslice;
}

/*
======================
BSEntity::Draw
======================
*/
void BSEntity::Draw()
{
	if( visible )
	{
		dc->DrawMaterialRotated( position.x, position.y, width, height, material, entColor, 1.0f, 1.0f, DEG2RAD( rotation ) );
	}
}

/*
*****************************************************************************
* idGameBearShootWindow
****************************************************************************
*/

idGameBearShootWindow::idGameBearShootWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idGameBearShootWindow::~idGameBearShootWindow()
{
	entities.DeleteContents( true );
}

/*
=============================
idGameBearShootWindow::WriteToSaveGame
=============================
*/
void idGameBearShootWindow::WriteToSaveGame( idFile* savefile )
{
	idWindow::WriteToSaveGame( savefile );

	gamerunning.WriteToSaveGame( savefile );
	onFire.WriteToSaveGame( savefile );
	onContinue.WriteToSaveGame( savefile );
	onNewGame.WriteToSaveGame( savefile );

	savefile->Write( &timeSlice, sizeof( timeSlice ) );
	savefile->Write( &timeRemaining, sizeof( timeRemaining ) );
	savefile->Write( &gameOver, sizeof( gameOver ) );

	savefile->Write( &currentLevel, sizeof( currentLevel ) );
	savefile->Write( &goalsHit, sizeof( goalsHit ) );
	savefile->Write( &updateScore, sizeof( updateScore ) );
	savefile->Write( &bearHitTarget, sizeof( bearHitTarget ) );

	savefile->Write( &bearScale, sizeof( bearScale ) );
	savefile->Write( &bearIsShrinking, sizeof( bearIsShrinking ) );
	savefile->Write( &bearShrinkStartTime, sizeof( bearShrinkStartTime ) );

	savefile->Write( &turretAngle, sizeof( turretAngle ) );
	savefile->Write( &turretForce, sizeof( turretForce ) );

	savefile->Write( &windForce, sizeof( windForce ) );
	savefile->Write( &windUpdateTime, sizeof( windUpdateTime ) );

	int numberOfEnts = entities.Num();
	savefile->Write( &numberOfEnts, sizeof( numberOfEnts ) );

	for( int i = 0; i < numberOfEnts; i++ )
	{
		entities[i]->WriteToSaveGame( savefile );
	}

	int index;
	index = entities.FindIndex( turret );
	savefile->Write( &index, sizeof( index ) );
	index = entities.FindIndex( bear );
	savefile->Write( &index, sizeof( index ) );
	index = entities.FindIndex( helicopter );
	savefile->Write( &index, sizeof( index ) );
	index = entities.FindIndex( goal );
	savefile->Write( &index, sizeof( index ) );
	index = entities.FindIndex( wind );
	savefile->Write( &index, sizeof( index ) );
	index = entities.FindIndex( gunblast );
	savefile->Write( &index, sizeof( index ) );
}

/*
=============================
idGameBearShootWindow::ReadFromSaveGame
=============================
*/
void idGameBearShootWindow::ReadFromSaveGame( idFile* savefile )
{
	idWindow::ReadFromSaveGame( savefile );

	// Remove all existing entities
	entities.DeleteContents( true );

	gamerunning.ReadFromSaveGame( savefile );
	onFire.ReadFromSaveGame( savefile );
	onContinue.ReadFromSaveGame( savefile );
	onNewGame.ReadFromSaveGame( savefile );

	savefile->Read( &timeSlice, sizeof( timeSlice ) );
	savefile->Read( &timeRemaining, sizeof( timeRemaining ) );
	savefile->Read( &gameOver, sizeof( gameOver ) );

	savefile->Read( &currentLevel, sizeof( currentLevel ) );
	savefile->Read( &goalsHit, sizeof( goalsHit ) );
	savefile->Read( &updateScore, sizeof( updateScore ) );
	savefile->Read( &bearHitTarget, sizeof( bearHitTarget ) );

	savefile->Read( &bearScale, sizeof( bearScale ) );
	savefile->Read( &bearIsShrinking, sizeof( bearIsShrinking ) );
	savefile->Read( &bearShrinkStartTime, sizeof( bearShrinkStartTime ) );

	savefile->Read( &turretAngle, sizeof( turretAngle ) );
	savefile->Read( &turretForce, sizeof( turretForce ) );

	savefile->Read( &windForce, sizeof( windForce ) );
	savefile->Read( &windUpdateTime, sizeof( windUpdateTime ) );

	int numberOfEnts;
	savefile->Read( &numberOfEnts, sizeof( numberOfEnts ) );

	for( int i = 0; i < numberOfEnts; i++ )
	{
		BSEntity* ent;

		ent = new( TAG_OLD_UI ) BSEntity( this );
		ent->ReadFromSaveGame( savefile, this );
		entities.Append( ent );
	}

	int index;
	savefile->Read( &index, sizeof( index ) );
	turret = entities[index];
	savefile->Read( &index, sizeof( index ) );
	bear = entities[index];
	savefile->Read( &index, sizeof( index ) );
	helicopter = entities[index];
	savefile->Read( &index, sizeof( index ) );
	goal = entities[index];
	savefile->Read( &index, sizeof( index ) );
	wind = entities[index];
	savefile->Read( &index, sizeof( index ) );
	gunblast = entities[index];
}

/*
=============================
idGameBearShootWindow::ResetGameState
=============================
*/
void idGameBearShootWindow::ResetGameState()
{
	gamerunning = false;
	gameOver = false;
	onFire = false;
	onContinue = false;
	onNewGame = false;

	// Game moves forward 16 milliseconds every frame
	timeSlice = 0.016f;
	timeRemaining = 60.f;
	goalsHit = 0;
	updateScore = false;
	bearHitTarget = false;
	currentLevel = 1;
	turretAngle = 0.f;
	turretForce = 200.f;
	windForce = 0.f;
	windUpdateTime = 0;

	bearIsShrinking = false;
	bearShrinkStartTime = 0;
	bearScale = 1.f;
}

/*
=============================
idGameBearShootWindow::CommonInit
=============================
*/
void idGameBearShootWindow::CommonInit()
{
	BSEntity* 			ent;

	// Precache sounds
	declManager->FindSound( "arcade_beargroan" );
	declManager->FindSound( "arcade_sargeshoot" );
	declManager->FindSound( "arcade_balloonpop" );
	declManager->FindSound( "arcade_levelcomplete1" );

	// Precache dynamically used materials
	declManager->FindMaterial( "game/bearshoot/helicopter_broken" );
	declManager->FindMaterial( "game/bearshoot/goal_dead" );
	declManager->FindMaterial( "game/bearshoot/gun_blast" );

	ResetGameState();

	ent = new( TAG_OLD_UI ) BSEntity( this );
	turret = ent;
	ent->SetMaterial( "game/bearshoot/turret" );
	ent->SetSize( 272, 144 );
	ent->position.x = -44;
	ent->position.y = 260;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	ent->SetMaterial( "game/bearshoot/turret_base" );
	ent->SetSize( 144, 160 );
	ent->position.x = 16;
	ent->position.y = 280;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	bear = ent;
	ent->SetMaterial( "game/bearshoot/bear" );
	ent->SetSize( BEAR_SIZE, BEAR_SIZE );
	ent->SetVisible( false );
	ent->position.x = 0;
	ent->position.y = 0;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	helicopter = ent;
	ent->SetMaterial( "game/bearshoot/helicopter" );
	ent->SetSize( 64, 64 );
	ent->position.x = 550;
	ent->position.y = 100;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	goal = ent;
	ent->SetMaterial( "game/bearshoot/goal" );
	ent->SetSize( 64, 64 );
	ent->position.x = 550;
	ent->position.y = 164;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	wind = ent;
	ent->SetMaterial( "game/bearshoot/wind" );
	ent->SetSize( 100, 40 );
	ent->position.x = 500;
	ent->position.y = 430;
	entities.Append( ent );

	ent = new( TAG_OLD_UI ) BSEntity( this );
	gunblast = ent;
	ent->SetMaterial( "game/bearshoot/gun_blast" );
	ent->SetSize( 64, 64 );
	ent->SetVisible( false );
	entities.Append( ent );
}

/*
=============================
idGameBearShootWindow::HandleEvent
=============================
*/
const char* idGameBearShootWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{
	int key = event->evValue;

	// need to call this to allow proper focus and capturing on embedded children
	const char* ret = idWindow::HandleEvent( event, updateVisuals );

	if( event->evType == SE_KEY )
	{

		if( !event->evValue2 )
		{
			return ret;
		}
		if( key == K_MOUSE1 )
		{
			// Mouse was clicked
		}
		else
		{
			return ret;
		}
	}

	return ret;
}

/*
=============================
idGameBearShootWindow::ParseInternalVar
=============================
*/
bool idGameBearShootWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{
	if( idStr::Icmp( _name, "gamerunning" ) == 0 )
	{
		gamerunning = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "onFire" ) == 0 )
	{
		onFire = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "onContinue" ) == 0 )
	{
		onContinue = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "onNewGame" ) == 0 )
	{
		onNewGame = src->ParseBool();
		return true;
	}

	return idWindow::ParseInternalVar( _name, src );
}

/*
=============================
idGameBearShootWindow::GetWinVarByName
=============================
*/
idWinVar* idGameBearShootWindow::GetWinVarByName( const char* _name, bool winLookup, drawWin_t** owner )
{
	idWinVar* retVar = NULL;

	if( idStr::Icmp( _name, "gamerunning" ) == 0 )
	{
		retVar = &gamerunning;
	}
	else 	if( idStr::Icmp( _name, "onFire" ) == 0 )
	{
		retVar = &onFire;
	}
	else 	if( idStr::Icmp( _name, "onContinue" ) == 0 )
	{
		retVar = &onContinue;
	}
	else 	if( idStr::Icmp( _name, "onNewGame" ) == 0 )
	{
		retVar = &onNewGame;
	}

	if( retVar )
	{
		return retVar;
	}

	return idWindow::GetWinVarByName( _name, winLookup, owner );
}

/*
=============================
idGameBearShootWindow::PostParse
=============================
*/
void idGameBearShootWindow::PostParse()
{
	idWindow::PostParse();
}

/*
=============================
idGameBearShootWindow::Draw
=============================
*/
void idGameBearShootWindow::Draw( int time, float x, float y )
{
	int i;

	//Update the game every frame before drawing
	UpdateGame();

	for( i = entities.Num() - 1; i >= 0; i-- )
	{
		entities[i]->Draw();
	}
}

/*
=============================
idGameBearShootWindow::Activate
=============================
*/
const char* idGameBearShootWindow::Activate( bool activate )
{
	return "";
}

/*
=============================
idGameBearShootWindow::UpdateTurret
=============================
*/
void idGameBearShootWindow::UpdateTurret()
{
	idVec2	pt;
	idVec2	turretOrig;
	idVec2	right;
	float	dot, angle;

	pt.x = gui->CursorX();
	pt.y = gui->CursorY();
	turretOrig.Set( 80.f, 348.f );

	pt = pt - turretOrig;
	pt.NormalizeFast();

	right.x = 1.f;
	right.y = 0.f;

	dot = pt * right;

	angle = RAD2DEG( acosf( dot ) );

	turretAngle = idMath::ClampFloat( 0.f, 90.f, angle );
}

/*
=============================
idGameBearShootWindow::UpdateBear
=============================
*/
void idGameBearShootWindow::UpdateBear()
{
	int time = gui->GetTime();
	bool startShrink = false;

	// Apply gravity
	bear->velocity.y += BEAR_GRAVITY * timeSlice;

	// Apply wind
	bear->velocity.x += windForce * timeSlice;

	// Check for collisions
	if( !bearHitTarget && !gameOver )
	{
		idVec2 bearCenter;
		bool	collision = false;

		bearCenter.x = bear->position.x + bear->width / 2;
		bearCenter.y = bear->position.y + bear->height / 2;

		if( bearCenter.x > ( helicopter->position.x + 16 ) && bearCenter.x < ( helicopter->position.x + helicopter->width - 29 ) )
		{
			if( bearCenter.y > ( helicopter->position.y + 12 ) && bearCenter.y < ( helicopter->position.y + helicopter->height - 7 ) )
			{
				collision = true;
			}
		}

		if( collision )
		{
			// balloons pop and bear tumbles to ground
			helicopter->SetMaterial( "game/bearshoot/helicopter_broken" );
			helicopter->velocity.y = 230.f;
			goal->velocity.y = 230.f;
			common->SW()->PlayShaderDirectly( "arcade_balloonpop" );

			bear->SetVisible( false );
			if( bear->velocity.x > 0 )
			{
				bear->velocity.x *= -1.f;
			}
			bear->velocity *= 0.666f;
			bearHitTarget = true;
			updateScore = true;
			startShrink = true;
		}
	}

	// Check for ground collision
	if( bear->position.y > 380 )
	{
		bear->position.y = 380;

		if( bear->velocity.Length() < 25 )
		{
			bear->velocity.Zero();
		}
		else
		{
			startShrink = true;

			bear->velocity.y *= -1.f;
			bear->velocity *= 0.5f;

			if( bearScale )
			{
				common->SW()->PlayShaderDirectly( "arcade_balloonpop" );
			}
		}
	}

	// Bear rotation is based on velocity
	float angle;
	idVec2 dir;

	dir = bear->velocity;
	dir.NormalizeFast();

	angle = RAD2DEG( atan2( dir.x, dir.y ) );
	bear->rotation = angle - 90;

	// Update Bear scale
	if( bear->position.x > 650 )
	{
		startShrink = true;
	}

	if( !bearIsShrinking && bearScale && startShrink )
	{
		bearShrinkStartTime = time;
		bearIsShrinking = true;
	}

	if( bearIsShrinking )
	{
		if( bearHitTarget )
		{
			bearScale = 1 - ( ( float )( time - bearShrinkStartTime ) / BEAR_SHRINK_TIME );
		}
		else
		{
			bearScale = 1 - ( ( float )( time - bearShrinkStartTime ) / 750 );
		}
		bearScale *= BEAR_SIZE;
		bear->SetSize( bearScale, bearScale );

		if( bearScale < 0 )
		{
			gui->HandleNamedEvent( "EnableFireButton" );
			bearIsShrinking = false;
			bearScale = 0.f;

			if( bearHitTarget )
			{
				goal->SetMaterial( "game/bearshoot/goal" );
				goal->position.x = 550;
				goal->position.y = 164;
				goal->velocity.Zero();
				goal->velocity.y = ( currentLevel - 1 ) * 30;
				goal->entColor.w = 0.f;
				goal->fadeIn = true;
				goal->fadeOut = false;

				helicopter->SetVisible( true );
				helicopter->SetMaterial( "game/bearshoot/helicopter" );
				helicopter->position.x = 550;
				helicopter->position.y = 100;
				helicopter->velocity.Zero();
				helicopter->velocity.y = goal->velocity.y;
				helicopter->entColor.w = 0.f;
				helicopter->fadeIn = true;
				helicopter->fadeOut = false;
			}
		}
	}
}

/*
=============================
idGameBearShootWindow::UpdateHelicopter
=============================
*/
void idGameBearShootWindow::UpdateHelicopter()
{

	if( bearHitTarget && bearIsShrinking )
	{
		if( helicopter->velocity.y != 0 && helicopter->position.y > 264 )
		{
			helicopter->velocity.y = 0;
			goal->velocity.y = 0;

			helicopter->SetVisible( false );
			goal->SetMaterial( "game/bearshoot/goal_dead" );
			common->SW()->PlayShaderDirectly( "arcade_beargroan", 1 );

			helicopter->fadeOut = true;
			goal->fadeOut = true;
		}
	}
	else if( currentLevel > 1 )
	{
		int height = helicopter->position.y;
		float speed = ( currentLevel - 1 ) * 30;

		if( height > 240 )
		{
			helicopter->velocity.y = -speed;
			goal->velocity.y = -speed;
		}
		else if( height < 30 )
		{
			helicopter->velocity.y = speed;
			goal->velocity.y = speed;
		}
	}
}

/*
=============================
idGameBearShootWindow::UpdateButtons
=============================
*/
void idGameBearShootWindow::UpdateButtons()
{

	if( onFire )
	{
		idVec2 vec;

		gui->HandleNamedEvent( "DisableFireButton" );
		common->SW()->PlayShaderDirectly( "arcade_sargeshoot" );

		bear->SetVisible( true );
		bearScale = 1.f;
		bear->SetSize( BEAR_SIZE, BEAR_SIZE );

		vec.x = idMath::Cos( DEG2RAD( turretAngle ) );
		vec.x += ( 1 - vec.x ) * 0.18f;
		vec.y = -idMath::Sin( DEG2RAD( turretAngle ) );

		turretForce = bearTurretForce.GetFloat();

		bear->position.x = 80 + ( 96 * vec.x );
		bear->position.y = 334 + ( 96 * vec.y );
		bear->velocity.x = vec.x * turretForce;
		bear->velocity.y = vec.y * turretForce;

		gunblast->position.x = 55 + ( 96 * vec.x );
		gunblast->position.y = 310 + ( 100 * vec.y );
		gunblast->SetVisible( true );
		gunblast->entColor.w = 1.f;
		gunblast->rotation = turretAngle;
		gunblast->fadeOut = true;

		bearHitTarget = false;

		onFire = false;
	}
}

/*
=============================
idGameBearShootWindow::UpdateScore
=============================
*/
void idGameBearShootWindow::UpdateScore()
{

	if( gameOver )
	{
		gui->HandleNamedEvent( "GameOver" );
		return;
	}

	goalsHit++;
	gui->SetStateString( "player_score", va( "%i", goalsHit ) );

	// Check for level progression
	if( !( goalsHit % 5 ) )
	{
		currentLevel++;
		gui->SetStateString( "current_level", va( "%i", currentLevel ) );
		common->SW()->PlayShaderDirectly( "arcade_levelcomplete1", 3 );

		timeRemaining += 30;
	}
}

/*
=============================
idGameBearShootWindow::UpdateGame
=============================
*/
void idGameBearShootWindow::UpdateGame()
{
	int i;

	if( onNewGame )
	{
		ResetGameState();
		if( goal )
		{
			goal->position.x = 550;
			goal->position.y = 164;
			goal->velocity.Zero();
		}
		if( helicopter )
		{
			helicopter->position.x = 550;
			helicopter->position.y = 100;
			helicopter->velocity.Zero();
		}
		if( bear )
		{
			bear->SetVisible( false );
		}

		bearTurretAngle.SetFloat( 0.f );
		bearTurretForce.SetFloat( 200.f );

		gamerunning = true;
	}
	if( onContinue )
	{
		gameOver = false;
		timeRemaining = 60.f;

		onContinue = false;
	}

	if( gamerunning == true )
	{
		int current_time = gui->GetTime();
		idRandom rnd( current_time );

		// Check for button presses
		UpdateButtons();

		if( bear )
		{
			UpdateBear();
		}
		if( helicopter && goal )
		{
			UpdateHelicopter();
		}

		// Update Wind
		if( windUpdateTime < current_time )
		{
			float	scale;
			int		width;

			windForce = rnd.CRandomFloat() * ( MAX_WINDFORCE * 0.75f );
			if( windForce > 0 )
			{
				windForce += ( MAX_WINDFORCE * 0.25f );
				wind->rotation = 0;
			}
			else
			{
				windForce -= ( MAX_WINDFORCE * 0.25f );
				wind->rotation = 180;
			}

			scale = 1.f - ( ( MAX_WINDFORCE - idMath::Fabs( windForce ) ) / MAX_WINDFORCE );
			width = 100 * scale;

			if( windForce < 0 )
			{
				wind->position.x = 500 - width + 1;
			}
			else
			{
				wind->position.x = 500;
			}
			wind->SetSize( width, 40 );

			windUpdateTime = current_time + 7000 + rnd.RandomInt( 5000 );
		}

		// Update turret rotation angle
		if( turret )
		{
			turretAngle = bearTurretAngle.GetFloat();
			turret->rotation = turretAngle;
		}

		for( i = 0; i < entities.Num(); i++ )
		{
			entities[i]->Update( timeSlice );
		}

		// Update countdown timer
		timeRemaining -= timeSlice;
		timeRemaining = idMath::ClampFloat( 0.f, 99999.f, timeRemaining );
		gui->SetStateString( "time_remaining", va( "%2.1f", timeRemaining ) );

		if( timeRemaining <= 0.f && !gameOver )
		{
			gameOver = true;
			updateScore = true;
		}

		if( updateScore )
		{
			UpdateScore();
			updateScore = false;
		}
	}
}
