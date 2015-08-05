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

#include "Game_local.h"


/*
===============================================================================

  idTrigger

===============================================================================
*/

const idEventDef EV_Enable( "enable", NULL );
const idEventDef EV_Disable( "disable", NULL );

CLASS_DECLARATION( idEntity, idTrigger )
EVENT( EV_Enable,	idTrigger::Event_Enable )
EVENT( EV_Disable,	idTrigger::Event_Disable )
END_CLASS

/*
================
idTrigger::DrawDebugInfo
================
*/
void idTrigger::DrawDebugInfo()
{
	idMat3		axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();
	idVec3		up = axis[ 2 ] * 5.0f;
	idBounds	viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds	viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds	box( idVec3( -4.0f, -4.0f, -4.0f ), idVec3( 4.0f, 4.0f, 4.0f ) );
	idEntity*	ent;
	idEntity*	target;
	int			i;
	bool		show;
	const function_t* func;
	
	viewTextBounds.ExpandSelf( 128.0f );
	viewBounds.ExpandSelf( 512.0f );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->GetPhysics()->GetContents() & ( CONTENTS_TRIGGER | CONTENTS_FLASHLIGHT_TRIGGER ) )
		{
			show = viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() );
			if( !show )
			{
				for( i = 0; i < ent->targets.Num(); i++ )
				{
					target = ent->targets[ i ].GetEntity();
					if( target != NULL && viewBounds.IntersectsBounds( target->GetPhysics()->GetAbsBounds() ) )
					{
						show = true;
						break;
					}
				}
			}
			
			if( !show )
			{
				continue;
			}
			
			gameRenderWorld->DebugBounds( colorOrange, ent->GetPhysics()->GetAbsBounds() );
			if( viewTextBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
			{
				gameRenderWorld->DrawText( ent->name.c_str(), ent->GetPhysics()->GetAbsBounds().GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( ent->GetEntityDefName(), ent->GetPhysics()->GetAbsBounds().GetCenter() + up, 0.1f, colorWhite, axis, 1 );
				if( ent->IsType( idTrigger::Type ) )
				{
					func = static_cast<idTrigger*>( ent )->GetScriptFunction();
				}
				else
				{
					func = NULL;
				}
				
				if( func )
				{
					gameRenderWorld->DrawText( va( "call script '%s'", func->Name() ), ent->GetPhysics()->GetAbsBounds().GetCenter() - up, 0.1f, colorWhite, axis, 1 );
				}
			}
			
			for( i = 0; i < ent->targets.Num(); i++ )
			{
				target = ent->targets[ i ].GetEntity();
				if( target )
				{
					gameRenderWorld->DebugArrow( colorYellow, ent->GetPhysics()->GetAbsBounds().GetCenter(), target->GetPhysics()->GetOrigin(), 10, 0 );
					gameRenderWorld->DebugBounds( colorGreen, box, target->GetPhysics()->GetOrigin() );
					if( viewTextBounds.IntersectsBounds( target->GetPhysics()->GetAbsBounds() ) )
					{
						gameRenderWorld->DrawText( target->name.c_str(), target->GetPhysics()->GetAbsBounds().GetCenter(), 0.1f, colorWhite, axis, 1 );
					}
				}
			}
		}
	}
}

/*
================
idTrigger::Enable
================
*/
void idTrigger::Enable()
{
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	GetPhysics()->EnableClip();
}

/*
================
idTrigger::Disable
================
*/
void idTrigger::Disable()
{
	// we may be relinked if we're bound to another object, so clear the contents as well
	GetPhysics()->SetContents( 0 );
	GetPhysics()->DisableClip();
}

/*
================
idTrigger::CallScript
================
*/
void idTrigger::CallScript() const
{
	idThread* thread;
	
	if( scriptFunction )
	{
		thread = new idThread( scriptFunction );
		thread->DelayedStart( 0 );
	}
}

/*
================
idTrigger::GetScriptFunction
================
*/
const function_t* idTrigger::GetScriptFunction() const
{
	return scriptFunction;
}

