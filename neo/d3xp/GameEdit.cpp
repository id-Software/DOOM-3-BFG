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

	Ingame cursor.

===============================================================================
*/

CLASS_DECLARATION( idEntity, idCursor3D )
END_CLASS

/*
===============
idCursor3D::idCursor3D
===============
*/
idCursor3D::idCursor3D()
{
	draggedPosition.Zero();
}

/*
===============
idCursor3D::~idCursor3D
===============
*/
idCursor3D::~idCursor3D()
{
}

/*
===============
idCursor3D::Spawn
===============
*/
void idCursor3D::Spawn()
{
}

/*
===============
idCursor3D::Present
===============
*/
void idCursor3D::Present()
{
	// don't present to the renderer if the entity hasn't changed
	if( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	const idVec3& origin = GetPhysics()->GetOrigin();
	const idMat3& axis = GetPhysics()->GetAxis();
	gameRenderWorld->DebugArrow( colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2 );
	gameRenderWorld->DebugArrow( colorRed, origin, draggedPosition, 2 );
}

/*
===============
idCursor3D::Think
===============
*/
void idCursor3D::Think()
{
	if( thinkFlags & TH_THINK )
	{
		drag.Evaluate( gameLocal.time );
	}
	Present();
}


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

#define MAX_DRAG_TRACE_DISTANCE			2048.0f

/*
==============
idDragEntity::idDragEntity
==============
*/
idDragEntity::idDragEntity()
{
	cursor = NULL;
	Clear();
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idDragEntity::~idDragEntity()
{
	StopDrag();
	selected = NULL;
	delete cursor;
	cursor = NULL;
}


/*
==============
idDragEntity::Clear
==============
*/
void idDragEntity::Clear()
{
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= NULL;
}

/*
==============
idDragEntity::StopDrag
==============
*/
void idDragEntity::StopDrag()
{
	dragEnt = NULL;
	if( cursor )
	{
		cursor->BecomeInactive( TH_THINK );
	}
}

/*
==============
idDragEntity::Update
==============
*/
void idDragEntity::Update( idPlayer* player )
{
	idVec3 viewPoint, origin;
	idMat3 viewAxis, axis;
	trace_t trace;
	idEntity* newEnt = NULL;
	idAngles angles;
	jointHandle_t newJoint = INVALID_JOINT;
	idStr newBodyName;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
	if( !dragEnt.GetEntity() )
	{

		if( player->usercmd.buttons & BUTTON_ATTACK )
		{

			gameLocal.clip.TracePoint( trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, ( CONTENTS_SOLID | CONTENTS_RENDERMODEL | CONTENTS_BODY ), player );
			if( trace.fraction < 1.0f )
			{

				newEnt = gameLocal.entities[ trace.c.entityNum ];
				if( newEnt )
				{

					if( newEnt->GetBindMaster() )
					{
						if( newEnt->GetBindJoint() )
						{
							trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
						}
						else
						{
							trace.c.id = newEnt->GetBindBody();
						}
						newEnt = newEnt->GetBindMaster();
					}

					if( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base*>( newEnt )->IsActiveAF() )
					{
						idAFEntity_Base* af = static_cast<idAFEntity_Base*>( newEnt );

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

					}
					else if( !newEnt->IsType( idWorldspawn::Type ) )
					{

						if( trace.c.id < 0 )
						{
							newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						}
						else
						{
							newJoint = INVALID_JOINT;
						}
						newBodyName = "";

					}
					else
					{

						newJoint = INVALID_JOINT;
						newEnt = NULL;
					}
				}
				if( newEnt )
				{
					dragEnt = newEnt;
					selected = newEnt;
					joint = newJoint;
					id = trace.c.id;
					bodyName = newBodyName;

					if( !cursor )
					{
						cursor = ( idCursor3D* )gameLocal.SpawnEntityType( idCursor3D::Type );
					}

					idPhysics* phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat() );
					cursor->drag.SetPhysics( phys, id, localEntityPoint );
					cursor->Show();

					if( phys->IsType( idPhysics_AF::Type ) ||
							phys->IsType( idPhysics_RigidBody::Type ) ||
							phys->IsType( idPhysics_Monster::Type ) )
					{
						cursor->BecomeActive( TH_THINK );
					}
				}
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity* drag = dragEnt.GetEntity();
	if( drag )
	{

		if( !( player->usercmd.buttons & BUTTON_ATTACK ) )
		{
			StopDrag();
			return;
		}

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		renderEntity_t* renderEntity = drag->GetRenderEntity();
		idAnimator* dragAnimator = drag->GetAnimator();

		if( joint != INVALID_JOINT && renderEntity != NULL && dragAnimator != NULL )
		{
			dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
			cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
			gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
		else
		{
			cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
			gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
	}

	// if there is a selected entity
	if( selected.GetEntity() && g_dragShowSelection.GetBool() )
	{
		// draw the bbox of the selected entity
		renderEntity_t* renderEntity = selected.GetEntity()->GetRenderEntity();
		if( renderEntity )
		{
			gameRenderWorld->DebugBox( colorYellow, idBox( renderEntity->bounds, renderEntity->origin, renderEntity->axis ) );
		}
	}
}

/*
==============
idDragEntity::SetSelected
==============
*/
void idDragEntity::SetSelected( idEntity* ent )
{
	selected = ent;
	StopDrag();
}

/*
==============
idDragEntity::DeleteSelected
==============
*/
void idDragEntity::DeleteSelected()
{
	delete selected.GetEntity();
	selected = NULL;
	StopDrag();
}

/*
==============
idDragEntity::BindSelected
==============
*/
void idDragEntity::BindSelected()
{
	int num, largestNum;
	idLexer lexer;
	idToken type, bodyName;
	idStr key, value, bindBodyName;
	const idKeyValue* kv;
	idAFEntity_Base* af;

	af = static_cast<idAFEntity_Base*>( dragEnt.GetEntity() );

	if( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() )
	{
		return;
	}

	bindBodyName = af->GetAFPhysics()->GetBody( id )->GetName();
	largestNum = 1;

	// parse all the bind constraints
	kv = af->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while( kv )
	{
		key = kv->GetKey();
		key.Strip( "bindConstraint " );
		if( sscanf( key, "bind%d", &num ) )
		{
			if( num >= largestNum )
			{
				largestNum = num + 1;
			}
		}

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );
		lexer.ReadToken( &bodyName );
		lexer.FreeSource();

		// if there already exists a bind constraint for this body
		if( bodyName.Icmp( bindBodyName ) == 0 )
		{
			// delete the bind constraint
			af->spawnArgs.Delete( kv->GetKey() );
			kv = NULL;
		}

		kv = af->spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	sprintf( key, "bindConstraint bind%d", largestNum );
	sprintf( value, "ballAndSocket %s %s", bindBodyName.c_str(), af->GetAnimator()->GetJointName( joint ) );

	af->spawnArgs.Set( key, value );
	af->spawnArgs.Set( "bind", "worldspawn" );
	af->Bind( gameLocal.world, true );
}

/*
==============
idDragEntity::UnbindSelected
==============
*/
void idDragEntity::UnbindSelected()
{
	const idKeyValue* kv;
	idAFEntity_Base* af;

	af = static_cast<idAFEntity_Base*>( selected.GetEntity() );

	if( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() )
	{
		return;
	}

	// unbind the selected entity
	af->Unbind();

	// delete all the bind constraints
	kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while( kv )
	{
		selected.GetEntity()->spawnArgs.Delete( kv->GetKey() );
		kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	}

	// delete any bind information
	af->spawnArgs.Delete( "bind" );
	af->spawnArgs.Delete( "bindToJoint" );
	af->spawnArgs.Delete( "bindToBody" );
}


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/

/*
==============
idEditEntities::idEditEntities
==============
*/
idEditEntities::idEditEntities()
{
	selectableEntityClasses.Clear();
	nextSelectTime = 0;
}

/*
=============
idEditEntities::SelectEntity
=============
*/
bool idEditEntities::SelectEntity( const idVec3& origin, const idVec3& dir, const idEntity* skip )
{
	idVec3		end;
	idEntity*	ent;

	if( !g_editEntityMode.GetInteger() || selectableEntityClasses.Num() == 0 )
	{
		return false;
	}

	if( gameLocal.time < nextSelectTime )
	{
		return true;
	}
	nextSelectTime = gameLocal.time + 300;

	end = origin + dir * 4096.0f;

	ent = NULL;
	for( int i = 0; i < selectableEntityClasses.Num(); i++ )
	{
		ent = gameLocal.FindTraceEntity( origin, end, *selectableEntityClasses[i].typeInfo, skip );
		if( ent )
		{
			break;
		}
	}
	if( ent )
	{
		ClearSelectedEntities();
		if( EntityIsSelectable( ent ) )
		{
			AddSelectedEntity( ent );
			gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );
			ent->ShowEditingDialog();
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::AddSelectedEntity
=============
*/
void idEditEntities::AddSelectedEntity( idEntity* ent )
{
	ent->fl.selected = true;
	selectedEntities.AddUnique( ent );
}

/*
==============
idEditEntities::RemoveSelectedEntity
==============
*/
void idEditEntities::RemoveSelectedEntity( idEntity* ent )
{
	if( selectedEntities.Find( ent ) )
	{
		selectedEntities.Remove( ent );
	}
}

/*
=============
idEditEntities::ClearSelectedEntities
=============
*/
void idEditEntities::ClearSelectedEntities()
{
	int i, count;

	count = selectedEntities.Num();
	for( i = 0; i < count; i++ )
	{
		selectedEntities[i]->fl.selected = false;
	}
	selectedEntities.Clear();
}


/*
=============
idEditEntities::EntityIsSelectable
=============
*/
bool idEditEntities::EntityIsSelectable( idEntity* ent, idVec4* color, idStr* text )
{
	for( int i = 0; i < selectableEntityClasses.Num(); i++ )
	{
		if( ent->GetType() == selectableEntityClasses[i].typeInfo )
		{
			if( text )
			{
				*text = selectableEntityClasses[i].textKey;
			}
			if( color )
			{
				if( ent->fl.selected )
				{
					*color = colorRed;
				}
				else
				{
					switch( i )
					{
						case 1 :
							*color = colorYellow;
							break;
						case 2 :
							*color = colorBlue;
							break;
						default:
							*color = colorGreen;
					}
				}
			}
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::DisplayEntities
=============
*/
void idEditEntities::DisplayEntities()
{
	idEntity* ent;

	if( !gameLocal.GetLocalPlayer() )
	{
		return;
	}

	selectableEntityClasses.Clear();
	selectedTypeInfo_t sit;

	switch( g_editEntityMode.GetInteger() )
	{
		case 1:
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 2:
			sit.typeInfo = &idSound::Type;
			sit.textKey = "s_shader";
			selectableEntityClasses.Append( sit );
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 3:
			sit.typeInfo = &idAFEntity_Base::Type;
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 4:
			sit.typeInfo = &idFuncEmitter::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
			sit.typeInfo = &idAI::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 6:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 7:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		default:
			return;
	}

	idBounds viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	viewBounds.ExpandSelf( 512 );
	viewTextBounds.ExpandSelf( 128 );

	idStr textKey;

	r_singleLight.SetInteger( -1 );
	r_showLights.SetInteger( 0 );

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		idVec4 color;

		textKey = "";
		if( !EntityIsSelectable( ent, &color, &textKey ) )
		{
			continue;
		}

		bool drawArrows = false;
		if( ent->GetType() == &idAFEntity_Base::Type )
		{
			if( !static_cast<idAFEntity_Base*>( ent )->IsActiveAF() )
			{
				continue;
			}
		}
		else if( ent->GetType() == &idSound::Type )
		{
			if( ent->fl.selected )
			{
				drawArrows = true;
			}
			const idSoundShader* ss = declManager->FindSound( ent->spawnArgs.GetString( textKey ) );
			if( ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED )
			{
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
			}
		}
		else if( ent->GetType() == &idFuncEmitter::Type )
		{
			if( ent->fl.selected )
			{
				drawArrows = true;
			}
		}
		else if( ent->GetType() == &idLight::Type )
		{
			// RB: use renderer backend to display light properties
			if( ent->fl.selected )
			{
				drawArrows = true;

				idLight* light = static_cast<idLight*>( ent );

				r_singleLight.SetInteger( light->GetLightDefHandle() );
				r_showLights.SetInteger( 3 );

				renderLight_t renderLight = light->GetRenderLight();

				// draw arrow from entity origin to globalLightOrigin

				idVec3 globalLightOrigin;
				if( renderLight.parallel )
				{
					idVec3 dir = renderLight.lightCenter;
					if( dir.Normalize() == 0.0f )
					{
						// make point straight up if not specified
						dir[2] = 1.0f;
					}
					globalLightOrigin = renderLight.origin + dir * 100000.0f;
				}
				else
				{
					globalLightOrigin = renderLight.origin + renderLight.axis * renderLight.lightCenter;
				}

				idVec3 start = ent->GetPhysics()->GetOrigin();
				idVec3 end = globalLightOrigin;
				gameRenderWorld->DebugArrow( colorYellow, start, end, 2 );

				if( !renderLight.parallel )
				{
					gameRenderWorld->DrawText( "globalLightOrigin", end + idVec3( 4, 0, 0 ), 0.15f, colorYellow, axis );
				}
			}
		}

		if( !viewBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) )
		{
			continue;
		}

		gameRenderWorld->DebugBounds( color, idBounds( ent->GetPhysics()->GetOrigin() ).Expand( 8 ) );
		if( drawArrows )
		{
			idVec3 start = ent->GetPhysics()->GetOrigin();
			idVec3 end = start + idVec3( 1, 0, 0 ) * 20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x+", end + idVec3( 4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 1, 0, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x-", end + idVec3( -4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y+", end + idVec3( 0, 4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y-", end + idVec3( 0, -4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z+", end + idVec3( 0, 0, 4 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z-", end + idVec3( 0, 0, -4 ), 0.15f, colorWhite, axis );
		}

		if( textKey.Length() )
		{
			const char* text = ent->spawnArgs.GetString( textKey );
			if( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) )
			{
				gameRenderWorld->DrawText( text, ent->GetPhysics()->GetOrigin() + idVec3( 0, 0, 12 ), 0.25, colorWhite, axis, 1 );
			}
		}
	}
}


/*
===============================================================================

	idGameEdit

===============================================================================
*/

idGameEdit			gameEditLocal;
idGameEdit* 		gameEdit = &gameEditLocal;


/*
=============
idGameEdit::GetSelectedEntities
=============
*/
int idGameEdit::GetSelectedEntities( idEntity* list[], int max )
{
	int num = 0;
	idEntity* ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->fl.selected )
		{
			list[num++] = ent;
			if( num >= max )
			{
				break;
			}
		}
	}
	return num;
}

/*
=============
idGameEdit::TriggerSelected
=============
*/
void idGameEdit::TriggerSelected()
{
	idEntity* ent;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->fl.selected )
		{
			ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		}
	}
}

/*
================
idGameEdit::ClearEntitySelection
================
*/
void idGameEdit::ClearEntitySelection()
{
	idEntity* ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		ent->fl.selected = false;
	}
	gameLocal.editEntities->ClearSelectedEntities();
}

/*
================
idGameEdit::AddSelectedEntity
================
*/
void idGameEdit::AddSelectedEntity( idEntity* ent )
{
	if( ent )
	{
		gameLocal.editEntities->AddSelectedEntity( ent );
	}
}

/*
================
idGameEdit::FindEntityDefDict
================
*/
const idDict* idGameEdit::FindEntityDefDict( const char* name, bool makeDefault ) const
{
	return gameLocal.FindEntityDefDict( name, makeDefault );
}

/*
================
idGameEdit::SpawnEntityDef
================
*/
void idGameEdit::SpawnEntityDef( const idDict& args, idEntity** ent )
{
	gameLocal.SpawnEntityDef( args, ent );
}

/*
================
idGameEdit::FindEntity
================
*/
idEntity* idGameEdit::FindEntity( const char* name ) const
{
	return gameLocal.FindEntity( name );
}

/*
=============
idGameEdit::GetUniqueEntityName

generates a unique name for a given classname
=============
*/
const char* idGameEdit::GetUniqueEntityName( const char* classname ) const
{
	int			id;
	static char	name[1024];

	// can only have MAX_GENTITIES, so if we have a spot available, we're guaranteed to find one
	for( id = 0; id < MAX_GENTITIES; id++ )
	{
		idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
		if( !gameLocal.FindEntity( name ) )
		{
			return name;
		}
	}

	// id == MAX_GENTITIES + 1, which can't be in use if we get here
	idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
	return name;
}

/*
================
idGameEdit::EntityGetOrigin
================
*/
void  idGameEdit::EntityGetOrigin( idEntity* ent, idVec3& org ) const
{
	if( ent )
	{
		org = ent->GetPhysics()->GetOrigin();
	}
}

/*
================
idGameEdit::EntityGetAxis
================
*/
void idGameEdit::EntityGetAxis( idEntity* ent, idMat3& axis ) const
{
	if( ent )
	{
		axis = ent->GetPhysics()->GetAxis();
	}
}

/*
================
idGameEdit::EntitySetOrigin
================
*/
void idGameEdit::EntitySetOrigin( idEntity* ent, const idVec3& org )
{
	if( ent )
	{
		ent->SetOrigin( org );
	}
}

/*
================
idGameEdit::EntitySetAxis
================
*/
void idGameEdit::EntitySetAxis( idEntity* ent, const idMat3& axis )
{
	if( ent )
	{
		ent->SetAxis( axis );
	}
}

/*
================
idGameEdit::EntitySetColor
================
*/
void idGameEdit::EntitySetColor( idEntity* ent, const idVec3 color )
{
	if( ent )
	{
		ent->SetColor( color );
	}
}

/*
================
idGameEdit::EntityTranslate
================
*/
void idGameEdit::EntityTranslate( idEntity* ent, const idVec3& org )
{
	if( ent )
	{
		ent->GetPhysics()->Translate( org );
	}
}

/*
================
idGameEdit::EntityGetSpawnArgs
================
*/
const idDict* idGameEdit::EntityGetSpawnArgs( idEntity* ent ) const
{
	if( ent )
	{
		return &ent->spawnArgs;
	}
	return NULL;
}

/*
================
idGameEdit::EntityUpdateChangeableSpawnArgs
================
*/
void idGameEdit::EntityUpdateChangeableSpawnArgs( idEntity* ent, const idDict* dict )
{
	if( ent )
	{
		ent->UpdateChangeableSpawnArgs( dict );
	}
}

/*
================
idGameEdit::EntityChangeSpawnArgs
================
*/
void idGameEdit::EntityChangeSpawnArgs( idEntity* ent, const idDict* newArgs )
{
	if( ent )
	{
		for( int i = 0 ; i < newArgs->GetNumKeyVals() ; i ++ )
		{
			const idKeyValue* kv = newArgs->GetKeyVal( i );

			if( kv->GetValue().Length() > 0 )
			{
				ent->spawnArgs.Set( kv->GetKey() , kv->GetValue() );
			}
			else
			{
				ent->spawnArgs.Delete( kv->GetKey() );
			}
		}
	}
}

/*
================
idGameEdit::EntityUpdateVisuals
================
*/
void idGameEdit::EntityUpdateVisuals( idEntity* ent )
{
	if( ent )
	{
		ent->UpdateVisuals();
	}
}

/*
================
idGameEdit::EntitySetModel
================
*/
void idGameEdit::EntitySetModel( idEntity* ent, const char* val )
{
	if( ent )
	{
		ent->spawnArgs.Set( "model", val );
		ent->SetModel( val );
	}
}

/*
================
idGameEdit::EntityStopSound
================
*/
void idGameEdit::EntityStopSound( idEntity* ent )
{
	if( ent )
	{
		ent->StopSound( SND_CHANNEL_ANY, false );
	}
}

/*
================
idGameEdit::EntityDelete
================
*/
void idGameEdit::EntityDelete( idEntity* ent )
{
	delete ent;
}

/*
================
idGameEdit::PlayerIsValid
================
*/
bool idGameEdit::PlayerIsValid() const
{
	return ( gameLocal.GetLocalPlayer() != NULL );
}

/*
================
idGameEdit::PlayerGetOrigin
================
*/
void idGameEdit::PlayerGetOrigin( idVec3& org ) const
{
	org = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
}

/*
================
idGameEdit::PlayerGetAxis
================
*/
void idGameEdit::PlayerGetAxis( idMat3& axis ) const
{
	axis = gameLocal.GetLocalPlayer()->GetPhysics()->GetAxis();
}

/*
================
idGameEdit::PlayerGetViewAngles
================
*/
void idGameEdit::PlayerGetViewAngles( idAngles& angles ) const
{
	angles = gameLocal.GetLocalPlayer()->viewAngles;
}

/*
================
idGameEdit::PlayerGetEyePosition
================
*/
void idGameEdit::PlayerGetEyePosition( idVec3& org ) const
{
	org = gameLocal.GetLocalPlayer()->GetEyePosition();
}


/*
================
idGameEdit::MapGetEntityDict
================
*/
const idDict* idGameEdit::MapGetEntityDict( const char* name ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile && name && *name )
	{
		idMapEntity* mapent = mapFile->FindEntity( name );
		if( mapent )
		{
			return &mapent->epairs;
		}
	}
	return NULL;
}

/*
================
idGameEdit::MapSave
================
*/
void idGameEdit::MapSave( const char* path ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile )
	{
		mapFile->Write( ( path ) ? path : mapFile->GetName(), ".map" );
	}
}

/*
================
idGameEdit::MapSetEntityKeyVal
================
*/
void idGameEdit::MapSetEntityKeyVal( const char* name, const char* key, const char* val ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile && name && *name )
	{
		idMapEntity* mapent = mapFile->FindEntity( name );
		if( mapent )
		{
			mapent->epairs.Set( key, val );
		}
	}
}

/*
================
idGameEdit::MapCopyDictToEntity
================
*/
void idGameEdit::MapCopyDictToEntity( const char* name, const idDict* dict ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile && name && *name )
	{
		idMapEntity* mapent = mapFile->FindEntity( name );
		if( mapent )
		{
			for( int i = 0; i < dict->GetNumKeyVals(); i++ )
			{
				const idKeyValue* kv = dict->GetKeyVal( i );
				const char* key = kv->GetKey();
				const char* val = kv->GetValue();

				// DG: if val is "", delete key from the entity
				//     => same behavior as EntityChangeSpawnArgs()
				if( val[0] == '\0' )
				{
					mapent->epairs.Delete( key );
				}
				else
				{
					mapent->epairs.Set( key, val );
				}
				// DG end
			}
		}
	}
}

