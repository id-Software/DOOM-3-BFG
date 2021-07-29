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

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "GameSSDWindow.h"


#define Z_NEAR 100.0f
#define Z_FAR  4000.0f
#define ENTITY_START_DIST 3000

#define V_WIDTH 640.0f
#define V_HEIGHT 480.0f

/*
*****************************************************************************
* SSDCrossHair
****************************************************************************
*/

#define CROSSHAIR_STANDARD_MATERIAL "game/SSD/crosshair_standard"
#define CROSSHAIR_SUPER_MATERIAL "game/SSD/crosshair_super"

SSDCrossHair::SSDCrossHair()
{
}

SSDCrossHair::~SSDCrossHair()
{
}

void SSDCrossHair::WriteToSaveGame( idFile* savefile )
{

	savefile->Write( &currentCrosshair, sizeof( currentCrosshair ) );
	savefile->Write( &crosshairWidth, sizeof( crosshairWidth ) );
	savefile->Write( &crosshairHeight, sizeof( crosshairHeight ) );

}

void SSDCrossHair::ReadFromSaveGame( idFile* savefile )
{

	InitCrosshairs();

	savefile->Read( &currentCrosshair, sizeof( currentCrosshair ) );
	savefile->Read( &crosshairWidth, sizeof( crosshairWidth ) );
	savefile->Read( &crosshairHeight, sizeof( crosshairHeight ) );

}

void SSDCrossHair::InitCrosshairs()
{

	crosshairMaterial[CROSSHAIR_STANDARD] = declManager->FindMaterial( CROSSHAIR_STANDARD_MATERIAL );
	crosshairMaterial[CROSSHAIR_SUPER] = declManager->FindMaterial( CROSSHAIR_SUPER_MATERIAL );

	crosshairWidth = 64;
	crosshairHeight = 64;

	currentCrosshair = CROSSHAIR_STANDARD;

}

void SSDCrossHair::Draw( const idVec2& cursor )
{

	float x, y;
	x = cursor.x - ( crosshairWidth / 2 );
	y = cursor.y - ( crosshairHeight / 2 );
	dc->DrawMaterial( x, y, crosshairWidth, crosshairHeight, crosshairMaterial[currentCrosshair], colorWhite, 1.0f, 1.0f );

}

/*
*****************************************************************************
* SSDEntity
****************************************************************************
*/

SSDEntity::SSDEntity()
{
	EntityInit();
}

SSDEntity::~SSDEntity()
{
}

void SSDEntity::WriteToSaveGame( idFile* savefile )
{

	savefile->Write( &type, sizeof( type ) );
	game->WriteSaveGameString( materialName, savefile );
	savefile->Write( &position, sizeof( position ) );
	savefile->Write( &size, sizeof( size ) );
	savefile->Write( &radius, sizeof( radius ) );
	savefile->Write( &hitRadius, sizeof( hitRadius ) );
	savefile->Write( &rotation, sizeof( rotation ) );

	savefile->Write( &matColor, sizeof( matColor ) );

	game->WriteSaveGameString( text, savefile );
	savefile->Write( &textScale, sizeof( textScale ) );
	savefile->Write( &foreColor, sizeof( foreColor ) );

	savefile->Write( &currentTime, sizeof( currentTime ) );
	savefile->Write( &lastUpdate, sizeof( lastUpdate ) );
	savefile->Write( &elapsed, sizeof( elapsed ) );

	savefile->Write( &destroyed, sizeof( destroyed ) );
	savefile->Write( &noHit, sizeof( noHit ) );
	savefile->Write( &noPlayerDamage, sizeof( noPlayerDamage ) );

	savefile->Write( &inUse, sizeof( inUse ) );

}

void SSDEntity::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{

	savefile->Read( &type, sizeof( type ) );
	game->ReadSaveGameString( materialName, savefile );
	SetMaterial( materialName );
	savefile->Read( &position, sizeof( position ) );
	savefile->Read( &size, sizeof( size ) );
	savefile->Read( &radius, sizeof( radius ) );
	savefile->Read( &hitRadius, sizeof( hitRadius ) );
	savefile->Read( &rotation, sizeof( rotation ) );

	savefile->Read( &matColor, sizeof( matColor ) );

	game->ReadSaveGameString( text, savefile );
	savefile->Read( &textScale, sizeof( textScale ) );
	savefile->Read( &foreColor, sizeof( foreColor ) );

	game = _game;
	savefile->Read( &currentTime, sizeof( currentTime ) );
	savefile->Read( &lastUpdate, sizeof( lastUpdate ) );
	savefile->Read( &elapsed, sizeof( elapsed ) );

	savefile->Read( &destroyed, sizeof( destroyed ) );
	savefile->Read( &noHit, sizeof( noHit ) );
	savefile->Read( &noPlayerDamage, sizeof( noPlayerDamage ) );

	savefile->Read( &inUse, sizeof( inUse ) );
}

void SSDEntity::EntityInit()
{

	inUse = false;


	type = SSD_ENTITY_BASE;

	materialName = "";
	material = NULL;
	position.Zero();
	size.Zero();
	radius = 0.0f;
	hitRadius = 0.0f;
	rotation = 0.0f;


	currentTime = 0;
	lastUpdate = 0;

	destroyed = false;
	noHit = false;
	noPlayerDamage = false;

	matColor.Set( 1, 1, 1, 1 );

	text = "";
	textScale = 1.0f;
	foreColor.Set( 1, 1, 1, 1 );
}

void SSDEntity::SetGame( idGameSSDWindow* _game )
{
	game = _game;
}

void SSDEntity::SetMaterial( const char* name )
{
	materialName = name;
	material = declManager->FindMaterial( name );
	material->SetSort( SS_GUI );
}

void SSDEntity::SetPosition( const idVec3& _position )
{
	position = _position;
}

void SSDEntity::SetSize( const idVec2& _size )
{
	size = _size;
}

void SSDEntity::SetRadius( float _radius, float _hitFactor )
{
	radius = _radius;
	hitRadius = _radius * _hitFactor;
}

void SSDEntity::SetRotation( float _rotation )
{
	rotation = _rotation;
}

void SSDEntity::Update()
{

	currentTime = game->ssdTime;

	//Is this the first update
	if( lastUpdate == 0 )
	{
		lastUpdate = currentTime;
		return;
	}

	elapsed = currentTime - lastUpdate;

	EntityUpdate();

	lastUpdate = currentTime;
}

bool SSDEntity::HitTest( const idVec2& pt )
{

	if( noHit )
	{
		return false;
	}

	idVec3 screenPos = WorldToScreen( position );


	//Scale the radius based on the distance from the player
	float scale = 1.0f - ( ( screenPos.z - Z_NEAR ) / ( Z_FAR - Z_NEAR ) );
	float scaledRad = scale * hitRadius;

	//So we can compare against the square of the length between two points
	float scaleRadSqr = scaledRad * scaledRad;

	idVec2 diff = screenPos.ToVec2() - pt;
	float dist = idMath::Fabs( diff.LengthSqr() );

	if( dist < scaleRadSqr )
	{
		return true;
	}
	return false;
}

void SSDEntity::Draw()
{


	idVec2 persize;
	float x, y;

	idBounds bounds;
	bounds[0] = idVec3( position.x - ( size.x / 2.0f ), position.y - ( size.y / 2.0f ), position.z );
	bounds[1] = idVec3( position.x + ( size.x / 2.0f ), position.y + ( size.y / 2.0f ), position.z );

	idBounds screenBounds = WorldToScreen( bounds );
	persize.x = idMath::Fabs( screenBounds[1].x - screenBounds[0].x );
	persize.y = idMath::Fabs( screenBounds[1].y - screenBounds[0].y );

	idVec3 center = screenBounds.GetCenter();

	x = screenBounds[0].x;
	y = screenBounds[1].y;
	dc->DrawMaterialRotated( x, y, persize.x, persize.y, material, matColor, 1.0f, 1.0f, DEG2RAD( rotation ) );

	if( text.Length() > 0 )
	{
		idRectangle rect( x, y, VIRTUAL_WIDTH, VIRTUAL_HEIGHT );
		dc->DrawText( text, textScale, 0, foreColor, rect, false );
	}

}

void SSDEntity::DestroyEntity()
{
	inUse = false;
}

idBounds SSDEntity::WorldToScreen( const idBounds worldBounds )
{

	idVec3 screenMin = WorldToScreen( worldBounds[0] );
	idVec3 screenMax = WorldToScreen( worldBounds[1] );

	idBounds screenBounds( screenMin, screenMax );
	return screenBounds;
}

idVec3 SSDEntity::WorldToScreen( const idVec3& worldPos )
{

	float d = 0.5f * V_WIDTH * idMath::Tan( DEG2RAD( 90.0f ) / 2.0f );

	//World To Camera Coordinates
	idVec3 cameraTrans( 0, 0, d );
	idVec3 cameraPos;
	cameraPos = worldPos + cameraTrans;

	//Camera To Screen Coordinates
	idVec3 screenPos;
	screenPos.x = d * cameraPos.x / cameraPos.z + ( 0.5f * V_WIDTH - 0.5f );
	screenPos.y = -d * cameraPos.y / cameraPos.z + ( 0.5f * V_HEIGHT - 0.5f );
	screenPos.z = cameraPos.z;

	return screenPos;
}