/*
================
idTrigger::Save
================
*/
void idTrigger::Save( idSaveGame* savefile ) const
{
	if( scriptFunction )
	{
		savefile->WriteString( scriptFunction->Name() );
	}
	else
	{
		savefile->WriteString( "" );
	}
}

/*
================
idTrigger::Restore
================
*/
void idTrigger::Restore( idRestoreGame* savefile )
{
	idStr funcname;
	savefile->ReadString( funcname );
	if( funcname.Length() )
	{
		scriptFunction = gameLocal.program.FindFunction( funcname );
		if( scriptFunction == NULL )
		{
			gameLocal.Warning( "idTrigger_Multi '%s' at (%s) calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
		}
	}
	else
	{
		scriptFunction = NULL;
	}
}

/*
================
idTrigger::Event_Enable
================
*/
void idTrigger::Event_Enable()
{
	Enable();
}

/*
================
idTrigger::Event_Disable
================
*/
void idTrigger::Event_Disable()
{
	Disable();
}

/*
================
idTrigger::idTrigger
================
*/
idTrigger::idTrigger()
{
	scriptFunction = NULL;
}

/*
================
idTrigger::Spawn
================
*/
void idTrigger::Spawn()
{
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	
	idStr funcname = spawnArgs.GetString( "call", "" );
	if( funcname.Length() )
	{
		scriptFunction = gameLocal.program.FindFunction( funcname );
		if( scriptFunction == NULL )
		{
			gameLocal.Warning( "trigger '%s' at (%s) calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
		}
	}
	else
	{
		scriptFunction = NULL;
	}
}


/*
===============================================================================

  idTrigger_Multi

===============================================================================
*/

const idEventDef EV_TriggerAction( "<triggerAction>", "e" );

CLASS_DECLARATION( idTrigger, idTrigger_Multi )
EVENT( EV_Touch,			idTrigger_Multi::Event_Touch )
EVENT( EV_Activate,			idTrigger_Multi::Event_Trigger )
EVENT( EV_TriggerAction,	idTrigger_Multi::Event_TriggerAction )
END_CLASS


/*
================
idTrigger_Multi::idTrigger_Multi
================
*/
idTrigger_Multi::idTrigger_Multi()
{
	wait = 0.0f;
	random = 0.0f;
	delay = 0.0f;
	random_delay = 0.0f;
	nextTriggerTime = 0;
	removeItem = 0;
	touchClient = false;
	touchOther = false;
	triggerFirst = false;
	triggerWithSelf = false;
}

/*
================
idTrigger_Multi::Save
================
*/
void idTrigger_Multi::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteFloat( delay );
	savefile->WriteFloat( random_delay );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteString( requires );
	savefile->WriteInt( removeItem );
	savefile->WriteBool( touchClient );
	savefile->WriteBool( touchOther );
	savefile->WriteBool( triggerFirst );
	savefile->WriteBool( triggerWithSelf );
}

/*
================
idTrigger_Multi::Restore
================
*/
void idTrigger_Multi::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadFloat( delay );
	savefile->ReadFloat( random_delay );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadString( requires );
	savefile->ReadInt( removeItem );
	savefile->ReadBool( touchClient );
	savefile->ReadBool( touchOther );
	savefile->ReadBool( triggerFirst );
	savefile->ReadBool( triggerWithSelf );
}

