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
/*
=============================================================================

  MODEL TESTING

Model viewing can begin with either "testmodel <modelname>"

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Extension will default to ".ase" if not specified.

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

  g_testModelRotate
  g_testModelAnimate
  g_testModelBlend

=============================================================================
*/

#include "precompiled.h"
#pragma hdrstop


#include "../Game_local.h"

CLASS_DECLARATION( idAnimatedEntity, idTestModel )
EVENT( EV_FootstepLeft,			idTestModel::Event_Footstep )
EVENT( EV_FootstepRight,		idTestModel::Event_Footstep )
END_CLASS

/*
================
idTestModel::idTestModel
================
*/
idTestModel::idTestModel()
{
	head = NULL;
	headAnimator = NULL;
	anim = 0;
	headAnim = 0;
	starttime = 0;
	animtime = 0;
	mode = 0;
	frame = 0;
}

/*
================
idTestModel::Save
================
*/
void idTestModel::Save( idSaveGame* savefile )
{
}

/*
================
idTestModel::Restore
================
*/
void idTestModel::Restore( idRestoreGame* savefile )
{
	// FIXME: one day we may actually want to save/restore test models, but for now we'll just delete them
	delete this;
}

/*
================
idTestModel::Spawn
================
*/
void idTestModel::Spawn()
{
	idVec3				size;
	idBounds			bounds;
	const char*			headModel;
	jointHandle_t		joint;
	idStr				jointName;
	idVec3				origin, modelOffset;
	idMat3				axis;
	const idKeyValue*	kv;
	copyJoints_t		copyJoint;

	if( renderEntity.hModel && renderEntity.hModel->IsDefaultModel() && !animator.ModelDef() )
	{
		gameLocal.Warning( "Unable to create testmodel for '%s' : model defaulted", spawnArgs.GetString( "model" ) );
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	mode = g_testModelAnimate.GetInteger();
	animator.RemoveOriginOffset( g_testModelAnimate.GetInteger() == 1 );

	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );

	if( spawnArgs.GetVector( "mins", NULL, bounds[0] ) )
	{
		spawnArgs.GetVector( "maxs", NULL, bounds[1] );
		physicsObj.SetClipBox( bounds, 1.0f );
		physicsObj.SetContents( 0 );
	}
	else if( spawnArgs.GetVector( "size", NULL, size ) )
	{
		bounds[ 0 ].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
		bounds[ 1 ].Set( size.x * 0.5f, size.y * 0.5f, size.z );
		physicsObj.SetClipBox( bounds, 1.0f );
		physicsObj.SetContents( 0 );
	}

	spawnArgs.GetVector( "offsetModel", "0 0 0", modelOffset );

	// add the head model if it has one
	headModel = spawnArgs.GetString( "def_head", "" );
	if( headModel[ 0 ] )
	{
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if( joint == INVALID_JOINT )
		{
			gameLocal.Warning( "Joint '%s' not found for 'head_joint'", jointName.c_str() );
		}
		else
		{
			// copy any sounds in case we have frame commands on the head
			idDict				args;
			const idKeyValue*	sndKV = spawnArgs.MatchPrefix( "snd_", NULL );
			while( sndKV )
			{
				args.Set( sndKV->GetKey(), sndKV->GetValue() );
				sndKV = spawnArgs.MatchPrefix( "snd_", sndKV );
			}

			head = gameLocal.SpawnEntityType( idAnimatedEntity::Type, &args );
			animator.GetJointTransform( joint, gameLocal.time, origin, axis );
			origin = GetPhysics()->GetOrigin() + ( origin + modelOffset ) * GetPhysics()->GetAxis();
			head.GetEntity()->SetModel( headModel );
			head.GetEntity()->SetOrigin( origin );
			head.GetEntity()->SetAxis( GetPhysics()->GetAxis() );
			head.GetEntity()->BindToJoint( this, animator.GetJointName( joint ), true );

			headAnimator = head.GetEntity()->GetAnimator();

			// set up the list of joints to copy to the head
			for( kv = spawnArgs.MatchPrefix( "copy_joint", NULL ); kv != NULL; kv = spawnArgs.MatchPrefix( "copy_joint", kv ) )
			{
				jointName = kv->GetKey();

				// RB: TrenchBroom interop use copy_joint_world.<name> instead so we can build this up using the FGD files
				if( jointName.StripLeadingOnce( "copy_joint_world " ) || jointName.StripLeadingOnce( "copy_joint_world." ) )
				{
					copyJoint.mod = JOINTMOD_WORLD_OVERRIDE;
				}
				else
				{
					if( !jointName.StripLeadingOnce( "copy_joint " ) )
					{
						jointName.StripLeadingOnce( "copy_joint." );
					}
					copyJoint.mod = JOINTMOD_LOCAL_OVERRIDE;
				}
				// RB end

				copyJoint.from = animator.GetJointHandle( jointName );
				if( copyJoint.from == INVALID_JOINT )
				{
					gameLocal.Warning( "Unknown copy_joint '%s'", jointName.c_str() );
					continue;
				}

				copyJoint.to = headAnimator->GetJointHandle( jointName );
				if( copyJoint.to == INVALID_JOINT )
				{
					gameLocal.Warning( "Unknown copy_joint '%s' on head", jointName.c_str() );
					continue;
				}

				copyJoints.Append( copyJoint );
			}
		}
	}

	// start any shader effects based off of the spawn time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	SetPhysics( &physicsObj );

	gameLocal.Printf( "Added testmodel at origin = '%s',  angles = '%s'\n", GetPhysics()->GetOrigin().ToString(), GetPhysics()->GetAxis().ToAngles().ToString() );
	BecomeActive( TH_THINK );
}