/*
================
RB idGameEdit::MapCopyDictToEntityAtOrigin
================
*/
void idGameEdit::MapCopyDictToEntityAtOrigin( const idVec3& origin, const idDict* dict ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile )//&& name && *name )
	{
		idMapEntity* mapent = mapFile->FindEntityAtOrigin( origin );
		if( mapent )
		{
			for( int i = 0; i < dict->GetNumKeyVals(); i++ )
			{
				const idKeyValue* kv = dict->GetKeyVal( i );
				const char* key = kv->GetKey();
				const char* val = kv->GetValue();

				// DG: if val is "", delete key from the entity
				//     => same behavior as EntityChangeSpawnArgs()
				if( val[0] == '\0' )
				{
					mapent->epairs.Delete( key );
				}
				else
				{
					mapent->epairs.Set( key, val );
				}
				// DG end
			}
		}
	}
}


/*
================
idGameEdit::MapGetUniqueMatchingKeyVals
================
*/
int idGameEdit::MapGetUniqueMatchingKeyVals( const char* key, const char* list[], int max ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if( mapFile )
	{
		for( int i = 0; i < mapFile->GetNumEntities(); i++ )
		{
			idMapEntity* ent = mapFile->GetEntity( i );
			if( ent )
			{
				const char* k = ent->epairs.GetString( key );
				if( k != NULL && *k != '\0' && count < max )
				{
					list[count++] = k;
				}
			}
		}
	}
	return count;
}