/*
================
idTrigger_Multi::Spawn

"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"call" : Script function to call when triggered
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
================
*/
void idTrigger_Multi::Spawn()
{
	spawnArgs.GetFloat( "wait", "0.5", wait );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "delay", "0", delay );
	spawnArgs.GetFloat( "random_delay", "0", random_delay );
	
	if( random && ( random >= wait ) && ( wait >= 0 ) )
	{
		random = wait - 1;
		gameLocal.Warning( "idTrigger_Multi '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	if( random_delay && ( random_delay >= delay ) && ( delay >= 0 ) )
	{
		random_delay = delay - 1;
		gameLocal.Warning( "idTrigger_Multi '%s' at (%s) has random_delay >= delay", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	spawnArgs.GetString( "requires", "", requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );
	spawnArgs.GetBool( "triggerFirst", "0", triggerFirst );
	spawnArgs.GetBool( "triggerWithSelf", "0", triggerWithSelf );
	
	if( spawnArgs.GetBool( "anyTouch" ) )
	{
		touchClient = true;
		touchOther = true;
	}
	else if( spawnArgs.GetBool( "noTouch" ) )
	{
		touchClient = false;
		touchOther = false;
	}
	else if( spawnArgs.GetBool( "noClient" ) )
	{
		touchClient = false;
		touchOther = true;
	}
	else
	{
		touchClient = true;
		touchOther = false;
	}
	
	nextTriggerTime = 0;
	
	if( spawnArgs.GetBool( "flashlight_trigger" ) )
	{
		GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	}
	else
	{
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}
}

/*
================
idTrigger_Multi::CheckFacing
================
*/
bool idTrigger_Multi::CheckFacing( idEntity* activator )
{
	if( spawnArgs.GetBool( "facing" ) )
	{
		if( !activator->IsType( idPlayer::Type ) )
		{
			return true;
		}
		idPlayer* player = static_cast< idPlayer* >( activator );
		float dot = player->viewAngles.ToForward() * GetPhysics()->GetAxis()[0];
		float angle = RAD2DEG( idMath::ACos( dot ) );
		if( angle  > spawnArgs.GetFloat( "angleLimit", "30" ) )
		{
			return false;
		}
	}
	return true;
}


/*
================
idTrigger_Multi::TriggerAction
================
*/
void idTrigger_Multi::TriggerAction( idEntity* activator )
{
	ActivateTargets( triggerWithSelf ? this : activator );
	CallScript();
	
	if( wait >= 0 )
	{
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	}
	else
	{
		// we can't just remove (this) here, because this is a touch function
		// called while looping through area links...
		// If the player spawned inside the trigger, the player Spawn function called Think directly,
		// allowing for multiple triggers on a trigger_once.  Increasing the nextTriggerTime prevents it.
		nextTriggerTime = gameLocal.time + 99999;
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idTrigger_Multi::Event_TriggerAction
================
*/
void idTrigger_Multi::Event_TriggerAction( idEntity* activator )
{
	TriggerAction( activator );
}

/*
================
idTrigger_Multi::Event_Trigger

the trigger was just activated
activated should be the entity that originated the activation sequence (ie. the original target)
activator should be set to the activator so it can be held through a delay
so wait for the delay time before firing
================
*/
void idTrigger_Multi::Event_Trigger( idEntity* activator )
{
	if( nextTriggerTime > gameLocal.time )
	{
		// can't retrigger until the wait is over
		return;
	}
	
	// see if this trigger requires an item
	if( !gameLocal.RequirementMet( activator, requires, removeItem ) )
	{
		return;
	}
	
	if( !CheckFacing( activator ) )
	{
		return;
	}
	
	if( triggerFirst )
	{
		triggerFirst = false;
		return;
	}
	
	// don't allow it to trigger twice in a single frame
	nextTriggerTime = gameLocal.time + 1;
	
	if( delay > 0 )
	{
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, activator );
	}
	else
	{
		TriggerAction( activator );
	}
}

/*
================
idTrigger_Multi::Event_Touch
================
*/
void idTrigger_Multi::Event_Touch( idEntity* other, trace_t* trace )
{
	if( common->IsClient() )
	{
		return;
	}
	
	if( triggerFirst )
	{
		return;
	}
	
	bool player = other->IsType( idPlayer::Type );
	if( player )
	{
		if( !touchClient )
		{
			return;
		}
		if( static_cast< idPlayer* >( other )->spectating )
		{
			return;
		}
	}
	else if( !touchOther )
	{
		return;
	}
	
	if( nextTriggerTime > gameLocal.time )
	{
		// can't retrigger until the wait is over
		return;
	}
	
	// see if this trigger requires an item
	if( !gameLocal.RequirementMet( other, requires, removeItem ) )
	{
		return;
	}
	
	if( !CheckFacing( other ) )
	{
		return;
	}
	
	if( spawnArgs.GetBool( "toggleTriggerFirst" ) )
	{
		triggerFirst = true;
	}
	
	nextTriggerTime = gameLocal.time + 1;
	if( delay > 0 )
	{
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, other );
	}
	else
	{
		TriggerAction( other );
	}
}

/*
===============================================================================

  idTrigger_EntityName

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_EntityName )
EVENT( EV_Touch,			idTrigger_EntityName::Event_Touch )
EVENT( EV_Activate,			idTrigger_EntityName::Event_Trigger )
EVENT( EV_TriggerAction,	idTrigger_EntityName::Event_TriggerAction )
END_CLASS

/*
================
idTrigger_EntityName::idTrigger_EntityName
================
*/
idTrigger_EntityName::idTrigger_EntityName()
{
	wait = 0.0f;
	random = 0.0f;
	delay = 0.0f;
	random_delay = 0.0f;
	nextTriggerTime = 0;
	triggerFirst = false;
	testPartialName = false;
}

/*
================
idTrigger_EntityName::Save
================
*/
void idTrigger_EntityName::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteFloat( delay );
	savefile->WriteFloat( random_delay );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteBool( triggerFirst );
	savefile->WriteString( entityName );
	savefile->WriteBool( testPartialName );
}

/*
================
idTrigger_EntityName::Restore
================
*/
void idTrigger_EntityName::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadFloat( delay );
	savefile->ReadFloat( random_delay );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadBool( triggerFirst );
	savefile->ReadString( entityName );
	savefile->ReadBool( testPartialName );
}

/*
================
idTrigger_EntityName::Spawn
================
*/
void idTrigger_EntityName::Spawn()
{
	spawnArgs.GetFloat( "wait", "0.5", wait );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "delay", "0", delay );
	spawnArgs.GetFloat( "random_delay", "0", random_delay );
	
	if( random && ( random >= wait ) && ( wait >= 0 ) )
	{
		random = wait - 1;
		gameLocal.Warning( "idTrigger_EntityName '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	if( random_delay && ( random_delay >= delay ) && ( delay >= 0 ) )
	{
		random_delay = delay - 1;
		gameLocal.Warning( "idTrigger_EntityName '%s' at (%s) has random_delay >= delay", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	spawnArgs.GetBool( "triggerFirst", "0", triggerFirst );
	
	entityName = spawnArgs.GetString( "entityname" );
	if( !entityName.Length() )
	{
		gameLocal.Error( "idTrigger_EntityName '%s' at (%s) doesn't have 'entityname' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	nextTriggerTime = 0;
	
	if( !spawnArgs.GetBool( "noTouch" ) )
	{
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}
	
	testPartialName = spawnArgs.GetBool( "testPartialName", testPartialName );
}

/*
================
idTrigger_EntityName::TriggerAction
================
*/
void idTrigger_EntityName::TriggerAction( idEntity* activator )
{
	ActivateTargets( activator );
	CallScript();
	
	if( wait >= 0 )
	{
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	}
	else
	{
		// we can't just remove (this) here, because this is a touch function
		// called while looping through area links...
		nextTriggerTime = gameLocal.time + 1;
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idTrigger_EntityName::Event_TriggerAction
================
*/
void idTrigger_EntityName::Event_TriggerAction( idEntity* activator )
{
	TriggerAction( activator );
}

/*
================
idTrigger_EntityName::Event_Trigger

the trigger was just activated
activated should be the entity that originated the activation sequence (ie. the original target)
activator should be set to the activator so it can be held through a delay
so wait for the delay time before firing
================
*/
void idTrigger_EntityName::Event_Trigger( idEntity* activator )
{
	if( nextTriggerTime > gameLocal.time )
	{
		// can't retrigger until the wait is over
		return;
	}
	
	bool validEntity = false;
	if( activator )
	{
		if( testPartialName )
		{
			if( activator->name.Find( entityName, false ) >= 0 )
			{
				validEntity = true;
			}
		}
		if( activator->name == entityName )
		{
			validEntity = true;
		}
	}
	
	if( !validEntity )
	{
		return;
	}
	
	if( triggerFirst )
	{
		triggerFirst = false;
		return;
	}
	
	// don't allow it to trigger twice in a single frame
	nextTriggerTime = gameLocal.time + 1;
	
	if( delay > 0 )
	{
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, activator );
	}
	else
	{
		TriggerAction( activator );
	}
}

/*
================
idTrigger_EntityName::Event_Touch
================
*/
void idTrigger_EntityName::Event_Touch( idEntity* other, trace_t* trace )
{
	if( common->IsClient() )
	{
		return;
	}
	
	if( triggerFirst )
	{
		return;
	}
	
	if( nextTriggerTime > gameLocal.time )
	{
		// can't retrigger until the wait is over
		return;
	}
	
	bool validEntity = false;
	if( other )
	{
		if( testPartialName )
		{
			if( other->name.Find( entityName, false ) >= 0 )
			{
				validEntity = true;
			}
		}
		if( other->name == entityName )
		{
			validEntity = true;
		}
	}
	
	if( !validEntity )
	{
		return;
	}
	
	nextTriggerTime = gameLocal.time + 1;
	if( delay > 0 )
	{
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, other );
	}
	else
	{
		TriggerAction( other );
	}
}

/*
===============================================================================

  idTrigger_Timer

===============================================================================
*/

const idEventDef EV_Timer( "<timer>", NULL );

CLASS_DECLARATION( idTrigger, idTrigger_Timer )
EVENT( EV_Timer,		idTrigger_Timer::Event_Timer )
EVENT( EV_Activate,		idTrigger_Timer::Event_Use )
END_CLASS

/*
================
idTrigger_Timer::idTrigger_Timer
================
*/
idTrigger_Timer::idTrigger_Timer()
{
	random = 0.0f;
	wait = 0.0f;
	on = false;
	delay = 0.0f;
}

/*
================
idTrigger_Timer::Save
================
*/
void idTrigger_Timer::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( random );
	savefile->WriteFloat( wait );
	savefile->WriteBool( on );
	savefile->WriteFloat( delay );
	savefile->WriteString( onName );
	savefile->WriteString( offName );
}

/*
================
idTrigger_Timer::Restore
================
*/
void idTrigger_Timer::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( random );
	savefile->ReadFloat( wait );
	savefile->ReadBool( on );
	savefile->ReadFloat( delay );
	savefile->ReadString( onName );
	savefile->ReadString( offName );
}

/*
================
idTrigger_Timer::Spawn

Repeatedly fires its targets.
Can be turned on or off by using.
================
*/
void idTrigger_Timer::Spawn()
{
	spawnArgs.GetFloat( "random", "1", random );
	spawnArgs.GetFloat( "wait", "1", wait );
	spawnArgs.GetBool( "start_on", "0", on );
	spawnArgs.GetFloat( "delay", "0", delay );
	onName = spawnArgs.GetString( "onName" );
	offName = spawnArgs.GetString( "offName" );
	
	if( random >= wait && wait >= 0 )
	{
		random = wait - 0.001;
		gameLocal.Warning( "idTrigger_Timer '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}
	
	if( on )
	{
		PostEventSec( &EV_Timer, delay );
	}
}

/*
================
idTrigger_Timer::Enable
================
*/
void idTrigger_Timer::Enable()
{
	// if off, turn it on
	if( !on )
	{
		on = true;
		PostEventSec( &EV_Timer, delay );
	}
}

/*
================
idTrigger_Timer::Disable
================
*/
void idTrigger_Timer::Disable()
{
	// if on, turn it off
	if( on )
	{
		on = false;
		CancelEvents( &EV_Timer );
	}
}

/*
================
idTrigger_Timer::Event_Timer
================
*/
void idTrigger_Timer::Event_Timer()
{
	ActivateTargets( this );
	
	// set time before next firing
	if( wait >= 0.0f )
	{
		PostEventSec( &EV_Timer, wait + gameLocal.random.CRandomFloat() * random );
	}
}

/*
================
idTrigger_Timer::Event_Use
================
*/
void idTrigger_Timer::Event_Use( idEntity* activator )
{
	// if on, turn it off
	if( on )
	{
		if( offName.Length() && offName.Icmp( activator->GetName() ) )
		{
			return;
		}
		on = false;
		CancelEvents( &EV_Timer );
	}
	else
	{
		// turn it on
		if( onName.Length() && onName.Icmp( activator->GetName() ) )
		{
			return;
		}
		on = true;
		PostEventSec( &EV_Timer, delay );
	}
}

/*
===============================================================================

  idTrigger_Count

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Count )
EVENT( EV_Activate,	idTrigger_Count::Event_Trigger )
EVENT( EV_TriggerAction,	idTrigger_Count::Event_TriggerAction )
END_CLASS

/*
================
idTrigger_Count::idTrigger_Count
================
*/
idTrigger_Count::idTrigger_Count()
{
	goal = 0;
	count = 0;
	delay = 0.0f;
}

/*
================
idTrigger_Count::Save
================
*/
void idTrigger_Count::Save( idSaveGame* savefile ) const
{
	savefile->WriteInt( goal );
	savefile->WriteInt( count );
	savefile->WriteFloat( delay );
}

/*
================
idTrigger_Count::Restore
================
*/
void idTrigger_Count::Restore( idRestoreGame* savefile )
{
	savefile->ReadInt( goal );
	savefile->ReadInt( count );
	savefile->ReadFloat( delay );
}

/*
================
idTrigger_Count::Spawn
================
*/
void idTrigger_Count::Spawn()
{
	spawnArgs.GetInt( "count", "1", goal );
	spawnArgs.GetFloat( "delay", "0", delay );
	count = 0;
}

/*
================
idTrigger_Count::Event_Trigger
================
*/
void idTrigger_Count::Event_Trigger( idEntity* activator )
{
	// goal of -1 means trigger has been exhausted
	if( goal >= 0 )
	{
		count++;
		if( count >= goal )
		{
			if( spawnArgs.GetBool( "repeat" ) )
			{
				count = 0;
			}
			else
			{
				goal = -1;
			}
			PostEventSec( &EV_TriggerAction, delay, activator );
		}
	}
}

/*
================
idTrigger_Count::Event_TriggerAction
================
*/
void idTrigger_Count::Event_TriggerAction( idEntity* activator )
{
	ActivateTargets( activator );
	CallScript();
	if( goal == -1 )
	{
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
===============================================================================

  idTrigger_Hurt

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Hurt )
EVENT( EV_Touch,		idTrigger_Hurt::Event_Touch )
EVENT( EV_Activate,		idTrigger_Hurt::Event_Toggle )
END_CLASS


/*
================
idTrigger_Hurt::idTrigger_Hurt
================
*/
idTrigger_Hurt::idTrigger_Hurt()
{
	on = false;
	delay = 0.0f;
	nextTime = 0;
}

/*
================
idTrigger_Hurt::Save
================
*/
void idTrigger_Hurt::Save( idSaveGame* savefile ) const
{
	savefile->WriteBool( on );
	savefile->WriteFloat( delay );
	savefile->WriteInt( nextTime );
}

/*
================
idTrigger_Hurt::Restore
================
*/
void idTrigger_Hurt::Restore( idRestoreGame* savefile )
{
	savefile->ReadBool( on );
	savefile->ReadFloat( delay );
	savefile->ReadInt( nextTime );
}

/*
================
idTrigger_Hurt::Spawn

	Damages activator
	Can be turned on or off by using.
================
*/
void idTrigger_Hurt::Spawn()
{
	spawnArgs.GetBool( "on", "1", on );
	spawnArgs.GetFloat( "delay", "1.0", delay );
	nextTime = gameLocal.time;
	Enable();
}

/*
================
idTrigger_Hurt::Event_Touch
================
*/
void idTrigger_Hurt::Event_Touch( idEntity* other, trace_t* trace )
{
	const char* damage;
	
	if( common->IsClient() )
	{
		return;
	}
	
	if( on && other && gameLocal.time >= nextTime )
	{
		bool playerOnly = spawnArgs.GetBool( "playerOnly" );
		if( playerOnly )
		{
			if( !other->IsType( idPlayer::Type ) )
			{
				return;
			}
		}
		damage = spawnArgs.GetString( "def_damage", "damage_painTrigger" );
		
		idVec3 dir = vec3_origin;
		if( spawnArgs.GetBool( "kick_from_center", "0" ) )
		{
			dir = other->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();
		}
		other->Damage( NULL, NULL, dir, damage, 1.0f, INVALID_JOINT );
		
		ActivateTargets( other );
		CallScript();
		
		nextTime = gameLocal.time + SEC2MS( delay );
	}
}

/*
================
idTrigger_Hurt::Event_Toggle
================
*/
void idTrigger_Hurt::Event_Toggle( idEntity* activator )
{
	on = !on;
}


/*
===============================================================================

  idTrigger_Fade

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Fade )
EVENT( EV_Activate,		idTrigger_Fade::Event_Trigger )
END_CLASS

/*
================
idTrigger_Fade::Event_Trigger
================
*/
void idTrigger_Fade::Event_Trigger( idEntity* activator )
{
	idVec4		fadeColor;
	int			fadeTime;
	idPlayer*	player;
	
	player = gameLocal.GetLocalPlayer();
	if( player )
	{
		fadeColor = spawnArgs.GetVec4( "fadeColor", "0, 0, 0, 1" );
		fadeTime = SEC2MS( spawnArgs.GetFloat( "fadeTime", "0.5" ) );
		player->playerView.Fade( fadeColor, fadeTime );
		PostEventMS( &EV_ActivateTargets, fadeTime, activator );
	}
}

/*
===============================================================================

  idTrigger_Touch

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Touch )
EVENT( EV_Activate,		idTrigger_Touch::Event_Trigger )
END_CLASS


/*
================
idTrigger_Touch::idTrigger_Touch
================
*/
idTrigger_Touch::idTrigger_Touch()
{
	clipModel = NULL;
}

/*
================
idTrigger_Touch::Spawn
================
*/
void idTrigger_Touch::Spawn()
{
	// get the clip model
	clipModel = new( TAG_THREAD ) idClipModel( GetPhysics()->GetClipModel() );
	
	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );
	
	if( spawnArgs.GetBool( "start_on" ) )
	{
		BecomeActive( TH_THINK );
	}
}

/*
================
idTrigger_Touch::Save
================
*/
void idTrigger_Touch::Save( idSaveGame* savefile )
{
	savefile->WriteClipModel( clipModel );
}

/*
================
idTrigger_Touch::Restore
================
*/
void idTrigger_Touch::Restore( idRestoreGame* savefile )
{
	savefile->ReadClipModel( clipModel );
}

/*
================
idTrigger_Touch::TouchEntities
================
*/
void idTrigger_Touch::TouchEntities()
{
	int numClipModels, i;
	idBounds bounds;
	idClipModel* cm, *clipModelList[ MAX_GENTITIES ];
	
	if( clipModel == NULL || scriptFunction == NULL )
	{
		return;
	}
	
	bounds.FromTransformedBounds( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );
	
	for( i = 0; i < numClipModels; i++ )
	{
		cm = clipModelList[ i ];
		
		if( !cm->IsTraceModel() )
		{
			continue;
		}
		
		idEntity* entity = cm->GetEntity();
		
		if( !entity )
		{
			continue;
		}
		
		if( !gameLocal.clip.ContentsModel( cm->GetOrigin(), cm, cm->GetAxis(), -1,
										   clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis() ) )
		{
			continue;
		}
		
		ActivateTargets( entity );
		
		idThread* thread = new idThread();
		thread->CallFunction( entity, scriptFunction, false );
		thread->DelayedStart( 0 );
	}
}

/*
================
idTrigger_Touch::Think
================
*/
void idTrigger_Touch::Think()
{
	if( thinkFlags & TH_THINK )
	{
		TouchEntities();
	}
	idEntity::Think();
}

/*
================
idTrigger_Touch::Event_Trigger
================
*/
void idTrigger_Touch::Event_Trigger( idEntity* activator )
{
	if( thinkFlags & TH_THINK )
	{
		BecomeInactive( TH_THINK );
	}
	else
	{
		BecomeActive( TH_THINK );
	}
}

/*
================
idTrigger_Touch::Enable
================
*/
void idTrigger_Touch::Enable()
{
	BecomeActive( TH_THINK );
}

/*
================
idTrigger_Touch::Disable
================
*/
void idTrigger_Touch::Disable()
{
	BecomeInactive( TH_THINK );
}

/*
===============================================================================

  idTrigger_Flag

===============================================================================
*/

CLASS_DECLARATION( idTrigger_Multi, idTrigger_Flag )
EVENT( EV_Touch, idTrigger_Flag::Event_Touch )
END_CLASS

idTrigger_Flag::idTrigger_Flag()
{
	team		= -1;
	player		= false;
	eventFlag	= NULL;
}

void idTrigger_Flag::Spawn()
{
	team = spawnArgs.GetInt( "team", "0" );
	player = spawnArgs.GetBool( "player", "0" );
	
	idStr funcname = spawnArgs.GetString( "eventflag", "" );
	if( funcname.Length() )
	{
		eventFlag = idEventDef::FindEvent( funcname );// gameLocal.program.FindFunction( funcname );//, &idItemTeam::Type );
		if( eventFlag == NULL )
		{
			gameLocal.Warning( "trigger '%s' at (%s) event unknown '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
		}
	}
	else
	{
		eventFlag = NULL;
	}
	
	idTrigger_Multi::Spawn();
}

void idTrigger_Flag::Event_Touch( idEntity* other, trace_t* trace )
{
	idItemTeam* flag = NULL;
	
	if( common->IsClient() )
	{
		return;
	}
	
	if( player )
	{
		if( !other->IsType( idPlayer::Type ) )
			return;
			
		idPlayer* player = static_cast<idPlayer*>( other );
		if( player->carryingFlag == false )
			return;
			
		if( team != -1 && ( player->team != team || ( player->team != 0 && player->team != 1 ) ) )
			return;
			
		idItemTeam* flags[2];
		
		flags[0] = gameLocal.mpGame.GetTeamFlag( 0 );
		flags[1] = gameLocal.mpGame.GetTeamFlag( 1 );
		
		int iFriend = 1 - player->team;			// index to the flag player team wants
		int iOpp	= player->team;				// index to the flag opp team wants
		
		// flag is captured if :
		// 1)flag is truely bound to the player
		// 2)opponent flag has been return
		if( flags[iFriend]->carried && !flags[iFriend]->dropped &&  //flags[iFriend]->IsBoundTo( player ) &&
				!flags[iOpp]->carried && !flags[iOpp]->dropped )
			flag = flags[iFriend];
		else
			return;
	}
	else
	{
		if( !other->IsType( idItemTeam::Type ) )
			return;
			
		idItemTeam* item = static_cast<idItemTeam*>( other );
		
		if( item->team == team || team == -1 )
		{
			flag = item;
		}
		else
			return;
	}
	
	if( flag )
	{
		switch( eventFlag->GetNumArgs() )
		{
			default :
			case 0 :
				flag->PostEventMS( eventFlag, 0 );
				break;
				
			// RB: 64 bit fixes, changed NULL to 0
			case 1 :
				flag->PostEventMS( eventFlag, 0, 0 );
				break;
			case 2 :
				flag->PostEventMS( eventFlag, 0, 0, 0 );
				break;
				// RB end
		}
		
		/*
				ServerSendEvent( eventFlag->GetEventNum(), NULL, true );
		
				idThread *thread;
				if ( scriptFlag ) {
					thread = new idThread();
					thread->CallFunction( flag, scriptFlag, false );
					thread->DelayedStart( 0 );
				}
		*/
		idTrigger_Multi::Event_Touch( other, trace );
	}
}