/*
================
idTestModel::~idTestModel
================
*/
idTestModel::~idTestModel()
{
	StopSound( SND_CHANNEL_ANY, false );
	if( renderEntity.hModel )
	{
		gameLocal.Printf( "Removing testmodel %s\n", renderEntity.hModel->Name() );
	}
	else
	{
		gameLocal.Printf( "Removing testmodel\n" );
	}
	if( gameLocal.testmodel == this )
	{
		gameLocal.testmodel = NULL;
	}
	if( head.GetEntity() )
	{
		head.GetEntity()->StopSound( SND_CHANNEL_ANY, false );
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
}

/*
===============
idTestModel::Event_Footstep
===============
*/
void idTestModel::Event_Footstep()
{
	StartSound( "snd_footstep", SND_CHANNEL_BODY, 0, false, NULL );
}

/*
================
idTestModel::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idTestModel::ShouldConstructScriptObjectAtSpawn() const
{
	return false;
}

/*
================
idTestModel::Think
================
*/
void idTestModel::Think()
{
	idVec3 pos;
	idMat3 axis;
	idAngles ang;
	int	i;

	if( thinkFlags & TH_THINK )
	{
		if( anim && ( gameLocal.testmodel == this ) && ( mode != g_testModelAnimate.GetInteger() ) )
		{
			StopSound( SND_CHANNEL_ANY, false );
			if( head.GetEntity() )
			{
				head.GetEntity()->StopSound( SND_CHANNEL_ANY, false );
			}
			switch( g_testModelAnimate.GetInteger() )
			{
				default:
				case 0:
					// cycle anim with origin reset
					if( animator.NumFrames( anim ) <= 1 )
					{
						// single frame animations end immediately, so just cycle it since it's the same result
						animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
						if( headAnim )
						{
							headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
						}
					}
					else
					{
						animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
						if( headAnim )
						{
							headAnimator->PlayAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
							if( headAnimator->AnimLength( headAnim ) > animator.AnimLength( anim ) )
							{
								// loop the body anim when the head anim is longer
								animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( -1 );
							}
						}
					}
					animator.RemoveOriginOffset( false );
					break;

				case 1:
					// cycle anim with fixed origin
					animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					animator.RemoveOriginOffset( true );
					if( headAnim )
					{
						headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					}
					break;

				case 2:
					// cycle anim with continuous origin
					animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					animator.RemoveOriginOffset( false );
					if( headAnim )
					{
						headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					}
					break;

				case 3:
					// frame by frame with continuous origin
					animator.SetFrame( ANIMCHANNEL_ALL, anim, frame, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					animator.RemoveOriginOffset( false );
					if( headAnim )
					{
						headAnimator->SetFrame( ANIMCHANNEL_ALL, headAnim, frame, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					}
					break;

				case 4:
					// play anim once
					animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					animator.RemoveOriginOffset( false );
					if( headAnim )
					{
						headAnimator->PlayAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					}
					break;

				case 5:
					// frame by frame with fixed origin
					animator.SetFrame( ANIMCHANNEL_ALL, anim, frame, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					animator.RemoveOriginOffset( true );
					if( headAnim )
					{
						headAnimator->SetFrame( ANIMCHANNEL_ALL, headAnim, frame, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
					}
					break;
			}

			mode = g_testModelAnimate.GetInteger();
		}

		if( ( mode == 0 ) && ( gameLocal.time >= starttime + animtime ) )
		{
			starttime = gameLocal.time;
			StopSound( SND_CHANNEL_ANY, false );
			animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
			if( headAnim )
			{
				headAnimator->PlayAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, FRAME2MS( g_testModelBlend.GetInteger() ) );
				if( headAnimator->AnimLength( headAnim ) > animator.AnimLength( anim ) )
				{
					// loop the body anim when the head anim is longer
					animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( -1 );
				}
			}
		}

		if( headAnimator )
		{
			// copy the animation from the body to the head
			for( i = 0; i < copyJoints.Num(); i++ )
			{
				if( copyJoints[ i ].mod == JOINTMOD_WORLD_OVERRIDE )
				{
					idMat3 mat = head.GetEntity()->GetPhysics()->GetAxis().Transpose();
					GetJointWorldTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
					pos -= head.GetEntity()->GetPhysics()->GetOrigin();
					headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos * mat );
					headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis * mat );
				}
				else
				{
					animator.GetJointLocalTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
					headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos );
					headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis );
				}
			}
		}

		// update rotation
		RunPhysics();

		physicsObj.GetAngles( ang );
		physicsObj.SetAngularExtrapolation( extrapolation_t( EXTRAPOLATION_LINEAR | EXTRAPOLATION_NOSTOP ), gameLocal.time, 0, ang, idAngles( 0, g_testModelRotate.GetFloat() * 360.0f / 60.0f, 0 ), ang_zero );

		idClipModel* clip = physicsObj.GetClipModel();
		if( clip != NULL && animator.ModelDef() )
		{
			idVec3 neworigin;
			idMat3 axis;
			jointHandle_t joint;

			joint = animator.GetJointHandle( "origin" );
			animator.GetJointTransform( joint, gameLocal.time, neworigin, axis );
			neworigin = ( ( neworigin - animator.ModelDef()->GetVisualOffset() ) * physicsObj.GetAxis() ) + GetPhysics()->GetOrigin();
			clip->Link( gameLocal.clip, this, 0, neworigin, clip->GetAxis() );
		}
	}

	UpdateAnimation();
	Present();

	if( ( gameLocal.testmodel == this ) && g_showTestModelFrame.GetInteger() && anim )
	{
		gameLocal.Printf( "^5 Anim: ^7%s  ^5Frame: ^7%d/%d  Time: %.3f\n", animator.AnimFullName( anim ), animator.CurrentAnim( ANIMCHANNEL_ALL )->GetFrameNumber( gameLocal.time ),
						  animator.CurrentAnim( ANIMCHANNEL_ALL )->NumFrames(), MS2SEC( gameLocal.time - animator.CurrentAnim( ANIMCHANNEL_ALL )->GetStartTime() ) );
		if( headAnim )
		{
			gameLocal.Printf( "^5 Head: ^7%s  ^5Frame: ^7%d/%d  Time: %.3f\n\n", headAnimator->AnimFullName( headAnim ), headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetFrameNumber( gameLocal.time ),
							  headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->NumFrames(), MS2SEC( gameLocal.time - headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetStartTime() ) );
		}
		else
		{
			gameLocal.Printf( "\n\n" );
		}
	}
}