/*
================
idGameEdit::MapAddEntity
================
*/
void idGameEdit::MapAddEntity( const idDict* dict ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile )
	{
		idMapEntity* ent = new( TAG_GAME ) idMapEntity();
		ent->epairs = *dict;
		mapFile->AddEntity( ent );
	}
}

/*
================
idGameEdit::MapRemoveEntity
================
*/
void idGameEdit::MapRemoveEntity( const char* name ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile )
	{
		idMapEntity* ent = mapFile->FindEntity( name );
		if( ent )
		{
			mapFile->RemoveEntity( ent );
		}
	}
}


/*
================
idGameEdit::MapGetEntitiesMatchignClassWithString
================
*/
int idGameEdit::MapGetEntitiesMatchingClassWithString( const char* classname, const char* match, const char* list[], const int max ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if( mapFile )
	{
		int entCount = mapFile->GetNumEntities();
		for( int i = 0 ; i < entCount; i++ )
		{
			idMapEntity* ent = mapFile->GetEntity( i );
			if( ent )
			{
				idStr work = ent->epairs.GetString( "classname" );
				if( work.Icmp( classname ) == 0 )
				{
					if( match && *match )
					{
						work = ent->epairs.GetString( "soundgroup" );
						if( count < max && work.Icmp( match ) == 0 )
						{
							list[count++] = ent->epairs.GetString( "name" );
						}
					}
					else if( count < max )
					{
						list[count++] = ent->epairs.GetString( "name" );
					}
				}
			}
		}
	}
	return count;
}


/*
================
idGameEdit::MapEntityTranslate
================
*/
void idGameEdit::MapEntityTranslate( const char* name, const idVec3& v ) const
{
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if( mapFile && name && *name )
	{
		idMapEntity* mapent = mapFile->FindEntity( name );
		if( mapent )
		{
			idVec3 origin;
			mapent->epairs.GetVector( "origin", "", origin );
			origin += v;
			mapent->epairs.SetVector( "origin", origin );
		}
	}
}