idVec3 SSDEntity::ScreenToWorld( const idVec3& screenPos )
{

	idVec3 worldPos;

	worldPos.x = screenPos.x - 0.5f * V_WIDTH;
	worldPos.y = -( screenPos.y  - 0.5f * V_HEIGHT );
	worldPos.z = screenPos.z;

	return worldPos;
}

/*
*****************************************************************************
* SSDMover
****************************************************************************
*/

SSDMover::SSDMover()
{
}

SSDMover::~SSDMover()
{
}

void SSDMover::WriteToSaveGame( idFile* savefile )
{
	SSDEntity::WriteToSaveGame( savefile );

	savefile->Write( &speed, sizeof( speed ) );
	savefile->Write( &rotationSpeed, sizeof( rotationSpeed ) );
}

void SSDMover::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDEntity::ReadFromSaveGame( savefile, _game );

	savefile->Read( &speed, sizeof( speed ) );
	savefile->Read( &rotationSpeed, sizeof( rotationSpeed ) );
}

void SSDMover::MoverInit( const idVec3& _speed, float _rotationSpeed )
{

	speed = _speed;
	rotationSpeed = _rotationSpeed;
}

void SSDMover::EntityUpdate()
{

	SSDEntity::EntityUpdate();

	//Move forward based on speed (units per second)
	idVec3 moved = ( ( float )elapsed / 1000.0f ) * speed;
	position += moved;

	float rotated = ( ( float )elapsed / 1000.0f ) * rotationSpeed * 360.0f;
	rotation += rotated;
	if( rotation >= 360 )
	{
		rotation -= 360.0f;
	}
	if( rotation < 0 )
	{
		rotation += 360.0f;
	}
}


/*
*****************************************************************************
* SSDAsteroid
****************************************************************************
*/

SSDAsteroid	SSDAsteroid::asteroidPool[MAX_ASTEROIDS];

#define ASTEROID_MATERIAL "game/SSD/asteroid"

SSDAsteroid::SSDAsteroid()
{
}

SSDAsteroid::~SSDAsteroid()
{
}

void SSDAsteroid::WriteToSaveGame( idFile* savefile )
{
	SSDMover::WriteToSaveGame( savefile );

	savefile->Write( &health, sizeof( health ) );
}

void SSDAsteroid::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDMover::ReadFromSaveGame( savefile, _game );

	savefile->Read( &health, sizeof( health ) );
}

void SSDAsteroid::Init( idGameSSDWindow* _game, const idVec3& startPosition, const idVec2& _size, float _speed, float rotate, int _health )
{

	EntityInit();
	MoverInit( idVec3( 0, 0, -_speed ), rotate );

	SetGame( _game );

	type = SSD_ENTITY_ASTEROID;

	SetMaterial( ASTEROID_MATERIAL );
	SetSize( _size );
	SetRadius( Max( size.x, size.y ), 0.3f );
	SetRotation( game->random.RandomInt( 360 ) );


	position = startPosition;

	health = _health;
}

void SSDAsteroid::EntityUpdate()
{

	SSDMover::EntityUpdate();
}

SSDAsteroid* SSDAsteroid::GetNewAsteroid( idGameSSDWindow* _game, const idVec3& startPosition, const idVec2& _size, float _speed, float rotate, int _health )
{
	for( int i = 0; i < MAX_ASTEROIDS; i++ )
	{
		if( !asteroidPool[i].inUse )
		{
			asteroidPool[i].Init( _game, startPosition, _size, _speed, rotate, _health );
			asteroidPool[i].inUse = true;
			asteroidPool[i].id = i;

			return &asteroidPool[i];
		}
	}
	return NULL;
}

SSDAsteroid* SSDAsteroid::GetSpecificAsteroid( int id )
{
	return &asteroidPool[id];
}

