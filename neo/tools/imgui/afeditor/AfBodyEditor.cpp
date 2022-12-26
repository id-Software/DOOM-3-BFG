/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "AfBodyEditor.h"

#include "../imgui/BFGimgui.h"

static const char* bodyTypeNames[] =
{
	"none",
	"box",
	"octahedron",
	"dedecahedron",
	"cylinder",
	"cone",
	"bone",
	"polygon",
	"polygonvolume",
	"custom"
};

static const char* modifyJointsNames[] =
{
	"axis",
	"origin",
	"both"
};

static const char* afVecTypeNames[] =
{
	"coordinates",
	"joint",
	"bone center",
};

static bool ModelTypeItemGetter( void* data, int index, const char** out );

namespace ImGuiTools
{
AfBodyEditor::AfBodyEditor( idDeclAF* newDecl, idDeclAF_Body* newBody )
	: decl( newDecl )
	, body( newBody )
	, positionType( 0 )
	, modifyJointType( 0 )
	, comboJoint1( 0 )
	, comboJoint2( 0 )
	, originBoneCenterJoint1( 0 )
	, originBoneCenterJoint2( 0 )
	, originJoint( 0 )
	, contentWidget( MakePhysicsContentsSelector() )
{
	InitJointLists();
}

AfBodyEditor::~AfBodyEditor()
{
}

bool AfBodyEditor::Do()
{
	ImGui::PushID( body->name );

	bool changed = false;

	ImGui::InputText( "##rename", &renameBody );
	ImGui::SameLine();
	if( ImGui::Button( "Rename" ) )
	{
		if( renameBody.Allocated() > 0 )
		{
			decl->RenameBody( body->name, renameBody.c_str() );
			renameBody.Clear();
		}
		changed = true;
	}

	if( ImGui::CollapsingHeader( "Collision Model" ) )
	{
		changed = CollisionModel() || changed;
	}

	if( ImGui::CollapsingHeader( "Position" ) )
	{
		changed = Position() || changed;
	}

	if( ImGui::CollapsingHeader( "Collision Detection" ) )
	{
		changed = ImGui::Checkbox( "Self Collision", &body->selfCollision ) || changed;
		changed = DoMultiSelect( &contentWidget, &body->contents ) || changed;
	}

	if( ImGui::CollapsingHeader( "Friction" ) )
	{
		changed = ImGui::DragFloat( "Linear", &body->linearFriction ) || changed;
		changed = ImGui::DragFloat( "Angular", &body->angularFriction ) || changed;
		changed = ImGui::DragFloat( "Contact", &body->contactFriction ) || changed;
		changed = InputAfVector( "Friction Direction", &body->frictionDirection ) || changed;
		changed = InputAfVector( "Motor Direction", &body->contactMotorDirection ) || changed;
	}

	if( ImGui::CollapsingHeader( "Joints" ) )
	{
		if( ImGui::Combo( "Modified Joint", &comboJoint1, StringListItemGetter, &joints, joints.Num() ) )
		{
			body->jointName = joints[comboJoint1];
			changed = true;
		}
		changed = ImGui::Combo( "Modify", ( int* )&body->jointMod, modifyJointsNames, ARRAY_COUNT( modifyJointsNames ) ) || changed;
		changed = ImGui::InputText( "Contained Joints", &body->containedJoints ) || changed;
	}

	ImGui::PopID();

	return changed;
}

void AfBodyEditor::InitJointLists()
{
	const idRenderModel* model = gameEdit->ANIM_GetModelFromName( decl->model.c_str() );
	if( !model )
	{
		return;
	}

	const int numJoints = model->NumJoints();
	for( int i = 0; i < numJoints; i++ )
	{
		const char* jointName = model->GetJointName( ( jointHandle_t )i );
		joints[joints.Append( jointName )].ToLower();
	}
}

bool AfBodyEditor::Position()
{
	bool changed = false;

	ImGui::Columns( 2, "positionColumns2" );

	changed = ImGui::Combo( "Type", ( int* )&body->origin.type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;

	ImGui::NextColumn();
	changed = PositionProperty() || changed;
	ImGui::NextColumn();
	changed = PitchYawRoll() || changed;

	ImGui::Columns( 1 );

	return changed;
}

bool AfBodyEditor::PositionProperty()
{
	bool changed = false;

	idAFVector* afVec = &body->origin;

	switch( afVec->type )
	{
		case idAFVector::VEC_COORDS:
			changed = ImGui::DragFloat3( "Coordinates", ( float* )&afVec->ToVec3() ) || changed;
			break;
		case idAFVector::VEC_JOINT:
			if( ImGui::BeginCombo( "Joint", afVec->joint1.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint1 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						changed = true;
						afVec->joint1 = joints[n];
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			break;
		case idAFVector::VEC_BONECENTER:
		case idAFVector::VEC_BONEDIR:
			if( ImGui::BeginCombo( "Joint 1", afVec->joint1.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint1 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						changed = true;
						afVec->joint1 = joints[n];
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			if( ImGui::BeginCombo( "Joint 2", afVec->joint2.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint2 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						changed = true;
						afVec->joint2 = joints[n];
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			break;
		default:
			ImGui::Text( "*Unknown Type*" );
	}

	return changed;
}

bool AfBodyEditor::PitchYawRoll()
{
	bool changed = false;
	const ImGuiStyle& style = ImGui::GetStyle();
	const float w_full = ImGui::CalcItemWidth();
	const float square_sz = ImGui::GetFrameHeight();
	const float w_button = square_sz + style.ItemInnerSpacing.x;
	const float w_inputs = w_full - w_button;

	const float widthItemOne = Max( 1.0f, ( float )( int )( ( w_inputs - ( style.ItemInnerSpacing.x ) * 2 ) / 2.0f ) );
	const float widthItemLast = Max( 1.0f, ( float )( int )( w_inputs - ( widthItemOne + style.ItemInnerSpacing.x ) ) );

	ImGui::SetNextItemWidth( widthItemOne );
	changed = ImGui::DragFloat( "##p", &body->angles.pitch, 1.0f, 0.0f, 360.0f, "pitch: %.3f" ) || changed;
	ImGui::SameLine();
	ImGui::SetNextItemWidth( widthItemOne );
	changed = ImGui::DragFloat( "##y", &body->angles.yaw, 1.0f, 0.0f, 360.0f, "yaw: %.3f" ) || changed;
	ImGui::SameLine();
	ImGui::SetNextItemWidth( widthItemLast );
	changed = ImGui::DragFloat( "##r", &body->angles.roll, 1.0f, 0.0f, 360.0f, "roll: %.3f" );

	return changed;
}

bool AfBodyEditor::CollisionModel()
{
	bool changed = false;

	ImGui::PushID( "CollisionModel" );

	ImGui::Columns( 2, "collisonColumns2" );
	changed = ImGui::Combo( "Model Type", &body->modelType, ModelTypeItemGetter, nullptr, ARRAY_COUNT( bodyTypeNames ) ) || changed;
	changed = ImGui::DragFloat( "Density", &body->density ) || changed;
	ImGui::NextColumn();
	changed = CollisionModelSize() || changed;
	ImGui::Columns( 1 );

	ImGui::PopID();

	return changed;
}

bool AfBodyEditor::CollisionModelSize()
{
	bool changed = false;

	switch( body->modelType )
	{
		case TRM_INVALID:
			ImGui::Text( "Provide a valid model type." );
			break;
		case TRM_BOX:
		case TRM_OCTAHEDRON:
		case TRM_DODECAHEDRON:
			changed = InputAfVector( "min", &body->v1 ) || changed;
			changed = InputAfVector( "max", &body->v2 ) || changed;
			ImGui::NewLine();
			break;
		case TRM_CYLINDER:
		case TRM_CONE:
			changed = InputAfVector( "min", &body->v1 ) || changed;
			changed = InputAfVector( "max", &body->v2 ) || changed;
			changed = ImGui::InputInt( "Sides", &body->numSides ) || changed;
			break;
		case TRM_BONE:
			changed = InputAfVector( "min", &body->v1 ) || changed;
			changed = InputAfVector( "max", &body->v2 ) || changed;
			changed = ImGui::DragFloat( "Width", &body->width ) || changed;
			break;
		case TRM_POLYGON:
			ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "Polygon models are not supported :(" );
			break;
		case TRM_POLYGONVOLUME:
			ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "Polygon Volume models are not supported :(" );
			break;
		case TRM_CUSTOM:
			ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "Custom models are not supported :(" );
			break;
	}

	return changed;
}

bool AfBodyEditor::InputAfVector( const char* label, idAFVector* vec )
{
	return ImGui::DragFloat3( label, ( float* )&vec->ToVec3() );
}

}


static bool ModelTypeItemGetter( void* data, int index, const char** out )
{
	if( index < 0 || index >= ARRAY_COUNT( bodyTypeNames ) )
	{
		return false;
	}

	*out = bodyTypeNames[index];

	return true;
}