/*
================
idTestModel::NextAnim
================
*/
void idTestModel::NextAnim( const idCmdArgs& args )
{
	if( !animator.NumAnims() )
	{
		return;
	}

	anim++;
	if( anim >= animator.NumAnims() )
	{
		// anim 0 is no anim
		anim = 1;
	}

	starttime = gameLocal.time;
	animtime = animator.AnimLength( anim );
	animname = animator.AnimFullName( anim );
	headAnim = 0;
	if( headAnimator )
	{
		headAnimator->ClearAllAnims( gameLocal.time, 0 );
		headAnim = headAnimator->GetAnim( animname );
		if( !headAnim )
		{
			headAnim = headAnimator->GetAnim( "idle" );
		}

		if( headAnim && ( headAnimator->AnimLength( headAnim ) > animtime ) )
		{
			animtime = headAnimator->AnimLength( headAnim );
		}
	}

	gameLocal.Printf( "anim '%s', %d.%03d seconds, %d frames\n", animname.c_str(), animator.AnimLength( anim ) / 1000, animator.AnimLength( anim ) % 1000, animator.NumFrames( anim ) );
	if( headAnim )
	{
		gameLocal.Printf( "head '%s', %d.%03d seconds, %d frames\n", headAnimator->AnimFullName( headAnim ), headAnimator->AnimLength( headAnim ) / 1000, headAnimator->AnimLength( headAnim ) % 1000, headAnimator->NumFrames( headAnim ) );
	}

	// reset the anim
	mode = -1;
	frame = 1;
}

