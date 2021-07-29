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

#include "../Game_local.h"

CLASS_DECLARATION( idPhysics_Base, idPhysics_Parametric )
END_CLASS


/*
================
idPhysics_Parametric::Activate
================
*/
void idPhysics_Parametric::Activate()
{
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_Parametric::TestIfAtRest
================
*/
bool idPhysics_Parametric::TestIfAtRest() const
{

	if( ( current.linearExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
			( current.angularExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
			current.linearInterpolation.GetDuration() == 0 &&
			current.angularInterpolation.GetDuration() == 0 &&
			current.spline == NULL )
	{
		return true;
	}

	if( !current.linearExtrapolation.IsDone( current.time ) )
	{
		return false;
	}

	if( !current.angularExtrapolation.IsDone( current.time ) )
	{
		return false;
	}

	if( !current.linearInterpolation.IsDone( current.time ) )
	{
		return false;
	}

	if( !current.angularInterpolation.IsDone( current.time ) )
	{
		return false;
	}

	if( current.spline != NULL && !current.spline->IsDone( current.time ) )
	{
		return false;
	}

	return true;
}

/*
================
idPhysics_Parametric::Rest
================
*/
void idPhysics_Parametric::Rest()
{
	current.atRest = gameLocal.time;
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
idPhysics_Parametric::idPhysics_Parametric
================
*/
idPhysics_Parametric::idPhysics_Parametric()
{

	current.time = gameLocal.time;
	current.atRest = -1;
	current.useSplineAngles = false;
	current.origin.Zero();
	current.angles.Zero();
	current.axis.Identity();
	current.localOrigin.Zero();
	current.localAngles.Zero();
	current.linearExtrapolation.Init( 0, 0, vec3_zero, vec3_zero, vec3_zero, EXTRAPOLATION_NONE );
	current.angularExtrapolation.Init( 0, 0, ang_zero, ang_zero, ang_zero, EXTRAPOLATION_NONE );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	current.spline = NULL;
	current.splineInterpolate.Init( 0, 1, 1, 2, 0, 0 );

	saved = current;

	isPusher = false;
	pushFlags = 0;
	clipModel = NULL;
	isBlocked = false;
	memset( &pushResults, 0, sizeof( pushResults ) );

	hasMaster = false;
	isOrientated = false;
}

/*
================
idPhysics_Parametric::~idPhysics_Parametric
================
*/
idPhysics_Parametric::~idPhysics_Parametric()
{
	if( clipModel != NULL )
	{
		delete clipModel;
		clipModel = NULL;
	}
	if( current.spline != NULL )
	{
		delete current.spline;
		current.spline = NULL;
	}
}

/*
================
idPhysics_Parametric_SavePState
================
*/
void idPhysics_Parametric_SavePState( idSaveGame* savefile, const parametricPState_t& state )
{
	savefile->WriteInt( state.time );
	savefile->WriteInt( state.atRest );
	savefile->WriteBool( state.useSplineAngles );
	savefile->WriteVec3( state.origin );
	savefile->WriteAngles( state.angles );
	savefile->WriteMat3( state.axis );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteAngles( state.localAngles );

	savefile->WriteInt( ( int )state.linearExtrapolation.GetExtrapolationType() );
	savefile->WriteFloat( state.linearExtrapolation.GetStartTime() );
	savefile->WriteFloat( state.linearExtrapolation.GetDuration() );
	savefile->WriteVec3( state.linearExtrapolation.GetStartValue() );
	savefile->WriteVec3( state.linearExtrapolation.GetBaseSpeed() );
	savefile->WriteVec3( state.linearExtrapolation.GetSpeed() );

	savefile->WriteInt( ( int )state.angularExtrapolation.GetExtrapolationType() );
	savefile->WriteFloat( state.angularExtrapolation.GetStartTime() );
	savefile->WriteFloat( state.angularExtrapolation.GetDuration() );
	savefile->WriteAngles( state.angularExtrapolation.GetStartValue() );
	savefile->WriteAngles( state.angularExtrapolation.GetBaseSpeed() );
	savefile->WriteAngles( state.angularExtrapolation.GetSpeed() );

	savefile->WriteFloat( state.linearInterpolation.GetStartTime() );
	savefile->WriteFloat( state.linearInterpolation.GetAcceleration() );
	savefile->WriteFloat( state.linearInterpolation.GetDeceleration() );
	savefile->WriteFloat( state.linearInterpolation.GetDuration() );
	savefile->WriteVec3( state.linearInterpolation.GetStartValue() );
	savefile->WriteVec3( state.linearInterpolation.GetEndValue() );

	savefile->WriteFloat( state.angularInterpolation.GetStartTime() );
	savefile->WriteFloat( state.angularInterpolation.GetAcceleration() );
	savefile->WriteFloat( state.angularInterpolation.GetDeceleration() );
	savefile->WriteFloat( state.angularInterpolation.GetDuration() );
	savefile->WriteAngles( state.angularInterpolation.GetStartValue() );
	savefile->WriteAngles( state.angularInterpolation.GetEndValue() );

	// spline is handled by owner

	savefile->WriteFloat( state.splineInterpolate.GetStartTime() );
	savefile->WriteFloat( state.splineInterpolate.GetAcceleration() );
	savefile->WriteFloat( state.splineInterpolate.GetDuration() );
	savefile->WriteFloat( state.splineInterpolate.GetDeceleration() );
	savefile->WriteFloat( state.splineInterpolate.GetStartValue() );
	savefile->WriteFloat( state.splineInterpolate.GetEndValue() );
}

/*
================
idPhysics_Parametric_RestorePState
================
*/
void idPhysics_Parametric_RestorePState( idRestoreGame* savefile, parametricPState_t& state )
{
	extrapolation_t etype;
	float startTime, duration, accelTime, decelTime, startValue, endValue;
	idVec3 linearStartValue, linearBaseSpeed, linearSpeed, startPos, endPos;
	idAngles angularStartValue, angularBaseSpeed, angularSpeed, startAng, endAng;

	savefile->ReadInt( state.time );
	savefile->ReadInt( state.atRest );
	savefile->ReadBool( state.useSplineAngles );
	savefile->ReadVec3( state.origin );
	savefile->ReadAngles( state.angles );
	savefile->ReadMat3( state.axis );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadAngles( state.localAngles );

	savefile->ReadInt( ( int& )etype );
	savefile->ReadFloat( startTime );
	savefile->ReadFloat( duration );
	savefile->ReadVec3( linearStartValue );
	savefile->ReadVec3( linearBaseSpeed );
	savefile->ReadVec3( linearSpeed );

	state.linearExtrapolation.Init( startTime, duration, linearStartValue, linearBaseSpeed, linearSpeed, etype );

	savefile->ReadInt( ( int& )etype );
	savefile->ReadFloat( startTime );
	savefile->ReadFloat( duration );
	savefile->ReadAngles( angularStartValue );
	savefile->ReadAngles( angularBaseSpeed );
	savefile->ReadAngles( angularSpeed );

	state.angularExtrapolation.Init( startTime, duration, angularStartValue, angularBaseSpeed, angularSpeed, etype );

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( duration );
	savefile->ReadVec3( startPos );
	savefile->ReadVec3( endPos );

	state.linearInterpolation.Init( startTime, accelTime, decelTime, duration, startPos, endPos );

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( duration );
	savefile->ReadAngles( startAng );
	savefile->ReadAngles( endAng );

	state.angularInterpolation.Init( startTime, accelTime, decelTime, duration, startAng, endAng );

	// spline is handled by owner

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( duration );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( startValue );
	savefile->ReadFloat( endValue );

	state.splineInterpolate.Init( startTime, accelTime, decelTime, duration, startValue, endValue );
}

/*
================
idPhysics_Parametric::Save
================
*/
void idPhysics_Parametric::Save( idSaveGame* savefile ) const
{

	idPhysics_Parametric_SavePState( savefile, current );
	idPhysics_Parametric_SavePState( savefile, saved );

	savefile->WriteBool( isPusher );
	savefile->WriteClipModel( clipModel );
	savefile->WriteInt( pushFlags );

	savefile->WriteTrace( pushResults );
	savefile->WriteBool( isBlocked );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );
}

/*
================
idPhysics_Parametric::Restore
================
*/
void idPhysics_Parametric::Restore( idRestoreGame* savefile )
{

	idPhysics_Parametric_RestorePState( savefile, current );
	idPhysics_Parametric_RestorePState( savefile, saved );

	savefile->ReadBool( isPusher );
	savefile->ReadClipModel( clipModel );
	savefile->ReadInt( pushFlags );

	savefile->ReadTrace( pushResults );
	savefile->ReadBool( isBlocked );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );
}

/*
================
idPhysics_Parametric::SetPusher
================
*/
void idPhysics_Parametric::SetPusher( int flags )
{
	assert( clipModel );
	isPusher = true;
	pushFlags = flags;
}

/*
================
idPhysics_Parametric::IsPusher
================
*/
bool idPhysics_Parametric::IsPusher() const
{
	return isPusher;
}

/*
================
idPhysics_Parametric::SetLinearExtrapolation
================
*/
void idPhysics_Parametric::SetLinearExtrapolation( extrapolation_t type, int time, int duration, const idVec3& base, const idVec3& speed, const idVec3& baseSpeed )
{
	current.time = gameLocal.time;
	current.linearExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localOrigin = base;
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularExtrapolation
================
*/
void idPhysics_Parametric::SetAngularExtrapolation( extrapolation_t type, int time, int duration, const idAngles& base, const idAngles& speed, const idAngles& baseSpeed )
{
	current.time = gameLocal.time;
	current.angularExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localAngles = base;
	Activate();
}

/*
================
idPhysics_Parametric::GetLinearExtrapolationType
================
*/
extrapolation_t idPhysics_Parametric::GetLinearExtrapolationType() const
{
	return current.linearExtrapolation.GetExtrapolationType();
}

/*
================
idPhysics_Parametric::GetAngularExtrapolationType
================
*/
extrapolation_t idPhysics_Parametric::GetAngularExtrapolationType() const
{
	return current.angularExtrapolation.GetExtrapolationType();
}

/*
================
idPhysics_Parametric::SetLinearInterpolation
================
*/
void idPhysics_Parametric::SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const idVec3& startPos, const idVec3& endPos )
{
	current.time = gameLocal.time;
	current.linearInterpolation.Init( time, accelTime, decelTime, duration, startPos, endPos );
	current.localOrigin = startPos;
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularInterpolation
================
*/
void idPhysics_Parametric::SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const idAngles& startAng, const idAngles& endAng )
{
	current.time = gameLocal.time;
	current.angularInterpolation.Init( time, accelTime, decelTime, duration, startAng, endAng );
	current.localAngles = startAng;
	Activate();
}

/*
================
idPhysics_Parametric::SetSpline
================
*/
void idPhysics_Parametric::SetSpline( idCurve_Spline<idVec3>* spline, int accelTime, int decelTime, bool useSplineAngles )
{
	if( current.spline != NULL )
	{
		delete current.spline;
		current.spline = NULL;
	}
	current.spline = spline;
	if( current.spline != NULL )
	{
		float startTime = current.spline->GetTime( 0 );
		float endTime = current.spline->GetTime( current.spline->GetNumValues() - 1 );
		float length = current.spline->GetLengthForTime( endTime );
		current.splineInterpolate.Init( startTime, accelTime, decelTime, endTime - startTime, 0.0f, length );
	}
	current.useSplineAngles = useSplineAngles;
	Activate();
}

/*
================
idPhysics_Parametric::GetSpline
================
*/
idCurve_Spline<idVec3>* idPhysics_Parametric::GetSpline() const
{
	return current.spline;
}

/*
================
idPhysics_Parametric::GetSplineAcceleration
================
*/
int idPhysics_Parametric::GetSplineAcceleration() const
{
	return current.splineInterpolate.GetAcceleration();
}

/*
================
idPhysics_Parametric::GetSplineDeceleration
================
*/
int idPhysics_Parametric::GetSplineDeceleration() const
{
	return current.splineInterpolate.GetDeceleration();
}

/*
================
idPhysics_Parametric::UsingSplineAngles
================
*/
bool idPhysics_Parametric::UsingSplineAngles() const
{
	return current.useSplineAngles;
}

/*
================
idPhysics_Parametric::GetLocalOrigin
================
*/
void idPhysics_Parametric::GetLocalOrigin( idVec3& curOrigin ) const
{
	curOrigin = current.localOrigin;
}

/*
================
idPhysics_Parametric::GetLocalAngles
================
*/
void idPhysics_Parametric::GetLocalAngles( idAngles& curAngles ) const
{
	curAngles = current.localAngles;
}

/*
================
idPhysics_Parametric::SetClipModel
================
*/
void idPhysics_Parametric::SetClipModel( idClipModel* model, float density, int id, bool freeOld )
{

	assert( self );
	assert( model );

	if( clipModel && clipModel != model && freeOld )
	{
		delete clipModel;
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
}

/*
================
idPhysics_Parametric::GetClipModel
================
*/
idClipModel* idPhysics_Parametric::GetClipModel( int id ) const
{
	return clipModel;
}

/*
================
idPhysics_Parametric::GetNumClipModels
================
*/
int idPhysics_Parametric::GetNumClipModels() const
{
	return ( clipModel != NULL );
}

/*
================
idPhysics_Parametric::SetMass
================
*/
void idPhysics_Parametric::SetMass( float mass, int id )
{
}

/*
================
idPhysics_Parametric::GetMass
================
*/
float idPhysics_Parametric::GetMass( int id ) const
{
	return 0.0f;
}

/*
================
idPhysics_Parametric::SetClipMask
================
*/
void idPhysics_Parametric::SetContents( int contents, int id )
{
	if( clipModel )
	{
		clipModel->SetContents( contents );
	}
}

/*
================
idPhysics_Parametric::SetClipMask
================
*/
int idPhysics_Parametric::GetContents( int id ) const
{
	if( clipModel )
	{
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
idPhysics_Parametric::GetBounds
================
*/
const idBounds& idPhysics_Parametric::GetBounds( int id ) const
{
	if( clipModel )
	{
		return clipModel->GetBounds();
	}
	return idPhysics_Base::GetBounds();
}

/*
================
idPhysics_Parametric::GetAbsBounds
================
*/
const idBounds& idPhysics_Parametric::GetAbsBounds( int id ) const
{
	if( clipModel )
	{
		return clipModel->GetAbsBounds();
	}
	return idPhysics_Base::GetAbsBounds();
}

/*
================
idPhysics_Parametric::Evaluate
================
*/
bool idPhysics_Parametric::Evaluate( int timeStepMSec, int endTimeMSec )
{
	idVec3 oldLocalOrigin, oldOrigin, masterOrigin;
	idAngles oldLocalAngles, oldAngles;
	idMat3 oldAxis, masterAxis;

	isBlocked = false;
	oldLocalOrigin = current.localOrigin;
	oldOrigin = current.origin;
	oldLocalAngles = current.localAngles;
	oldAngles = current.angles;
	oldAxis = current.axis;

	current.localOrigin.Zero();
	current.localAngles.Zero();

	if( current.spline != NULL )
	{
		float length = current.splineInterpolate.GetCurrentValue( endTimeMSec );
		float t = current.spline->GetTimeForLength( length, 0.01f );
		current.localOrigin = current.spline->GetCurrentValue( t );
		if( current.useSplineAngles )
		{
			current.localAngles = current.spline->GetCurrentFirstDerivative( t ).ToAngles();
		}
	}
	else if( current.linearInterpolation.GetDuration() != 0 )
	{
		current.localOrigin += current.linearInterpolation.GetCurrentValue( endTimeMSec );
	}
	else
	{
		current.localOrigin += current.linearExtrapolation.GetCurrentValue( endTimeMSec );
	}

	if( current.angularInterpolation.GetDuration() != 0 )
	{
		current.localAngles += current.angularInterpolation.GetCurrentValue( endTimeMSec );
	}
	else
	{
		current.localAngles += current.angularExtrapolation.GetCurrentValue( endTimeMSec );
	}

	current.localAngles.Normalize360();
	current.origin = current.localOrigin;
	current.angles = current.localAngles;
	current.axis = current.localAngles.ToMat3();

	if( hasMaster )
	{
		self->GetMasterPosition( masterOrigin, masterAxis );
		if( masterAxis.IsRotated() )
		{
			current.origin = current.origin * masterAxis + masterOrigin;
			if( isOrientated )
			{
				current.axis *= masterAxis;
				current.angles = current.axis.ToAngles();
			}
		}
		else
		{
			current.origin += masterOrigin;
		}
	}

	if( isPusher )
	{

		gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, current.axis );
		if( pushResults.fraction < 1.0f )
		{
			if( clipModel )
			{
				clipModel->Link( gameLocal.clip, self, 0, oldOrigin, oldAxis );
			}
			current.localOrigin = oldLocalOrigin;
			current.origin = oldOrigin;
			current.localAngles = oldLocalAngles;
			current.angles = oldAngles;
			current.axis = oldAxis;
			isBlocked = true;
			return false;
		}

		current.angles = current.axis.ToAngles();
	}

	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}

	current.time = endTimeMSec;

	if( TestIfAtRest() )
	{
		Rest();
	}

	return ( current.origin != oldOrigin || current.axis != oldAxis );
}

/*
================
Sets the currentInterpolated state based on previous, next, and the fraction.
================
*/
bool idPhysics_Parametric::Interpolate( const float fraction )
{

	if( self->GetNumSnapshotsReceived() <= 1 )
	{
		return false;
	}

	idVec3 oldOrigin = current.origin;
	idMat3 oldAxis = current.axis;

	const bool hasChanged = InterpolatePhysicsState( current, previous, next, fraction );

	gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, current.axis );

	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}

	return hasChanged;
}

/*
================
idPhysics_Parametric::UpdateTime
================
*/
void idPhysics_Parametric::UpdateTime( int endTimeMSec )
{
	int timeLeap = endTimeMSec - current.time;

	current.time = endTimeMSec;
	// move the trajectory start times to sync the trajectory with the current endTime
	current.linearExtrapolation.SetStartTime( current.linearExtrapolation.GetStartTime() + timeLeap );
	current.angularExtrapolation.SetStartTime( current.angularExtrapolation.GetStartTime() + timeLeap );
	current.linearInterpolation.SetStartTime( current.linearInterpolation.GetStartTime() + timeLeap );
	current.angularInterpolation.SetStartTime( current.angularInterpolation.GetStartTime() + timeLeap );
	if( current.spline != NULL )
	{
		current.spline->ShiftTime( timeLeap );
		current.splineInterpolate.SetStartTime( current.splineInterpolate.GetStartTime() + timeLeap );
	}
}

/*
================
idPhysics_Parametric::GetTime
================
*/
int idPhysics_Parametric::GetTime() const
{
	return current.time;
}

/*
================
idPhysics_Parametric::IsAtRest
================
*/
bool idPhysics_Parametric::IsAtRest() const
{
	return current.atRest >= 0;
}

/*
================
idPhysics_Parametric::GetRestStartTime
================
*/
int idPhysics_Parametric::GetRestStartTime() const
{
	return current.atRest;
}

/*
================
idPhysics_Parametric::IsPushable
================
*/
bool idPhysics_Parametric::IsPushable() const
{
	return false;
}

/*
================
idPhysics_Parametric::SaveState
================
*/
void idPhysics_Parametric::SaveState()
{
	saved = current;
}

/*
================
idPhysics_Parametric::RestoreState
================
*/
void idPhysics_Parametric::RestoreState()
{

	current = saved;

	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Parametric::SetOrigin
================
*/
void idPhysics_Parametric::SetOrigin( const idVec3& newOrigin, int id )
{
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.linearExtrapolation.SetStartValue( newOrigin );
	current.linearInterpolation.SetStartValue( newOrigin );

	current.localOrigin = current.linearExtrapolation.GetCurrentValue( current.time );
	if( hasMaster )
	{
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	}
	else
	{
		current.origin = current.localOrigin;
	}
	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
	Activate();
}

/*
================
idPhysics_Parametric::SetAxis
================
*/
void idPhysics_Parametric::SetAxis( const idMat3& newAxis, int id )
{
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localAngles = newAxis.ToAngles();

	current.angularExtrapolation.SetStartValue( current.localAngles );
	current.angularInterpolation.SetStartValue( current.localAngles );

	current.localAngles = current.angularExtrapolation.GetCurrentValue( current.time );
	if( hasMaster && isOrientated )
	{
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = current.localAngles.ToMat3() * masterAxis;
		current.angles = current.axis.ToAngles();
	}
	else
	{
		current.axis = current.localAngles.ToMat3();
		current.angles = current.localAngles;
	}
	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
	Activate();
}

/*
================
idPhysics_Parametric::Move
================
*/
void idPhysics_Parametric::Translate( const idVec3& translation, int id )
{
}

/*
================
idPhysics_Parametric::Rotate
================
*/
void idPhysics_Parametric::Rotate( const idRotation& rotation, int id )
{
}

/*
================
idPhysics_Parametric::GetOrigin
================
*/
const idVec3& idPhysics_Parametric::GetOrigin( int id ) const
{
	return current.origin;
}

/*
================
idPhysics_Parametric::GetAxis
================
*/
const idMat3& idPhysics_Parametric::GetAxis( int id ) const
{
	return current.axis;
}

/*
================
idPhysics_Parametric::GetAngles
================
*/
void idPhysics_Parametric::GetAngles( idAngles& curAngles ) const
{
	curAngles = current.angles;
}

/*
================
idPhysics_Parametric::SetLinearVelocity
================
*/
void idPhysics_Parametric::SetLinearVelocity( const idVec3& newLinearVelocity, int id )
{
	SetLinearExtrapolation( extrapolation_t( EXTRAPOLATION_LINEAR | EXTRAPOLATION_NOSTOP ), gameLocal.time, 0, current.origin, newLinearVelocity, vec3_origin );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	Activate();
}

/*
================
idPhysics_Parametric::SetAngularVelocity
================
*/
void idPhysics_Parametric::SetAngularVelocity( const idVec3& newAngularVelocity, int id )
{
	idRotation rotation;
	idVec3 vec;
	float angle;

	vec = newAngularVelocity;
	angle = vec.Normalize();
	rotation.Set( vec3_origin, vec, ( float ) RAD2DEG( angle ) );

	SetAngularExtrapolation( extrapolation_t( EXTRAPOLATION_LINEAR | EXTRAPOLATION_NOSTOP ), gameLocal.time, 0, current.angles, rotation.ToAngles(), ang_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	Activate();
}

/*
================
idPhysics_Parametric::GetLinearVelocity
================
*/
const idVec3& idPhysics_Parametric::GetLinearVelocity( int id ) const
{
	static idVec3 curLinearVelocity;

	curLinearVelocity = current.linearExtrapolation.GetCurrentSpeed( gameLocal.time );
	return curLinearVelocity;
}

/*
================
idPhysics_Parametric::GetAngularVelocity
================
*/
const idVec3& idPhysics_Parametric::GetAngularVelocity( int id ) const
{
	static idVec3 curAngularVelocity;
	idAngles angles;

	angles = current.angularExtrapolation.GetCurrentSpeed( gameLocal.time );
	curAngularVelocity = angles.ToAngularVelocity();
	return curAngularVelocity;
}

/*
================
idPhysics_Parametric::DisableClip
================
*/
void idPhysics_Parametric::DisableClip()
{
	if( clipModel )
	{
		clipModel->Disable();
	}
}

/*
================
idPhysics_Parametric::EnableClip
================
*/
void idPhysics_Parametric::EnableClip()
{
	if( clipModel )
	{
		clipModel->Enable();
	}
}

/*
================
idPhysics_Parametric::UnlinkClip
================
*/
void idPhysics_Parametric::UnlinkClip()
{
	if( clipModel )
	{
		clipModel->Unlink();
	}
}

/*
================
idPhysics_Parametric::LinkClip
================
*/
void idPhysics_Parametric::LinkClip()
{
	if( clipModel )
	{
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Parametric::GetBlockingInfo
================
*/
const trace_t* idPhysics_Parametric::GetBlockingInfo() const
{
	return ( isBlocked ? &pushResults : NULL );
}

/*
================
idPhysics_Parametric::GetBlockingEntity
================
*/
idEntity* idPhysics_Parametric::GetBlockingEntity() const
{
	if( isBlocked )
	{
		return gameLocal.entities[ pushResults.c.entityNum ];
	}
	return NULL;
}

/*
================
idPhysics_Parametric::SetMaster
================
*/
void idPhysics_Parametric::SetMaster( idEntity* master, const bool orientated )
{
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if( master )
	{
		if( !hasMaster )
		{

			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			if( orientated )
			{
				current.localAngles = ( current.axis * masterAxis.Transpose() ).ToAngles();
			}
			else
			{
				current.localAngles = current.axis.ToAngles();
			}

			current.linearExtrapolation.SetStartValue( current.localOrigin );
			current.angularExtrapolation.SetStartValue( current.localAngles );
			hasMaster = true;
			isOrientated = orientated;
		}
	}
	else
	{
		if( hasMaster )
		{
			// transform from master space to world space
			current.localOrigin = current.origin;
			current.localAngles = current.angles;
			SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.origin, vec3_origin, vec3_origin );
			SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.angles, ang_zero, ang_zero );
			hasMaster = false;
		}
	}
}

/*
================
idPhysics_Parametric::GetLinearEndTime
================
*/
int idPhysics_Parametric::GetLinearEndTime() const
{
	if( current.spline != NULL )
	{
		if( current.spline->GetBoundaryType() != idCurve_Spline<idVec3>::BT_CLOSED )
		{
			return current.spline->GetTime( current.spline->GetNumValues() - 1 );
		}
		else
		{
			return 0;
		}
	}
	else if( current.linearInterpolation.GetDuration() != 0 )
	{
		return current.linearInterpolation.GetEndTime();
	}
	else
	{
		return current.linearExtrapolation.GetEndTime();
	}
}

/*
================
idPhysics_Parametric::GetAngularEndTime
================
*/
int idPhysics_Parametric::GetAngularEndTime() const
{
	if( current.angularInterpolation.GetDuration() != 0 )
	{
		return current.angularInterpolation.GetEndTime();
	}
	else
	{
		return current.angularExtrapolation.GetEndTime();
	}
}

/*
================
idPhysics_Parametric::WriteToSnapshot
================
*/
void idPhysics_Parametric::WriteToSnapshot( idBitMsg& msg ) const
{

	const idQuat currentQuat = current.axis.ToQuat();

	WriteFloatArray( msg, current.origin );
	WriteFloatArray( msg, currentQuat );
}

/*
================
idPhysics_Parametric::ReadFromSnapshot
================
*/
void idPhysics_Parametric::ReadFromSnapshot( const idBitMsg& msg )
{

	previous = next;

	next.origin = ReadFloatArray< idVec3 >( msg );
	next.axis = ReadFloatArray< idQuat >( msg );

	if( self->GetNumSnapshotsReceived() <= 1 )
	{
		current.origin = next.origin;
		previous.origin = next.origin;
		current.axis = next.axis.ToMat3();
		previous.axis = next.axis;
	}
}