void SSDAsteroid::WriteAsteroids( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_ASTEROIDS; i++ )
	{
		if( asteroidPool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_ASTEROIDS; i++ )
	{
		if( asteroidPool[i].inUse )
		{
			savefile->Write( &( asteroidPool[i].id ), sizeof( asteroidPool[i].id ) );
			asteroidPool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDAsteroid::ReadAsteroids( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDAsteroid* ent = GetSpecificAsteroid( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* SSDAstronaut
****************************************************************************
*/

SSDAstronaut	SSDAstronaut::astronautPool[MAX_ASTRONAUT];

#define ASTRONAUT_MATERIAL "game/SSD/astronaut"

SSDAstronaut::SSDAstronaut()
{
}

SSDAstronaut::~SSDAstronaut()
{
}

void SSDAstronaut::WriteToSaveGame( idFile* savefile )
{
	SSDMover::WriteToSaveGame( savefile );

	savefile->Write( &health, sizeof( health ) );
}

void SSDAstronaut::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDMover::ReadFromSaveGame( savefile, _game );

	savefile->Read( &health, sizeof( health ) );
}

void SSDAstronaut::Init( idGameSSDWindow* _game, const idVec3& startPosition, float _speed, float rotate, int _health )
{

	EntityInit();
	MoverInit( idVec3( 0, 0, -_speed ), rotate );

	SetGame( _game );

	type = SSD_ENTITY_ASTRONAUT;

	SetMaterial( ASTRONAUT_MATERIAL );
	SetSize( idVec2( 256, 256 ) );
	SetRadius( Max( size.x, size.y ), 0.3f );
	SetRotation( game->random.RandomInt( 360 ) );

	position = startPosition;
	health = _health;
}

SSDAstronaut* SSDAstronaut::GetNewAstronaut( idGameSSDWindow* _game, const idVec3& startPosition, float _speed, float rotate, int _health )
{
	for( int i = 0; i < MAX_ASTRONAUT; i++ )
	{
		if( !astronautPool[i].inUse )
		{
			astronautPool[i].Init( _game, startPosition, _speed, rotate, _health );
			astronautPool[i].inUse = true;
			astronautPool[i].id = i;
			return &astronautPool[i];
		}
	}
	return NULL;
}

SSDAstronaut* SSDAstronaut::GetSpecificAstronaut( int id )
{
	return &astronautPool[id];

}

void SSDAstronaut::WriteAstronauts( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_ASTRONAUT; i++ )
	{
		if( astronautPool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_ASTRONAUT; i++ )
	{
		if( astronautPool[i].inUse )
		{
			savefile->Write( &( astronautPool[i].id ), sizeof( astronautPool[i].id ) );
			astronautPool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDAstronaut::ReadAstronauts( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDAstronaut* ent = GetSpecificAstronaut( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* SSDExplosion
****************************************************************************
*/

SSDExplosion SSDExplosion::explosionPool[MAX_EXPLOSIONS];


//#define EXPLOSION_MATERIAL "game/SSD/fball"
//#define EXPLOSION_TELEPORT "game/SSD/teleport"

const char* explosionMaterials[] =
{
	"game/SSD/fball",
	"game/SSD/teleport"
};

#define EXPLOSION_MATERIAL_COUNT 2

SSDExplosion::SSDExplosion()
{
	type = SSD_ENTITY_EXPLOSION;
}

SSDExplosion::~SSDExplosion()
{
}

void SSDExplosion::WriteToSaveGame( idFile* savefile )
{
	SSDEntity::WriteToSaveGame( savefile );

	savefile->Write( &finalSize, sizeof( finalSize ) );
	savefile->Write( &length, sizeof( length ) );
	savefile->Write( &beginTime, sizeof( beginTime ) );
	savefile->Write( &endTime, sizeof( endTime ) );
	savefile->Write( &explosionType, sizeof( explosionType ) );


	savefile->Write( &( buddy->type ), sizeof( buddy->type ) );
	savefile->Write( &( buddy->id ), sizeof( buddy->id ) );

	savefile->Write( &killBuddy, sizeof( killBuddy ) );
	savefile->Write( &followBuddy, sizeof( followBuddy ) );
}

void SSDExplosion::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDEntity::ReadFromSaveGame( savefile, _game );

	savefile->Read( &finalSize, sizeof( finalSize ) );
	savefile->Read( &length, sizeof( length ) );
	savefile->Read( &beginTime, sizeof( beginTime ) );
	savefile->Read( &endTime, sizeof( endTime ) );
	savefile->Read( &explosionType, sizeof( explosionType ) );

	int type, id;
	savefile->Read( &type, sizeof( type ) );
	savefile->Read( &id, sizeof( id ) );

	//Get a pointer to my buddy
	buddy = _game->GetSpecificEntity( type, id );

	savefile->Read( &killBuddy, sizeof( killBuddy ) );
	savefile->Read( &followBuddy, sizeof( followBuddy ) );
}

void SSDExplosion::Init( idGameSSDWindow* _game, const idVec3& _position, const idVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy, bool _followBuddy )
{

	EntityInit();

	SetGame( _game );

	type = SSD_ENTITY_EXPLOSION;
	explosionType = _type;

	SetMaterial( explosionMaterials[explosionType] );
	SetPosition( _position );
	position.z -= 50;

	finalSize = _size;
	length = _length;
	beginTime = game->ssdTime;
	endTime = beginTime + length;

	buddy = _buddy;
	killBuddy = _killBuddy;
	followBuddy = _followBuddy;

	//Explosion Starts from nothing and will increase in size until it gets to final size
	size.Zero();

	noPlayerDamage = true;
	noHit = true;
}

void SSDExplosion::EntityUpdate()
{

	SSDEntity::EntityUpdate();

	//Always set my position to my buddies position except change z to be on top
	if( followBuddy )
	{
		position = buddy->position;
		position.z -= 50;
	}
	else
	{
		//Only mess with the z if we are not following
		position.z = buddy->position.z - 50;
	}

	//Scale the image based on the time
	size = finalSize * ( ( float )( currentTime - beginTime ) / ( float )length );

	//Destroy myself after the explosion is done
	if( currentTime > endTime )
	{
		destroyed = true;

		if( killBuddy )
		{
			//Destroy the exploding object
			buddy->destroyed = true;
		}
	}
}

SSDExplosion* SSDExplosion::GetNewExplosion( idGameSSDWindow* _game, const idVec3& _position, const idVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy, bool _followBuddy )
{
	for( int i = 0; i < MAX_EXPLOSIONS; i++ )
	{
		if( !explosionPool[i].inUse )
		{
			explosionPool[i].Init( _game, _position, _size, _length, _type, _buddy, _killBuddy, _followBuddy );
			explosionPool[i].inUse = true;
			return &explosionPool[i];
		}
	}
	return NULL;
}

SSDExplosion* SSDExplosion::GetSpecificExplosion( int id )
{
	return &explosionPool[id];
}

void SSDExplosion::WriteExplosions( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_EXPLOSIONS; i++ )
	{
		if( explosionPool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_EXPLOSIONS; i++ )
	{
		if( explosionPool[i].inUse )
		{
			savefile->Write( &( explosionPool[i].id ), sizeof( explosionPool[i].id ) );
			explosionPool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDExplosion::ReadExplosions( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDExplosion* ent = GetSpecificExplosion( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* SSDPoints
****************************************************************************
*/

SSDPoints	SSDPoints::pointsPool[MAX_POINTS];

SSDPoints::SSDPoints()
{
	type = SSD_ENTITY_POINTS;
}

SSDPoints::~SSDPoints()
{
}

void SSDPoints::WriteToSaveGame( idFile* savefile )
{
	SSDEntity::WriteToSaveGame( savefile );

	savefile->Write( &length, sizeof( length ) );
	savefile->Write( &distance, sizeof( distance ) );
	savefile->Write( &beginTime, sizeof( beginTime ) );
	savefile->Write( &endTime, sizeof( endTime ) );

	savefile->Write( &beginPosition, sizeof( beginPosition ) );
	savefile->Write( &endPosition, sizeof( endPosition ) );

	savefile->Write( &beginColor, sizeof( beginColor ) );
	savefile->Write( &endColor, sizeof( endColor ) );

}

void SSDPoints::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDEntity::ReadFromSaveGame( savefile, _game );

	savefile->Read( &length, sizeof( length ) );
	savefile->Read( &distance, sizeof( distance ) );
	savefile->Read( &beginTime, sizeof( beginTime ) );
	savefile->Read( &endTime, sizeof( endTime ) );

	savefile->Read( &beginPosition, sizeof( beginPosition ) );
	savefile->Read( &endPosition, sizeof( endPosition ) );

	savefile->Read( &beginColor, sizeof( beginColor ) );
	savefile->Read( &endColor, sizeof( endColor ) );
}

void SSDPoints::Init( idGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const idVec4& color )
{

	EntityInit();

	SetGame( _game );

	length = _length;
	distance = _distance;
	beginTime = game->ssdTime;
	endTime = beginTime + length;

	textScale = 0.4f;
	text = va( "%d", _points );

	float width = 0;
	for( int i = 0; i < text.Length(); i++ )
	{
		width += dc->CharWidth( text[i], textScale );
	}

	size.Set( 0, 0 );

	//Set the start position at the top of the passed in entity
	position = WorldToScreen( _ent->position );
	position = ScreenToWorld( position );

	position.z = 0;
	position.x -= ( width / 2.0f );

	beginPosition = position;

	endPosition = beginPosition;
	endPosition.y += _distance;

	//beginColor.Set(0,1,0,1);
	endColor.Set( 1, 1, 1, 0 );

	beginColor = color;
	beginColor.w = 1;

	noPlayerDamage = true;
	noHit = true;
}

void SSDPoints::EntityUpdate()
{

	float t = ( float )( currentTime - beginTime ) / ( float )length;

	//Move up from the start position
	position.Lerp( beginPosition, endPosition, t );

	//Interpolate the color
	foreColor.Lerp( beginColor, endColor, t );

	if( currentTime > endTime )
	{
		destroyed = true;
	}
}

SSDPoints* SSDPoints::GetNewPoints( idGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const idVec4& color )
{
	for( int i = 0; i < MAX_POINTS; i++ )
	{
		if( !pointsPool[i].inUse )
		{
			pointsPool[i].Init( _game, _ent, _points, _length, _distance, color );
			pointsPool[i].inUse = true;
			return &pointsPool[i];
		}
	}
	return NULL;
}

SSDPoints* SSDPoints::GetSpecificPoints( int id )
{
	return &pointsPool[id];
}

void SSDPoints::WritePoints( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_POINTS; i++ )
	{
		if( pointsPool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_POINTS; i++ )
	{
		if( pointsPool[i].inUse )
		{
			savefile->Write( &( pointsPool[i].id ), sizeof( pointsPool[i].id ) );
			pointsPool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDPoints::ReadPoints( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDPoints* ent = GetSpecificPoints( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* SSDProjectile
****************************************************************************
*/

SSDProjectile SSDProjectile::projectilePool[MAX_PROJECTILES];

#define PROJECTILE_MATERIAL "game/SSD/fball"

SSDProjectile::SSDProjectile()
{
	type = SSD_ENTITY_PROJECTILE;
}

SSDProjectile::~SSDProjectile()
{
}

void SSDProjectile::WriteToSaveGame( idFile* savefile )
{
	SSDEntity::WriteToSaveGame( savefile );

	savefile->Write( &dir, sizeof( dir ) );
	savefile->Write( &speed, sizeof( speed ) );
	savefile->Write( &beginTime, sizeof( beginTime ) );
	savefile->Write( &endTime, sizeof( endTime ) );

	savefile->Write( &endPosition, sizeof( endPosition ) );
}

void SSDProjectile::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDEntity::ReadFromSaveGame( savefile, _game );

	savefile->Read( &dir, sizeof( dir ) );
	savefile->Read( &speed, sizeof( speed ) );
	savefile->Read( &beginTime, sizeof( beginTime ) );
	savefile->Read( &endTime, sizeof( endTime ) );

	savefile->Read( &endPosition, sizeof( endPosition ) );
}

void SSDProjectile::Init( idGameSSDWindow* _game, const idVec3& _beginPosition, const idVec3& _endPosition, float _speed, float _size )
{

	EntityInit();

	SetGame( _game );

	SetMaterial( PROJECTILE_MATERIAL );
	size.Set( _size, _size );

	position = _beginPosition;
	endPosition = _endPosition;

	dir = _endPosition - position;
	dir.Normalize();

	//speed.Zero();
	speed.x = speed.y = speed.z = _speed;

	noHit = true;
}

void SSDProjectile::EntityUpdate()
{

	SSDEntity::EntityUpdate();

	//Move forward based on speed (units per second)
	idVec3 moved = dir * ( ( float )elapsed / 1000.0f ) * speed.z;
	position += moved;

	if( position.z > endPosition.z )
	{
		//We have reached our position
		destroyed = true;
	}
}

SSDProjectile* SSDProjectile::GetNewProjectile( idGameSSDWindow* _game, const idVec3& _beginPosition, const idVec3& _endPosition, float _speed, float _size )
{
	for( int i = 0; i < MAX_PROJECTILES; i++ )
	{
		if( !projectilePool[i].inUse )
		{
			projectilePool[i].Init( _game, _beginPosition, _endPosition, _speed, _size );
			projectilePool[i].inUse = true;
			return &projectilePool[i];
		}
	}
	return NULL;
}

SSDProjectile* SSDProjectile::GetSpecificProjectile( int id )
{
	return &projectilePool[id];
}

void SSDProjectile::WriteProjectiles( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_PROJECTILES; i++ )
	{
		if( projectilePool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_PROJECTILES; i++ )
	{
		if( projectilePool[i].inUse )
		{
			savefile->Write( &( projectilePool[i].id ), sizeof( projectilePool[i].id ) );
			projectilePool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDProjectile::ReadProjectiles( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDProjectile* ent = GetSpecificProjectile( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* SSDPowerup
****************************************************************************
*/

const char* powerupMaterials[][2] =
{
	"game/SSD/powerupHealthClosed",			"game/SSD/powerupHealthOpen",
	"game/SSD/powerupSuperBlasterClosed",	"game/SSD/powerupSuperBlasterOpen",
	"game/SSD/powerupNukeClosed",			"game/SSD/powerupNukeOpen",
	"game/SSD/powerupRescueClosed",			"game/SSD/powerupRescueOpen",
	"game/SSD/powerupBonusPointsClosed",	"game/SSD/powerupBonusPointsOpen",
	"game/SSD/powerupDamageClosed",			"game/SSD/powerupDamageOpen",
};

#define POWERUP_MATERIAL_COUNT 6

SSDPowerup	SSDPowerup::powerupPool[MAX_POWERUPS];

SSDPowerup::SSDPowerup()
{

}

SSDPowerup::~SSDPowerup()
{
}

void SSDPowerup::WriteToSaveGame( idFile* savefile )
{
	SSDMover::WriteToSaveGame( savefile );

	savefile->Write( &powerupState, sizeof( powerupState ) );
	savefile->Write( &powerupType, sizeof( powerupType ) );
}

void SSDPowerup::ReadFromSaveGame( idFile* savefile,  idGameSSDWindow* _game )
{
	SSDMover::ReadFromSaveGame( savefile, _game );

	savefile->Read( &powerupState, sizeof( powerupState ) );
	savefile->Read( &powerupType, sizeof( powerupType ) );
}

void SSDPowerup::OnHit( int key )
{

	if( powerupState == POWERUP_STATE_CLOSED )
	{

		//Small explosion to indicate it is opened
		SSDExplosion* explosion = SSDExplosion::GetNewExplosion( game, position, size * 2.0f, 300, SSDExplosion::EXPLOSION_NORMAL, this, false, true );
		game->entities.Append( explosion );


		powerupState = POWERUP_STATE_OPEN;
		SetMaterial( powerupMaterials[powerupType][powerupState] );
	}
	else
	{
		//Destory the powerup with a big explosion
		SSDExplosion* explosion = SSDExplosion::GetNewExplosion( game, position, size * 2, 300, SSDExplosion::EXPLOSION_NORMAL, this );
		game->entities.Append( explosion );
		game->PlaySound( "arcade_explode" );

		noHit = true;
		noPlayerDamage = true;
	}
}

void SSDPowerup::OnStrikePlayer()
{

	if( powerupState == POWERUP_STATE_OPEN )
	{
		//The powerup was open so activate it
		OnActivatePowerup();
	}

	//Just destroy the powerup
	destroyed = true;
}

void SSDPowerup::OnOpenPowerup()
{
}

void SSDPowerup::OnActivatePowerup()
{
	switch( powerupType )
	{
		case POWERUP_TYPE_HEALTH:
		{
			game->AddHealth( 10 );
			break;
		}
		case POWERUP_TYPE_SUPER_BLASTER:
		{
			game->OnSuperBlaster();
			break;
		}
		case POWERUP_TYPE_ASTEROID_NUKE:
		{
			game->OnNuke();
			break;
		}
		case POWERUP_TYPE_RESCUE_ALL:
		{
			game->OnRescueAll();
			break;
		}
		case POWERUP_TYPE_BONUS_POINTS:
		{
			int points = ( game->random.RandomInt( 5 ) + 1 ) * 100;
			game->AddScore( this, points );
			break;
		}
		case POWERUP_TYPE_DAMAGE:
		{
			game->AddDamage( 10 );
			game->PlaySound( "arcade_explode" );
			break;
		}

	}
}


void SSDPowerup::Init( idGameSSDWindow* _game, float _speed, float _rotation )
{

	EntityInit();
	MoverInit( idVec3( 0, 0, -_speed ), _rotation );

	SetGame( _game );
	SetSize( idVec2( 200, 200 ) );
	SetRadius( Max( size.x, size.y ), 0.3f );

	type = SSD_ENTITY_POWERUP;

	idVec3 startPosition;
	startPosition.x = game->random.RandomInt( V_WIDTH ) - ( V_WIDTH / 2.0f );
	startPosition.y = game->random.RandomInt( V_HEIGHT ) - ( V_HEIGHT / 2.0f );
	startPosition.z = ENTITY_START_DIST;

	position = startPosition;
	//SetPosition(startPosition);

	powerupState = POWERUP_STATE_CLOSED;
	powerupType = game->random.RandomInt( POWERUP_TYPE_MAX + 1 );
	if( powerupType >= POWERUP_TYPE_MAX )
	{
		powerupType = 0;
	}

	/*OutputDebugString(va("Powerup: %d\n", powerupType));
	if(powerupType == 0) {
		int x = 0;
	}*/

	SetMaterial( powerupMaterials[powerupType][powerupState] );
}

SSDPowerup* SSDPowerup::GetNewPowerup( idGameSSDWindow* _game, float _speed, float _rotation )
{

	for( int i = 0; i < MAX_POWERUPS; i++ )
	{
		if( !powerupPool[i].inUse )
		{
			powerupPool[i].Init( _game, _speed, _rotation );
			powerupPool[i].inUse = true;
			return &powerupPool[i];
		}
	}
	return NULL;
}

SSDPowerup* SSDPowerup::GetSpecificPowerup( int id )
{
	return &powerupPool[id];
}

void SSDPowerup::WritePowerups( idFile* savefile )
{
	int count = 0;
	for( int i = 0; i < MAX_POWERUPS; i++ )
	{
		if( powerupPool[i].inUse )
		{
			count++;
		}
	}
	savefile->Write( &count, sizeof( count ) );
	for( int i = 0; i < MAX_POWERUPS; i++ )
	{
		if( powerupPool[i].inUse )
		{
			savefile->Write( &( powerupPool[i].id ), sizeof( powerupPool[i].id ) );
			powerupPool[i].WriteToSaveGame( savefile );
		}
	}
}

void SSDPowerup::ReadPowerups( idFile* savefile, idGameSSDWindow* _game )
{

	int count;
	savefile->Read( &count, sizeof( count ) );
	for( int i = 0; i < count; i++ )
	{
		int id;
		savefile->Read( &id, sizeof( id ) );
		SSDPowerup* ent = GetSpecificPowerup( id );
		ent->ReadFromSaveGame( savefile, _game );
	}
}

/*
*****************************************************************************
* idGameSSDWindow
****************************************************************************
*/

idRandom idGameSSDWindow::random;

idGameSSDWindow::idGameSSDWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idGameSSDWindow::~idGameSSDWindow()
{
	ResetGameStats();
}

void idGameSSDWindow::WriteToSaveGame( idFile* savefile )
{
	idWindow::WriteToSaveGame( savefile );

	savefile->Write( &ssdTime, sizeof( ssdTime ) );

	beginLevel.WriteToSaveGame( savefile );
	resetGame.WriteToSaveGame( savefile );
	continueGame.WriteToSaveGame( savefile );
	refreshGuiData.WriteToSaveGame( savefile );

	crosshair.WriteToSaveGame( savefile );
	savefile->Write( &screenBounds, sizeof( screenBounds ) );

	savefile->Write( &levelCount, sizeof( levelCount ) );
	for( int i = 0; i < levelCount; i++ )
	{
		savefile->Write( &( levelData[i] ), sizeof( SSDLevelData_t ) );
		savefile->Write( &( asteroidData[i] ), sizeof( SSDAsteroidData_t ) );
		savefile->Write( &( astronautData[i] ), sizeof( SSDAstronautData_t ) );
		savefile->Write( &( powerupData[i] ), sizeof( SSDPowerupData_t ) );
	}

	savefile->Write( &weaponCount, sizeof( weaponCount ) );
	for( int i = 0; i < weaponCount; i++ )
	{
		savefile->Write( &( weaponData[i] ), sizeof( SSDWeaponData_t ) );
	}

	savefile->Write( &superBlasterTimeout, sizeof( superBlasterTimeout ) );
	savefile->Write( &gameStats, sizeof( SSDGameStats_t ) );

	//Write All Static Entities
	SSDAsteroid::WriteAsteroids( savefile );
	SSDAstronaut::WriteAstronauts( savefile );
	SSDExplosion::WriteExplosions( savefile );
	SSDPoints::WritePoints( savefile );
	SSDProjectile::WriteProjectiles( savefile );
	SSDPowerup::WritePowerups( savefile );

	int entCount = entities.Num();
	savefile->Write( &entCount, sizeof( entCount ) );
	for( int i = 0; i < entCount; i++ )
	{
		savefile->Write( &( entities[i]->type ), sizeof( entities[i]->type ) );
		savefile->Write( &( entities[i]->id ), sizeof( entities[i]->id ) );
	}
}

void idGameSSDWindow::ReadFromSaveGame( idFile* savefile )
{
	idWindow::ReadFromSaveGame( savefile );


	savefile->Read( &ssdTime, sizeof( ssdTime ) );

	beginLevel.ReadFromSaveGame( savefile );
	resetGame.ReadFromSaveGame( savefile );
	continueGame.ReadFromSaveGame( savefile );
	refreshGuiData.ReadFromSaveGame( savefile );

	crosshair.ReadFromSaveGame( savefile );
	savefile->Read( &screenBounds, sizeof( screenBounds ) );

	savefile->Read( &levelCount, sizeof( levelCount ) );
	for( int i = 0; i < levelCount; i++ )
	{
		SSDLevelData_t newLevel;
		savefile->Read( &newLevel, sizeof( SSDLevelData_t ) );
		levelData.Append( newLevel );

		SSDAsteroidData_t newAsteroid;
		savefile->Read( &newAsteroid, sizeof( SSDAsteroidData_t ) );
		asteroidData.Append( newAsteroid );

		SSDAstronautData_t newAstronaut;
		savefile->Read( &newAstronaut, sizeof( SSDAstronautData_t ) );
		astronautData.Append( newAstronaut );

		SSDPowerupData_t newPowerup;
		savefile->Read( &newPowerup, sizeof( SSDPowerupData_t ) );
		powerupData.Append( newPowerup );
	}

	savefile->Read( &weaponCount, sizeof( weaponCount ) );
	for( int i = 0; i < weaponCount; i++ )
	{
		SSDWeaponData_t newWeapon;
		savefile->Read( &newWeapon, sizeof( SSDWeaponData_t ) );
		weaponData.Append( newWeapon );
	}

	savefile->Read( &superBlasterTimeout, sizeof( superBlasterTimeout ) );

	savefile->Read( &gameStats, sizeof( SSDGameStats_t ) );
	//Reset this because it is no longer valid
	gameStats.levelStats.targetEnt = NULL;

	SSDAsteroid::ReadAsteroids( savefile, this );
	SSDAstronaut::ReadAstronauts( savefile, this );
	SSDExplosion::ReadExplosions( savefile, this );
	SSDPoints::ReadPoints( savefile, this );
	SSDProjectile::ReadProjectiles( savefile, this );
	SSDPowerup::ReadPowerups( savefile, this );

	int entCount;
	savefile->Read( &entCount, sizeof( entCount ) );

	for( int i = 0; i < entCount; i++ )
	{
		int type, id;
		savefile->Read( &type, sizeof( type ) );
		savefile->Read( &id, sizeof( id ) );

		SSDEntity* ent = GetSpecificEntity( type, id );
		if( ent )
		{
			entities.Append( ent );
		}
	}
}

const char* idGameSSDWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{

	// need to call this to allow proper focus and capturing on embedded children
	const char* ret = idWindow::HandleEvent( event, updateVisuals );

	if( !gameStats.gameRunning )
	{
		return ret;
	}

	int key = event->evValue;

	if( event->evType == SE_KEY )
	{

		if( !event->evValue2 )
		{
			return ret;
		}

		if( key == K_MOUSE1 || key == K_MOUSE2 )
		{
			FireWeapon( key );
		}
		else
		{
			return ret;
		}
	}
	return ret;
}

idWinVar* idGameSSDWindow::GetWinVarByName( const char* _name, bool winLookup, drawWin_t** owner )
{

	idWinVar* retVar = NULL;

	if( idStr::Icmp( _name, "beginLevel" ) == 0 )
	{
		retVar = &beginLevel;
	}

	if( idStr::Icmp( _name, "resetGame" ) == 0 )
	{
		retVar = &resetGame;
	}

	if( idStr::Icmp( _name, "continueGame" ) == 0 )
	{
		retVar = &continueGame;
	}
	if( idStr::Icmp( _name, "refreshGuiData" ) == 0 )
	{
		retVar = &refreshGuiData;
	}


	if( retVar )
	{
		return retVar;
	}

	return idWindow::GetWinVarByName( _name, winLookup, owner );
}


void idGameSSDWindow::Draw( int time, float x, float y )
{

	//Update the game every frame before drawing
	UpdateGame();

	RefreshGuiData();

	if( gameStats.gameRunning )
	{

		ZOrderEntities();

		//Draw from back to front
		for( int i = entities.Num() - 1; i >= 0; i-- )
		{
			entities[i]->Draw();
		}

		//The last thing to draw is the crosshair
		idVec2 cursor;
		//GetCursor(cursor);
		cursor.x = gui->CursorX();
		cursor.y = gui->CursorY();

		crosshair.Draw( cursor );
	}
}


bool idGameSSDWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{

	if( idStr::Icmp( _name, "beginLevel" ) == 0 )
	{
		beginLevel = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "resetGame" ) == 0 )
	{
		resetGame = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "continueGame" ) == 0 )
	{
		continueGame = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "refreshGuiData" ) == 0 )
	{
		refreshGuiData = src->ParseBool();
		return true;
	}

	if( idStr::Icmp( _name, "levelcount" ) == 0 )
	{
		levelCount = src->ParseInt();
		for( int i = 0; i < levelCount; i++ )
		{
			SSDLevelData_t newLevel;
			memset( &newLevel, 0, sizeof( SSDLevelData_t ) );
			levelData.Append( newLevel );

			SSDAsteroidData_t newAsteroid;
			memset( &newAsteroid, 0, sizeof( SSDAsteroidData_t ) );
			asteroidData.Append( newAsteroid );

			SSDAstronautData_t newAstronaut;
			memset( &newAstronaut, 0, sizeof( SSDAstronautData_t ) );
			astronautData.Append( newAstronaut );

			SSDPowerupData_t newPowerup;
			memset( &newPowerup, 0, sizeof( SSDPowerupData_t ) );
			powerupData.Append( newPowerup );


		}
		return true;
	}
	if( idStr::Icmp( _name, "weaponCount" ) == 0 )
	{
		weaponCount = src->ParseInt();
		for( int i = 0; i < weaponCount; i++ )
		{
			SSDWeaponData_t newWeapon;
			memset( &newWeapon, 0, sizeof( SSDWeaponData_t ) );
			weaponData.Append( newWeapon );
		}
		return true;
	}

	if( idStr::FindText( _name, "leveldata", false ) >= 0 )
	{
		idStr tempName = _name;
		int level = atoi( tempName.Right( 2 ) ) - 1;

		idStr levelData;
		ParseString( src, levelData );
		ParseLevelData( level, levelData );
		return true;
	}

	if( idStr::FindText( _name, "asteroiddata", false ) >= 0 )
	{
		idStr tempName = _name;
		int level = atoi( tempName.Right( 2 ) ) - 1;

		idStr asteroidData;
		ParseString( src, asteroidData );
		ParseAsteroidData( level, asteroidData );
		return true;
	}

	if( idStr::FindText( _name, "weapondata", false ) >= 0 )
	{
		idStr tempName = _name;
		int weapon = atoi( tempName.Right( 2 ) ) - 1;

		idStr weaponData;
		ParseString( src, weaponData );
		ParseWeaponData( weapon, weaponData );
		return true;
	}

	if( idStr::FindText( _name, "astronautdata", false ) >= 0 )
	{
		idStr tempName = _name;
		int level = atoi( tempName.Right( 2 ) ) - 1;

		idStr astronautData;
		ParseString( src, astronautData );
		ParseAstronautData( level, astronautData );
		return true;
	}

	if( idStr::FindText( _name, "powerupdata", false ) >= 0 )
	{
		idStr tempName = _name;
		int level = atoi( tempName.Right( 2 ) ) - 1;

		idStr powerupData;
		ParseString( src, powerupData );
		ParsePowerupData( level, powerupData );
		return true;
	}

	return idWindow::ParseInternalVar( _name, src );
}

void idGameSSDWindow::ParseLevelData( int level, const idStr& levelDataString )
{

	idParser parser;
	idToken token;
	parser.LoadMemory( levelDataString.c_str(), levelDataString.Length(), "LevelData" );

	levelData[level].spawnBuffer = parser.ParseFloat();
	levelData[level].needToWin = parser.ParseInt(); //Required Destroyed

}

void idGameSSDWindow::ParseAsteroidData( int level, const idStr& asteroidDataString )
{

	idParser parser;
	idToken token;
	parser.LoadMemory( asteroidDataString.c_str(), asteroidDataString.Length(), "AsteroidData" );

	asteroidData[level].speedMin = parser.ParseFloat(); //Speed Min
	asteroidData[level].speedMax = parser.ParseFloat(); //Speed Max

	asteroidData[level].sizeMin = parser.ParseFloat(); //Size Min
	asteroidData[level].sizeMax = parser.ParseFloat(); //Size Max

	asteroidData[level].rotateMin = parser.ParseFloat(); //Rotate Min (rotations per second)
	asteroidData[level].rotateMax = parser.ParseFloat(); //Rotate Max (rotations per second)

	asteroidData[level].spawnMin = parser.ParseInt(); //Spawn Min
	asteroidData[level].spawnMax = parser.ParseInt(); //Spawn Max

	asteroidData[level].asteroidHealth = parser.ParseInt(); //Health of the asteroid
	asteroidData[level].asteroidDamage = parser.ParseInt(); //Asteroid Damage
	asteroidData[level].asteroidPoints = parser.ParseInt(); //Points awarded for destruction
}

void idGameSSDWindow::ParsePowerupData( int level, const idStr& powerupDataString )
{

	idParser parser;
	idToken token;
	parser.LoadMemory( powerupDataString.c_str(), powerupDataString.Length(), "PowerupData" );

	powerupData[level].speedMin = parser.ParseFloat(); //Speed Min
	powerupData[level].speedMax = parser.ParseFloat(); //Speed Max

	powerupData[level].rotateMin = parser.ParseFloat(); //Rotate Min (rotations per second)
	powerupData[level].rotateMax = parser.ParseFloat(); //Rotate Max (rotations per second)

	powerupData[level].spawnMin = parser.ParseInt(); //Spawn Min
	powerupData[level].spawnMax = parser.ParseInt(); //Spawn Max

}

void idGameSSDWindow::ParseWeaponData( int weapon, const idStr& weaponDataString )
{

	idParser parser;
	idToken token;
	parser.LoadMemory( weaponDataString.c_str(), weaponDataString.Length(), "WeaponData" );

	weaponData[weapon].speed = parser.ParseFloat();
	weaponData[weapon].damage = parser.ParseFloat();
	weaponData[weapon].size = parser.ParseFloat();
}

void idGameSSDWindow::ParseAstronautData( int level, const idStr& astronautDataString )
{

	idParser parser;
	idToken token;
	parser.LoadMemory( astronautDataString.c_str(), astronautDataString.Length(), "AstronautData" );

	astronautData[level].speedMin = parser.ParseFloat(); //Speed Min
	astronautData[level].speedMax = parser.ParseFloat(); //Speed Max

	astronautData[level].rotateMin = parser.ParseFloat(); //Rotate Min (rotations per second)
	astronautData[level].rotateMax = parser.ParseFloat(); //Rotate Max (rotations per second)

	astronautData[level].spawnMin = parser.ParseInt(); //Spawn Min
	astronautData[level].spawnMax = parser.ParseInt(); //Spawn Max

	astronautData[level].health = parser.ParseInt(); //Health of the asteroid
	astronautData[level].points = parser.ParseInt(); //Asteroid Damage
	astronautData[level].penalty = parser.ParseInt(); //Points awarded for destruction
}

void idGameSSDWindow::CommonInit()
{
	crosshair.InitCrosshairs();


	beginLevel = false;
	resetGame = false;
	continueGame = false;
	refreshGuiData = false;

	ssdTime = 0;
	levelCount = 0;
	weaponCount = 0;
	screenBounds = idBounds( idVec3( -320, -240, 0 ), idVec3( 320, 240, 0 ) );

	superBlasterTimeout = 0;

	currentSound = 0;

	//Precahce all assets that are loaded dynamically
	declManager->FindMaterial( ASTEROID_MATERIAL );
	declManager->FindMaterial( ASTRONAUT_MATERIAL );

	for( int i = 0; i < EXPLOSION_MATERIAL_COUNT; i++ )
	{
		declManager->FindMaterial( explosionMaterials[i] );
	}
	declManager->FindMaterial( PROJECTILE_MATERIAL );
	for( int i = 0; i < POWERUP_MATERIAL_COUNT; i++ )
	{
		declManager->FindMaterial( powerupMaterials[i][0] );
		declManager->FindMaterial( powerupMaterials[i][1] );
	}

	// Precache sounds
	declManager->FindSound( "arcade_blaster" );
	declManager->FindSound( "arcade_capture" );
	declManager->FindSound( "arcade_explode" );

	ResetGameStats();
}

void idGameSSDWindow::ResetGameStats()
{

	ResetEntities();

	//Reset the gamestats structure
	memset( &gameStats, 0, sizeof( gameStats ) );

	gameStats.health = 100;

}

void idGameSSDWindow::ResetLevelStats()
{

	ResetEntities();

	//Reset the level statistics structure
	memset( &gameStats.levelStats, 0, sizeof( gameStats.levelStats ) );


}

void idGameSSDWindow::ResetEntities()
{
	//Destroy all of the entities
	for( int i = 0; i < entities.Num(); i++ )
	{
		entities[i]->DestroyEntity();
	}
	entities.Clear();
}

void idGameSSDWindow::StartGame()
{

	gameStats.gameRunning = true;
}

void idGameSSDWindow::StopGame()
{

	gameStats.gameRunning = false;
}

void idGameSSDWindow::GameOver()
{


	StopGame();

	gui->HandleNamedEvent( "gameOver" );
}

void idGameSSDWindow::BeginLevel( int level )
{

	ResetLevelStats();

	gameStats.currentLevel = level;

	StartGame();
}

/**
* Continue game resets the players health
*/
void idGameSSDWindow::ContinueGame()
{
	gameStats.health = 100;

	StartGame();
}

void idGameSSDWindow::LevelComplete()
{

	gameStats.prebonusscore = gameStats.score;

	// Add the bonuses
	int accuracy;
	if( !gameStats.levelStats.shotCount )
	{
		accuracy = 0;
	}
	else
	{
		accuracy = ( int )( ( ( float )gameStats.levelStats.hitCount / ( float )gameStats.levelStats.shotCount ) * 100.0f );
	}
	int accuracyPoints = Max( 0, accuracy - 50 ) * 20;

	gui->SetStateString( "player_accuracy_score", va( "%i", accuracyPoints ) );

	gameStats.score += accuracyPoints;

	int saveAccuracy;
	int totalAst = gameStats.levelStats.savedAstronauts + gameStats.levelStats.killedAstronauts;
	if( !totalAst )
	{
		saveAccuracy = 0;
	}
	else
	{
		saveAccuracy = ( int )( ( ( float )gameStats.levelStats.savedAstronauts / ( float )totalAst ) * 100.0f );
	}
	accuracyPoints = Max( 0, saveAccuracy - 50 ) * 20;

	gui->SetStateString( "save_accuracy_score", va( "%i", accuracyPoints ) );

	gameStats.score += accuracyPoints;



	StopSuperBlaster();

	gameStats.nextLevel++;

	if( gameStats.nextLevel >= levelCount )
	{
		//Have they beaten the game
		GameComplete();
	}
	else
	{

		//Make sure we don't go above the levelcount
		//min(gameStats.nextLevel, levelCount-1);

		StopGame();
		gui->HandleNamedEvent( "levelComplete" );
	}
}

void idGameSSDWindow::GameComplete()
{
	StopGame();
	gui->HandleNamedEvent( "gameComplete" );
}


void idGameSSDWindow::UpdateGame()
{

	//Check to see if and functions where called by the gui
	if( beginLevel == true )
	{
		beginLevel = false;
		BeginLevel( gameStats.nextLevel );
	}
	if( resetGame == true )
	{
		resetGame = false;
		ResetGameStats();
	}
	if( continueGame == true )
	{
		continueGame = false;
		ContinueGame();
	}
	if( refreshGuiData == true )
	{
		refreshGuiData = false;
		RefreshGuiData();
	}

	if( gameStats.gameRunning )
	{

		//We assume an upate every 16 milliseconds
		ssdTime += 16;

		if( superBlasterTimeout && ssdTime > superBlasterTimeout )
		{
			StopSuperBlaster();
		}

		//Find if we are targeting and enemy
		idVec2 cursor;
		//GetCursor(cursor);
		cursor.x = gui->CursorX();
		cursor.y = gui->CursorY();
		gameStats.levelStats.targetEnt = EntityHitTest( cursor );

		//Update from back to front
		for( int i = entities.Num() - 1; i >= 0; i-- )
		{
			entities[i]->Update();
		}

		CheckForHits();

		//Delete entities that need to be deleted
		for( int i = entities.Num() - 1; i >= 0; i-- )
		{
			if( entities[i]->destroyed )
			{
				SSDEntity* ent = entities[i];
				ent->DestroyEntity();
				entities.RemoveIndex( i );
			}
		}

		//Check if we can spawn an asteroid
		SpawnAsteroid();

		//Check if we should spawn an astronaut
		SpawnAstronaut();

		//Check if we should spawn an asteroid
		SpawnPowerup();
	}
}

void idGameSSDWindow::CheckForHits()
{

	//See if the entity has gotten close enough
	for( int i = 0; i < entities.Num(); i++ )
	{
		SSDEntity* ent = entities[i];
		if( ent->position.z <= Z_NEAR )
		{

			if( !ent->noPlayerDamage )
			{

				//Is the object still in the screen
				idVec3 entPos = ent->position;
				entPos.z = 0;

				idBounds entBounds( entPos );
				entBounds.ExpandSelf( ent->hitRadius );

				if( screenBounds.IntersectsBounds( entBounds ) )
				{

					ent->OnStrikePlayer();

					//The entity hit the player figure out what is was and act appropriately
					if( ent->type == SSD_ENTITY_ASTEROID )
					{
						AsteroidStruckPlayer( static_cast<SSDAsteroid*>( ent ) );
					}
					else if( ent->type == SSD_ENTITY_ASTRONAUT )
					{
						AstronautStruckPlayer( static_cast<SSDAstronaut*>( ent ) );
					}
				}
				else
				{
					//Tag for removal later in the frame
					ent->destroyed = true;
				}
			}
		}
	}
}

void idGameSSDWindow::ZOrderEntities()
{
	//Z-Order the entities
	//Using a simple sorting method
	for( int i = entities.Num() - 1; i >= 0; i-- )
	{
		bool flipped = false;
		for( int j = 0;  j < i ; j++ )
		{
			if( entities[j]->position.z > entities[j + 1]->position.z )
			{
				SSDEntity* ent = entities[j];
				entities[j] = entities[j + 1];
				entities[j + 1] = ent;
				flipped = true;
			}
		}
		if( !flipped )
		{
			//Jump out because it is sorted
			break;
		}
	}
}

void idGameSSDWindow::SpawnAsteroid()
{

	int currentTime = ssdTime;

	if( currentTime < gameStats.levelStats.nextAsteroidSpawnTime )
	{
		//Not time yet
		return;
	}

	//Lets spawn it
	idVec3 startPosition;

	float spawnBuffer = levelData[gameStats.currentLevel].spawnBuffer * 2.0f;
	startPosition.x = random.RandomInt( V_WIDTH + spawnBuffer ) - ( ( V_WIDTH / 2.0f ) + spawnBuffer );
	startPosition.y = random.RandomInt( V_HEIGHT + spawnBuffer ) - ( ( V_HEIGHT / 2.0f ) + spawnBuffer );
	startPosition.z = ENTITY_START_DIST;

	float speed = random.RandomInt( asteroidData[gameStats.currentLevel].speedMax - asteroidData[gameStats.currentLevel].speedMin ) + asteroidData[gameStats.currentLevel].speedMin;
	float size = random.RandomInt( asteroidData[gameStats.currentLevel].sizeMax - asteroidData[gameStats.currentLevel].sizeMin ) + asteroidData[gameStats.currentLevel].sizeMin;
	float rotate = ( random.RandomFloat() * ( asteroidData[gameStats.currentLevel].rotateMax - asteroidData[gameStats.currentLevel].rotateMin ) ) + asteroidData[gameStats.currentLevel].rotateMin;

	SSDAsteroid* asteroid = SSDAsteroid::GetNewAsteroid( this, startPosition, idVec2( size, size ), speed, rotate, asteroidData[gameStats.currentLevel].asteroidHealth );
	entities.Append( asteroid );

	gameStats.levelStats.nextAsteroidSpawnTime = currentTime + random.RandomInt( asteroidData[gameStats.currentLevel].spawnMax - asteroidData[gameStats.currentLevel].spawnMin ) + asteroidData[gameStats.currentLevel].spawnMin;
}

void idGameSSDWindow::FireWeapon( int key )
{

	idVec2 cursorWorld = GetCursorWorld();
	idVec2 cursor;
	//GetCursor(cursor);
	cursor.x = gui->CursorX();
	cursor.y = gui->CursorY();

	if( key == K_MOUSE1 )
	{

		gameStats.levelStats.shotCount++;

		if( gameStats.levelStats.targetEnt )
		{
			//Aim the projectile from the bottom of the screen directly at the ent
			//SSDProjectile* newProj = new (TAG_OLD_UI) SSDProjectile(this, idVec3(320,0,0), gameStats.levelStats.targetEnt->position, weaponData[gameStats.currentWeapon].speed, weaponData[gameStats.currentWeapon].size);
			SSDProjectile* newProj = SSDProjectile::GetNewProjectile( this, idVec3( 0, -180, 0 ), gameStats.levelStats.targetEnt->position, weaponData[gameStats.currentWeapon].speed, weaponData[gameStats.currentWeapon].size );
			entities.Append( newProj );
			//newProj = SSDProjectile::GetNewProjectile(this, idVec3(-320,-0,0), gameStats.levelStats.targetEnt->position, weaponData[gameStats.currentWeapon].speed, weaponData[gameStats.currentWeapon].size);
			//entities.Append(newProj);

			//We hit something
			gameStats.levelStats.hitCount++;

			gameStats.levelStats.targetEnt->OnHit( key );

			if( gameStats.levelStats.targetEnt->type == SSD_ENTITY_ASTEROID )
			{
				HitAsteroid( static_cast<SSDAsteroid*>( gameStats.levelStats.targetEnt ), key );
			}
			else if( gameStats.levelStats.targetEnt->type == SSD_ENTITY_ASTRONAUT )
			{
				HitAstronaut( static_cast<SSDAstronaut*>( gameStats.levelStats.targetEnt ), key );
			}
		}
		else
		{
			////Aim the projectile at the cursor position all the way to the far clipping
			//SSDProjectile* newProj = SSDProjectile::GetNewProjectile(this, idVec3(0,-180,0), idVec3(cursorWorld.x, cursorWorld.y, (Z_FAR-Z_NEAR)/2.0f), weaponData[gameStats.currentWeapon].speed, weaponData[gameStats.currentWeapon].size);

			//Aim the projectile so it crosses the cursor 1/4 of screen
			idVec3 vec = idVec3( cursorWorld.x, cursorWorld.y, ( Z_FAR - Z_NEAR ) / 8.0f );
			vec *= 8;
			SSDProjectile* newProj = SSDProjectile::GetNewProjectile( this, idVec3( 0, -180, 0 ), vec, weaponData[gameStats.currentWeapon].speed, weaponData[gameStats.currentWeapon].size );
			entities.Append( newProj );

		}


		//Play the blaster sound
		PlaySound( "arcade_blaster" );

	} /*else if (key == K_MOUSE2) {
		if(gameStats.levelStats.targetEnt) {
			if(gameStats.levelStats.targetEnt->type == SSD_ENTITY_ASTRONAUT) {
				HitAstronaut(static_cast<SSDAstronaut*>(gameStats.levelStats.targetEnt), key);
			}
		}
	}*/
}

SSDEntity* idGameSSDWindow::EntityHitTest( const idVec2& pt )
{

	for( int i = 0; i < entities.Num(); i++ )
	{
		//Since we ZOrder the entities every frame we can stop at the first entity we hit.
		//ToDo: Make sure this assumption is true
		if( entities[i]->HitTest( pt ) )
		{
			return entities[i];
		}
	}
	return NULL;
}

void idGameSSDWindow::HitAsteroid( SSDAsteroid* asteroid, int key )
{



	asteroid->health -= weaponData[gameStats.currentWeapon].damage;

	if( asteroid->health <= 0 )
	{

		//The asteroid has been destroyed
		SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, asteroid->position, asteroid->size * 2, 300, SSDExplosion::EXPLOSION_NORMAL, asteroid );
		entities.Append( explosion );
		PlaySound( "arcade_explode" );

		AddScore( asteroid, asteroidData[gameStats.currentLevel].asteroidPoints );

		//Don't let the player hit it anymore because
		asteroid->noHit = true;

		gameStats.levelStats.destroyedAsteroids++;
		//if(gameStats.levelStats.destroyedAsteroids >= levelData[gameStats.currentLevel].needToWin) {
		//	LevelComplete();
		//}

	}
	else
	{
		//This was a damage hit so create a real small quick explosion
		SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, asteroid->position, asteroid->size / 2.0f, 200, SSDExplosion::EXPLOSION_NORMAL, asteroid, false, false );
		entities.Append( explosion );
	}
}

void idGameSSDWindow::AsteroidStruckPlayer( SSDAsteroid* asteroid )
{

	asteroid->noPlayerDamage = true;
	asteroid->noHit = true;

	AddDamage( asteroidData[gameStats.currentLevel].asteroidDamage );

	SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, asteroid->position, asteroid->size * 2, 300, SSDExplosion::EXPLOSION_NORMAL, asteroid );
	entities.Append( explosion );
	PlaySound( "arcade_explode" );
}

void idGameSSDWindow::AddScore( SSDEntity* ent, int points )
{

	SSDPoints* pointsEnt;

	if( points > 0 )
	{
		pointsEnt = SSDPoints::GetNewPoints( this, ent, points, 1000, 50, idVec4( 0, 1, 0, 1 ) );
	}
	else
	{
		pointsEnt = SSDPoints::GetNewPoints( this, ent, points, 1000, 50, idVec4( 1, 0, 0, 1 ) );
	}
	entities.Append( pointsEnt );

	gameStats.score += points;
	gui->SetStateString( "player_score", va( "%i", gameStats.score ) );
}

void idGameSSDWindow::AddDamage( int damage )
{
	gameStats.health -= damage;
	gui->SetStateString( "player_health", va( "%i", gameStats.health ) );

	gui->HandleNamedEvent( "playerDamage" );

	if( gameStats.health <= 0 )
	{
		//The player is dead
		GameOver();
	}
}

void idGameSSDWindow::AddHealth( int health )
{
	gameStats.health += health;
	gameStats.health = Min( 100, gameStats.health );
}


void idGameSSDWindow::OnNuke()
{

	gui->HandleNamedEvent( "nuke" );

	//Destory All Asteroids
	for( int i = 0 ; i < entities.Num(); i++ )
	{

		if( entities[i]->type == SSD_ENTITY_ASTEROID )
		{

			//The asteroid has been destroyed
			SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, entities[i]->position, entities[i]->size * 2, 300, SSDExplosion::EXPLOSION_NORMAL, entities[i] );
			entities.Append( explosion );

			AddScore( entities[i], asteroidData[gameStats.currentLevel].asteroidPoints );

			//Don't let the player hit it anymore because
			entities[i]->noHit = true;

			gameStats.levelStats.destroyedAsteroids++;
		}
	}
	PlaySound( "arcade_explode" );

	//Check to see if a nuke ends the level
	/*if(gameStats.levelStats.destroyedAsteroids >= levelData[gameStats.currentLevel].needToWin) {
		LevelComplete();

	}*/
}

void idGameSSDWindow::OnRescueAll()
{

	gui->HandleNamedEvent( "rescueAll" );

	//Rescue All Astronauts
	for( int i = 0 ; i < entities.Num(); i++ )
	{

		if( entities[i]->type == SSD_ENTITY_ASTRONAUT )
		{

			AstronautStruckPlayer( ( SSDAstronaut* )entities[i] );
		}
	}
}

void idGameSSDWindow::OnSuperBlaster()
{

	StartSuperBlaster();
}



void idGameSSDWindow::RefreshGuiData()
{


	gui->SetStateString( "nextLevel", va( "%i", gameStats.nextLevel + 1 ) );
	gui->SetStateString( "currentLevel", va( "%i", gameStats.currentLevel + 1 ) );

	float accuracy;
	if( !gameStats.levelStats.shotCount )
	{
		accuracy = 0;
	}
	else
	{
		accuracy = ( ( float )gameStats.levelStats.hitCount / ( float )gameStats.levelStats.shotCount ) * 100.0f;
	}
	gui->SetStateString( "player_accuracy", va( "%d%%", ( int )accuracy ) );

	float saveAccuracy;
	int totalAst = gameStats.levelStats.savedAstronauts + gameStats.levelStats.killedAstronauts;

	if( !totalAst )
	{
		saveAccuracy = 0;
	}
	else
	{
		saveAccuracy = ( ( float )gameStats.levelStats.savedAstronauts / ( float )totalAst ) * 100.0f;
	}
	gui->SetStateString( "save_accuracy", va( "%d%%", ( int )saveAccuracy ) );




	if( gameStats.levelStats.targetEnt )
	{
		int dist = ( gameStats.levelStats.targetEnt->position.z / 100.0f );
		dist *= 100;
		gui->SetStateString( "target_info", va( "%i meters", dist ) );
	}
	else
	{
		gui->SetStateString( "target_info", "No Target" );
	}

	gui->SetStateString( "player_health", va( "%i", gameStats.health ) );
	gui->SetStateString( "player_score", va( "%i", gameStats.score ) );
	gui->SetStateString( "player_prebonusscore", va( "%i", gameStats.prebonusscore ) );
	gui->SetStateString( "level_complete", va( "%i/%i", gameStats.levelStats.savedAstronauts, levelData[gameStats.currentLevel].needToWin ) );


	if( superBlasterTimeout )
	{
		float timeRemaining = ( superBlasterTimeout - ssdTime ) / 1000.0f;
		gui->SetStateString( "super_blaster_time", va( "%.2f", timeRemaining ) );
	}
}

idVec2 idGameSSDWindow::GetCursorWorld()
{

	idVec2 cursor;
	//GetCursor(cursor);
	cursor.x = gui->CursorX();
	cursor.y = gui->CursorY();
	cursor.x = cursor.x - 0.5f * V_WIDTH;
	cursor.y = -( cursor.y  - 0.5f * V_HEIGHT );
	return cursor;
}

void idGameSSDWindow::SpawnAstronaut()
{

	int currentTime = ssdTime;

	if( currentTime < gameStats.levelStats.nextAstronautSpawnTime )
	{
		//Not time yet
		return;
	}

	//Lets spawn it
	idVec3 startPosition;

	startPosition.x = random.RandomInt( V_WIDTH ) - ( V_WIDTH / 2.0f );
	startPosition.y = random.RandomInt( V_HEIGHT ) - ( V_HEIGHT / 2.0f );
	startPosition.z = ENTITY_START_DIST;

	float speed = random.RandomInt( astronautData[gameStats.currentLevel].speedMax - astronautData[gameStats.currentLevel].speedMin ) + astronautData[gameStats.currentLevel].speedMin;
	float rotate = ( random.RandomFloat() * ( astronautData[gameStats.currentLevel].rotateMax - astronautData[gameStats.currentLevel].rotateMin ) ) + astronautData[gameStats.currentLevel].rotateMin;

	SSDAstronaut* astronaut = SSDAstronaut::GetNewAstronaut( this, startPosition, speed, rotate, astronautData[gameStats.currentLevel].health );
	entities.Append( astronaut );

	gameStats.levelStats.nextAstronautSpawnTime = currentTime + random.RandomInt( astronautData[gameStats.currentLevel].spawnMax - astronautData[gameStats.currentLevel].spawnMin ) + astronautData[gameStats.currentLevel].spawnMin;
}

void idGameSSDWindow::HitAstronaut( SSDAstronaut* astronaut, int key )
{


	if( key == K_MOUSE1 )
	{
		astronaut->health -= weaponData[gameStats.currentWeapon].damage;

		if( astronaut->health <= 0 )
		{

			gameStats.levelStats.killedAstronauts++;

			//The astronaut has been destroyed
			SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, astronaut->position, astronaut->size * 2, 300, SSDExplosion::EXPLOSION_NORMAL, astronaut );
			entities.Append( explosion );
			PlaySound( "arcade_explode" );

			//Add the penalty for killing the astronaut
			AddScore( astronaut, astronautData[gameStats.currentLevel].penalty );

			//Don't let the player hit it anymore
			astronaut->noHit = true;
		}
		else
		{
			//This was a damage hit so create a real small quick explosion
			SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, astronaut->position, astronaut->size / 2.0f, 200, SSDExplosion::EXPLOSION_NORMAL, astronaut, false, false );
			entities.Append( explosion );
		}
	}
}

void idGameSSDWindow::AstronautStruckPlayer( SSDAstronaut* astronaut )
{

	gameStats.levelStats.savedAstronauts++;

	astronaut->noPlayerDamage = true;
	astronaut->noHit = true;

	//We are saving an astronaut
	SSDExplosion* explosion = SSDExplosion::GetNewExplosion( this, astronaut->position, astronaut->size * 2, 300, SSDExplosion::EXPLOSION_TELEPORT, astronaut );
	entities.Append( explosion );
	PlaySound( "arcade_capture" );

	//Give the player points for saving the astronaut
	AddScore( astronaut, astronautData[gameStats.currentLevel].points );

	if( gameStats.levelStats.savedAstronauts >= levelData[gameStats.currentLevel].needToWin )
	{
		LevelComplete();
	}

}

void idGameSSDWindow::SpawnPowerup()
{

	int currentTime = ssdTime;

	if( currentTime < gameStats.levelStats.nextPowerupSpawnTime )
	{
		//Not time yet
		return;
	}

	float speed = random.RandomInt( powerupData[gameStats.currentLevel].speedMax - powerupData[gameStats.currentLevel].speedMin ) + powerupData[gameStats.currentLevel].speedMin;
	float rotate = ( random.RandomFloat() * ( powerupData[gameStats.currentLevel].rotateMax - powerupData[gameStats.currentLevel].rotateMin ) ) + powerupData[gameStats.currentLevel].rotateMin;

	SSDPowerup* powerup = SSDPowerup::GetNewPowerup( this, speed, rotate );
	entities.Append( powerup );

	gameStats.levelStats.nextPowerupSpawnTime = currentTime + random.RandomInt( powerupData[gameStats.currentLevel].spawnMax - powerupData[gameStats.currentLevel].spawnMin ) + powerupData[gameStats.currentLevel].spawnMin;

}

void idGameSSDWindow::StartSuperBlaster()
{

	gui->HandleNamedEvent( "startSuperBlaster" );
	gameStats.currentWeapon = 1;
	superBlasterTimeout = ssdTime + 10000;

}
void idGameSSDWindow::StopSuperBlaster()
{
	gui->HandleNamedEvent( "stopSuperBlaster" );
	gameStats.currentWeapon = 0;
	superBlasterTimeout = 0;

}

SSDEntity* idGameSSDWindow::GetSpecificEntity( int type, int id )
{
	SSDEntity* ent = NULL;
	switch( type )
	{
		case SSD_ENTITY_ASTEROID:
			ent = SSDAsteroid::GetSpecificAsteroid( id );
			break;
		case SSD_ENTITY_ASTRONAUT:
			ent = SSDAstronaut::GetSpecificAstronaut( id );
			break;
		case SSD_ENTITY_EXPLOSION:
			ent = SSDExplosion::GetSpecificExplosion( id );
			break;
		case SSD_ENTITY_POINTS:
			ent = SSDPoints::GetSpecificPoints( id );
			break;
		case SSD_ENTITY_PROJECTILE:
			ent = SSDProjectile::GetSpecificProjectile( id );
			break;
		case SSD_ENTITY_POWERUP:
			ent = SSDPowerup::GetSpecificPowerup( id );
			break;
	}
	return ent;
}

#define MAX_SOUND_CHANNEL 8

void idGameSSDWindow::PlaySound( const char* sound )
{

	common->SW()->PlayShaderDirectly( sound, currentSound );

	currentSound++;
	if( currentSound >= MAX_SOUND_CHANNEL )
	{
		currentSound = 0;
	}
}