/*
================
idTestModel::PrevAnim
================
*/
void idTestModel::PrevAnim( const idCmdArgs& args )
{
	if( !animator.NumAnims() )
	{
		return;
	}

	anim--;
	if( anim < 0 )
	{
		anim = animator.NumAnims() - 1;
	}

	starttime = gameLocal.time;
	animtime = animator.AnimLength( anim );
	animname = animator.AnimFullName( anim );
	headAnim = 0;
	if( headAnimator )
	{
		headAnimator->ClearAllAnims( gameLocal.time, 0 );
		headAnim = headAnimator->GetAnim( animname );
		if( !headAnim )
		{
			headAnim = headAnimator->GetAnim( "idle" );
		}

		if( headAnim && ( headAnimator->AnimLength( headAnim ) > animtime ) )
		{
			animtime = headAnimator->AnimLength( headAnim );
		}
	}

	gameLocal.Printf( "anim '%s', %d.%03d seconds, %d frames\n", animname.c_str(), animator.AnimLength( anim ) / 1000, animator.AnimLength( anim ) % 1000, animator.NumFrames( anim ) );
	if( headAnim )
	{
		gameLocal.Printf( "head '%s', %d.%03d seconds, %d frames\n", headAnimator->AnimFullName( headAnim ), headAnimator->AnimLength( headAnim ) / 1000, headAnimator->AnimLength( headAnim ) % 1000, headAnimator->NumFrames( headAnim ) );
	}

	// reset the anim
	mode = -1;
	frame = 1;
}

/*
================
idTestModel::NextFrame
================
*/
void idTestModel::NextFrame( const idCmdArgs& args )
{
	if( !anim || ( ( g_testModelAnimate.GetInteger() != 3 ) && ( g_testModelAnimate.GetInteger() != 5 ) ) )
	{
		return;
	}

	frame++;
	if( frame > animator.NumFrames( anim ) )
	{
		frame = 1;
	}

	gameLocal.Printf( "^5 Anim: ^7%s\n^5Frame: ^7%d/%d\n\n", animator.AnimFullName( anim ), frame, animator.NumFrames( anim ) );

	// reset the anim
	mode = -1;
}

/*
================
idTestModel::PrevFrame
================
*/
void idTestModel::PrevFrame( const idCmdArgs& args )
{
	if( !anim || ( ( g_testModelAnimate.GetInteger() != 3 ) && ( g_testModelAnimate.GetInteger() != 5 ) ) )
	{
		return;
	}

	frame--;
	if( frame < 1 )
	{
		frame = animator.NumFrames( anim );
	}

	gameLocal.Printf( "^5 Anim: ^7%s\n^5Frame: ^7%d/%d\n\n", animator.AnimFullName( anim ), frame, animator.NumFrames( anim ) );

	// reset the anim
	mode = -1;
}

/*
================
idTestModel::TestAnim
================
*/
void idTestModel::TestAnim( const idCmdArgs& args )
{
	idStr			name;
	int				animNum;
	const idAnim*	newanim;

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "usage: testanim <animname>\n" );
		return;
	}

	newanim = NULL;

	name = args.Argv( 1 );
	animNum = animator.GetAnim( name );

	if( !animNum )
	{
		gameLocal.Printf( "Animation '%s' not found.\n", name.c_str() );
		return;
	}

	anim = animNum;
	starttime = gameLocal.time;
	animtime = animator.AnimLength( anim );
	headAnim = 0;
	if( headAnimator )
	{
		headAnimator->ClearAllAnims( gameLocal.time, 0 );
		headAnim = headAnimator->GetAnim( animname );
		if( !headAnim )
		{
			headAnim = headAnimator->GetAnim( "idle" );
			if( !headAnim )
			{
				gameLocal.Printf( "Missing 'idle' anim for head.\n" );
			}
		}

		if( headAnim && ( headAnimator->AnimLength( headAnim ) > animtime ) )
		{
			animtime = headAnimator->AnimLength( headAnim );
		}
	}

	animname = name;
	gameLocal.Printf( "anim '%s', %d.%03d seconds, %d frames\n", animname.c_str(), animator.AnimLength( anim ) / 1000, animator.AnimLength( anim ) % 1000, animator.NumFrames( anim ) );

	// reset the anim
	mode = -1;
}

/*
=====================
idTestModel::BlendAnim
=====================
*/
void idTestModel::BlendAnim( const idCmdArgs& args )
{
	int anim1;
	int anim2;

	if( args.Argc() < 4 )
	{
		gameLocal.Printf( "usage: testblend <anim1> <anim2> <frames>\n" );
		return;
	}

	anim1 = gameLocal.testmodel->animator.GetAnim( args.Argv( 1 ) );
	if( !anim1 )
	{
		gameLocal.Printf( "Animation '%s' not found.\n", args.Argv( 1 ) );
		return;
	}

	anim2 = gameLocal.testmodel->animator.GetAnim( args.Argv( 2 ) );
	if( !anim2 )
	{
		gameLocal.Printf( "Animation '%s' not found.\n", args.Argv( 2 ) );
		return;
	}

	animname = args.Argv( 2 );
	animator.CycleAnim( ANIMCHANNEL_ALL, anim1, gameLocal.time, 0 );
	animator.CycleAnim( ANIMCHANNEL_ALL, anim2, gameLocal.time, FRAME2MS( atoi( args.Argv( 3 ) ) ) );

	anim = anim2;
	headAnim = 0;
}

/***********************************************************************

	Testmodel console commands

***********************************************************************/

/*
=================
idTestModel::KeepTestModel_f

Makes the current test model permanent, allowing you to place
multiple test models
=================
*/
void idTestModel::KeepTestModel_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No active testModel.\n" );
		return;
	}

	gameLocal.Printf( "modelDef %p kept\n", gameLocal.testmodel->renderEntity.hModel );

	gameLocal.testmodel = NULL;
}

/*
=================
idTestModel::TestSkin_f

Sets a skin on an existing testModel
=================
*/
void idTestModel::TestSkin_f( const idCmdArgs& args )
{
	idVec3		offset;
	idStr		name;
	idPlayer* 	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	// delete the testModel if active
	if( !gameLocal.testmodel )
	{
		common->Printf( "No active testModel\n" );
		return;
	}

	if( args.Argc() < 2 )
	{
		common->Printf( "removing testSkin.\n" );
		gameLocal.testmodel->SetSkin( NULL );
		return;
	}

	name = args.Argv( 1 );
	gameLocal.testmodel->SetSkin( declManager->FindSkin( name ) );
}

/*
=================
idTestModel::TestShaderParm_f

Sets a shaderParm on an existing testModel
=================
*/
void idTestModel::TestShaderParm_f( const idCmdArgs& args )
{
	idVec3		offset;
	idStr		name;
	idPlayer* 	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	// delete the testModel if active
	if( !gameLocal.testmodel )
	{
		common->Printf( "No active testModel\n" );
		return;
	}

	if( args.Argc() != 3 )
	{
		common->Printf( "USAGE: testShaderParm <parmNum> <float | \"time\">\n" );
		return;
	}

	int	parm = atoi( args.Argv( 1 ) );
	if( parm < 0 || parm >= MAX_ENTITY_SHADER_PARMS )
	{
		common->Printf( "parmNum %i out of range\n", parm );
		return;
	}

	float	value;
	if( !idStr::Icmp( args.Argv( 2 ), "time" ) )
	{
		value = gameLocal.time * -0.001;
	}
	else
	{
		value = atof( args.Argv( 2 ) );
	}

	gameLocal.testmodel->SetShaderParm( parm, value );
}

/*
=================
idTestModel::TestModel_f

Creates a static modelDef in front of the current position, which
can then be moved around
=================
*/
void idTestModel::TestModel_f( const idCmdArgs& args )
{
	idVec3			offset;
	idStr			name;
	idPlayer* 		player;
	const idDict* 	entityDef;
	idDict			dict;

	player = gameLocal.GetLocalPlayer();
	if( !player || !gameLocal.CheatsOk() )
	{
		return;
	}

	// delete the testModel if active
	if( gameLocal.testmodel )
	{
		delete gameLocal.testmodel;
		gameLocal.testmodel = NULL;
	}

	if( args.Argc() < 2 )
	{
		return;
	}

	name = args.Argv( 1 );

	entityDef = gameLocal.FindEntityDefDict( name, false );
	if( entityDef )
	{
		dict = *entityDef;
	}
	else
	{
		if( declManager->FindType( DECL_MODELDEF, name, false ) )
		{
			dict.Set( "model", name );
		}
		else
		{
			// allow map models with underscore prefixes to be tested during development
			// without appending an ase
			if( name[ 0 ] != '_' )
			{
				name.DefaultFileExtension( ".ase" );
			}
			if( !renderModelManager->CheckModel( name ) )
			{
				gameLocal.Printf( "Can't register model\n" );
				return;
			}
			dict.Set( "model", name );
		}
	}

	offset = player->GetPhysics()->GetOrigin() + player->viewAngles.ToForward() * 100.0f;

	dict.Set( "origin", offset.ToString() );
	dict.Set( "angle", va( "%f", player->viewAngles.yaw + 180.0f ) );
	gameLocal.testmodel = ( idTestModel* )gameLocal.SpawnEntityType( idTestModel::Type, &dict );
	gameLocal.testmodel->renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
}

/*
=====================
idTestModel::ArgCompletion_TestModel
=====================
*/
void idTestModel::ArgCompletion_TestModel( const idCmdArgs& args, void( *callback )( const char* s ) )
{
	int i, num;

	num = declManager->GetNumDecls( DECL_ENTITYDEF );
	for( i = 0; i < num; i++ )
	{
		callback( idStr( args.Argv( 0 ) ) + " " + declManager->DeclByIndex( DECL_ENTITYDEF, i , false )->GetName() );
	}
	num = declManager->GetNumDecls( DECL_MODELDEF );
	for( i = 0; i < num; i++ )
	{
		callback( idStr( args.Argv( 0 ) ) + " " + declManager->DeclByIndex( DECL_MODELDEF, i , false )->GetName() );
	}
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "models/", false, ".lwo", ".ase", ".md5mesh", ".ma", ".mb", NULL );
}

/*
=====================
idTestModel::TestParticleStopTime_f
=====================
*/
void idTestModel::TestParticleStopTime_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( gameLocal.time );
	gameLocal.testmodel->UpdateVisuals();
}

/*
=====================
idTestModel::TestAnim_f
=====================
*/
void idTestModel::TestAnim_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->TestAnim( args );
}


/*
=====================
idTestModel::ArgCompletion_TestAnim
=====================
*/
void idTestModel::ArgCompletion_TestAnim( const idCmdArgs& args, void( *callback )( const char* s ) )
{
	if( gameLocal.testmodel )
	{
		idAnimator* animator = gameLocal.testmodel->GetAnimator();
		for( int i = 0; i < animator->NumAnims(); i++ )
		{
			callback( va( "%s %s", args.Argv( 0 ), animator->AnimFullName( i ) ) );
		}
	}
}

/*
=====================
idTestModel::TestBlend_f
=====================
*/
void idTestModel::TestBlend_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->BlendAnim( args );
}

/*
=====================
idTestModel::TestModelNextAnim_f
=====================
*/
void idTestModel::TestModelNextAnim_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->NextAnim( args );
}

/*
=====================
idTestModel::TestModelPrevAnim_f
=====================
*/
void idTestModel::TestModelPrevAnim_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->PrevAnim( args );
}

/*
=====================
idTestModel::TestModelNextFrame_f
=====================
*/
void idTestModel::TestModelNextFrame_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->NextFrame( args );
}

/*
=====================
idTestModel::TestModelPrevFrame_f
=====================
*/
void idTestModel::TestModelPrevFrame_f( const idCmdArgs& args )
{
	if( !gameLocal.testmodel )
	{
		gameLocal.Printf( "No testModel active.\n" );
		return;
	}

	gameLocal.testmodel->PrevFrame( args );
}
